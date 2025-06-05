#include "beat_detector_simple_energy.h"
#include "logging.h"
#include "../../configuration/configuration_manager.h"
#include <algorithm>
#include <cmath>

SimpleEnergyBeatDetector::SimpleEnergyBeatDetector() 
    : is_running_(false), 
      last_beat_time_(0.0f),
      total_time_(0.0f),      beat_value_(0.0f),
      flux_threshold_(0.0f),
      beat_falloff_(Listeningway::ConfigurationManager::Snapshot().beat.falloffDefault)
{
    result_.beat = 0.0f;
    result_.tempo_detected = false; // Simple detector doesn't detect tempo
    LOG_DEBUG("[SimpleEnergyBeatDetector] Created");
}

SimpleEnergyBeatDetector::~SimpleEnergyBeatDetector() {
    Stop();
    LOG_DEBUG("[SimpleEnergyBeatDetector] Destroyed");
}

void SimpleEnergyBeatDetector::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_running_) {
        return;
    }
    
    is_running_ = true;
    last_beat_time_ = 0.0f;
    total_time_ = 0.0f;
    beat_value_ = 0.0f;
    flux_threshold_ = 0.0f;
    last_beat_timestamp_ = std::chrono::steady_clock::now();
    
    LOG_DEBUG("[SimpleEnergyBeatDetector] Started");
}

void SimpleEnergyBeatDetector::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_running_) {
        return;
    }
    
    is_running_ = false;
    LOG_DEBUG("[SimpleEnergyBeatDetector] Stopped");
}

void SimpleEnergyBeatDetector::Process(const std::vector<float>& magnitudes, float flux, float flux_low, float dt) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_running_) {
        return;
    }
    
    // Update total time
    total_time_ += dt;
    
    // Band-limited flux for beat detection (using low frequency range for better beat detection)
    float beat_flux = flux_low;
    
    // Use a smoother threshold adaptation by adjusting the alpha values
    // Slower adaptation means more stable threshold (less jittery beats)
    flux_threshold_ = flux_threshold_ * 0.98f + beat_flux * 0.02f;    // Check if this is a beat
    bool is_beat = false;
    const auto config = Listeningway::ConfigurationManager::Snapshot(); // Thread-safe snapshot for beat detection
    if (beat_flux > flux_threshold_ * config.beat.fluxLowThresholdMultiplier && 
        beat_flux > config.beat.fluxMin) {
        
        // Require minimum time between beats
        float time_since_last_beat = total_time_ - last_beat_time_;
        if (time_since_last_beat > 0.2f) { // Don't detect beats faster than 300 BPM (0.2 seconds apart)
            is_beat = true;
            last_beat_time_ = total_time_;
            
            // Record actual timestamp for real-time measurements
            auto now = std::chrono::steady_clock::now();
            last_beat_timestamp_ = now;
            
            // Set beat value high
            beat_value_ = 1.0f;
            
            // Use more advanced adaptive falloff based on time between beats            
            // This was the original algorithm from Listeningway
            if (time_since_last_beat > 0.0f) {
                // Calculate beat falloff based on elapsed time
                beat_falloff_ = config.beat.falloffDefault;
                
                // Adjust falloff based on time between beats
                // Reduce the divisor to make the decay slower
                float adaptive_time = config.beat.timeScale * std::exp(time_since_last_beat) + 
                                     config.beat.timeInitial;
                adaptive_time = std::max(adaptive_time, config.beat.timeMin);
                
                // Reduce the falloff rate by using a smaller divisor
                beat_falloff_ = beat_falloff_ / (adaptive_time * config.beat.timeDivisor * 2.0f);
            }
        }
    }
    
    // Decay beat value over time with a more gradual falloff curve
    // Use a non-linear decay curve that's slower at the beginning
    float decay_amount = beat_falloff_ * dt;
    
    // Apply non-linear decay that slows down as the beat value decreases
    // This creates a more natural sounding decay
    if (beat_value_ > 0.5f) {
        // Faster decay for high values
        beat_value_ = std::max(0.0f, beat_value_ - decay_amount);
    } else {
        // Slower decay for lower values
        beat_value_ = std::max(0.0f, beat_value_ - decay_amount * 0.6f * beat_value_);
    }
    
    // Update result
    result_.beat = beat_value_;
}

BeatDetectorResult SimpleEnergyBeatDetector::GetResult() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return result_;
}