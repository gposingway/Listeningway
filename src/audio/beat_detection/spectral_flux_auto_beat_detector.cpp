#include "spectral_flux_auto_beat_detector.h"
#include "logging.h"
#include "../../core/configuration/configuration_manager.h"
#include <algorithm>
#include <cmath>
#include <numeric>

// Constants for tempo detection
constexpr size_t FLUX_HISTORY_SIZE = 2048;
constexpr float MIN_TEMPO_BPM = 60.0f;
constexpr float MAX_TEMPO_BPM = 180.0f;
constexpr float ANALYSIS_INTERVAL = 2.0f; // Seconds between tempo analysis runs

SpectralFluxAutoBeatDetector::SpectralFluxAutoBeatDetector()
    : is_running_(false),      analysis_pending_(false),
      flux_threshold_(Listeningway::ConfigurationManager::Snapshot().beat.spectralFluxThreshold),
      beat_value_(0.0f),
      current_tempo_bpm_(0.0f),
      tempo_confidence_(0.0f),
      beat_phase_(0.0f),
      time_since_last_analysis_(0.0f),
      time_since_last_beat_(0.0f),
      total_time_(0.0f)
{
    result_.beat = 0.0f;
    result_.tempo_detected = false;
    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Created");
}

SpectralFluxAutoBeatDetector::~SpectralFluxAutoBeatDetector() {
    Stop();
    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Destroyed");
}

void SpectralFluxAutoBeatDetector::Start() {
    // Stop first if already running
    Stop();
    
    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Starting");
    
    // Initialize state
    {
        std::lock_guard<std::mutex> lock(mutex_);
        flux_history_.clear();
        beat_value_ = 0.0f;
        current_tempo_bpm_ = 0.0f;
        tempo_confidence_ = 0.0f;
        beat_phase_ = 0.0f;
        time_since_last_analysis_ = 0.0f;
        time_since_last_beat_ = 0.0f;
        total_time_ = 0.0f;
        result_.tempo_detected = false;
    }
    
    // Start analysis thread
    is_running_ = true;
    analysis_thread_ = std::thread(&SpectralFluxAutoBeatDetector::TempoAnalysisThread, this);
    
    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Started");
}

void SpectralFluxAutoBeatDetector::Stop() {
    if (!is_running_.load()) {
        return;
    }
    
    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Stopping");
    
    // Stop the analysis thread
    is_running_ = false;
    if (analysis_thread_.joinable()) {
        analysis_thread_.join();
    }
    
    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Stopped");
}

void SpectralFluxAutoBeatDetector::Process(const std::vector<float>& magnitudes, float flux, float flux_low, float dt) {
    if (!is_running_.load()) {
        return;
    }
    
    // Update time tracking
    total_time_ += dt;
    time_since_last_analysis_ += dt;
    
    // Store flux for analysis
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Store flux for tempo analysis
        flux_history_.push_back(flux_low);
        if (flux_history_.size() > FLUX_HISTORY_SIZE) {
            flux_history_.pop_front();
        }        // Update beat detection using low frequency flux
        // For this advanced detector, we use a dynamic threshold based on recent history
        if (flux_low > flux_threshold_ * Listeningway::ConfigurationManager::Snapshot().beat.fluxLowThresholdMultiplier) { // Thread-safe for beat detection thread
            float beat_gap = 0.0f;
            
            if (current_tempo_bpm_ > 0.0f) {
                // If we have a tempo, use it to adjust beat timing
                float expected_beat_time = 60.0f / current_tempo_bpm_;
                beat_gap = total_time_ - time_since_last_beat_;                // Only accept beats that are close to the expected timing
                // Allow more flexibility for lower confidence levels
                float window = Listeningway::ConfigurationManager::Snapshot().beat.beatInductionWindow * (1.0f + (1.0f - tempo_confidence_)); // Thread-safe for beat detection thread
                
                if (beat_gap > expected_beat_time * (1.0f - window) &&
                    beat_gap < expected_beat_time * (1.0f + window)) {
                    // This is a beat aligned with our tempo
                    beat_value_ = 1.0f;
                    time_since_last_beat_ = total_time_;
                    last_beat_time_ = std::chrono::steady_clock::now();
                    
                    // Reset phase based on actual beat time
                    beat_phase_ = 0.0f;
                    
                    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Beat aligned with tempo: " + 
                              std::to_string(current_tempo_bpm_) + " BPM");
                } 
                else if (beat_gap > expected_beat_time * 0.5f) {
                    // Not aligned exactly, but could be a genuine beat
                    // We still trigger a beat but we don't reset phase
                    beat_value_ = 1.0f;
                    time_since_last_beat_ = total_time_;
                    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Beat detected (unaligned): " +
                              std::to_string(beat_gap) + "s gap");
                }
            } else {
                // No tempo detected yet, use simple threshold approach
                beat_value_ = 1.0f;
                time_since_last_beat_ = total_time_;
                last_beat_time_ = std::chrono::steady_clock::now();
                LOG_DEBUG("[SpectralFluxAutoBeatDetector] Beat detected (no tempo)");
            }
        }
        
        // Decay beat value over time based on tempo if detected
        float decay_rate;        if (current_tempo_bpm_ > 0.0f) {
            // Adjust decay rate based on tempo        // Faster tempo = faster decay
            float beat_length = 60.0f / current_tempo_bpm_;            decay_rate = Listeningway::ConfigurationManager::Snapshot().beat.spectralFluxDecayMultiplier / beat_length; // Thread-safe for beat detection thread
        } else {
            // Default decay rate
            decay_rate = Listeningway::ConfigurationManager::Snapshot().beat.falloffDefault; // Thread-safe for beat detection thread
        }
        
        beat_value_ = std::max(0.0f, beat_value_ - decay_rate * dt);
        
        // Update result
        result_.beat = beat_value_;
        result_.tempo_bpm = current_tempo_bpm_;
        result_.confidence = tempo_confidence_;
        result_.beat_phase = beat_phase_;
        result_.tempo_detected = (current_tempo_bpm_ > 0.0f);
    }
    
    // Update beat phase based on current tempo
    UpdateBeatPhase(dt);
    
    // Trigger tempo analysis if needed
    if (time_since_last_analysis_ >= ANALYSIS_INTERVAL && !analysis_pending_.load()) {
        analysis_pending_ = true;
        time_since_last_analysis_ = 0.0f;
    }
}

BeatDetectorResult SpectralFluxAutoBeatDetector::GetResult() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return result_;
}

void SpectralFluxAutoBeatDetector::TempoAnalysisThread() {
    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Tempo analysis thread started");
    
    while (is_running_.load()) {
        // Wait for new analysis request
        if (analysis_pending_.load()) {
            LOG_DEBUG("[SpectralFluxAutoBeatDetector] Running tempo analysis");
            
            // Copy flux history for analysis
            std::vector<float> flux_copy;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                flux_copy.assign(flux_history_.begin(), flux_history_.end());
            }
            
            if (flux_copy.size() > 100) {  // Need enough data for analysis
                // Perform tempo detection
                float detected_tempo = DetectTempo(flux_copy);
                
                // Update tempo if valid
                if (detected_tempo > 0.0f) {
                    std::lock_guard<std::mutex> lock(mutex_);                    // If we already have a tempo, only change it if the new one is significantly different
                    if (current_tempo_bpm_ <= 0.0f || 
                        std::abs(current_tempo_bpm_ - detected_tempo) / current_tempo_bpm_ > Listeningway::ConfigurationManager::Snapshot().beat.tempoChangeThreshold) { // Thread-safe for beat detection thread
                        
                        LOG_DEBUG("[SpectralFluxAutoBeatDetector] Tempo changed from " + 
                                  std::to_string(current_tempo_bpm_) + " to " + 
                                  std::to_string(detected_tempo) + " BPM");
                        
                        current_tempo_bpm_ = detected_tempo;
                        // Confidence starts low and increases over time if tempo stays consistent
                        tempo_confidence_ = std::min(0.8f, tempo_confidence_ + 0.2f);
                    } else {
                        // Tempo is consistent, increase confidence
                        tempo_confidence_ = std::min(1.0f, tempo_confidence_ + 0.1f);
                    }
                }
            }
            
            analysis_pending_ = false;
        }
        
        // Sleep to avoid wasting CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    LOG_DEBUG("[SpectralFluxAutoBeatDetector] Tempo analysis thread stopped");
}

float SpectralFluxAutoBeatDetector::DetectTempo(const std::vector<float>& flux_history) {
    const auto config = Listeningway::ConfigurationManager::Snapshot(); // Thread-safe snapshot for tempo analysis
    
    if (flux_history.size() < 100) {
        return 0.0f;
    }
    
    // Normalize the flux data
    std::vector<float> normalized_flux(flux_history.size());
    float max_flux = *std::max_element(flux_history.begin(), flux_history.end());
    if (max_flux > 0.0f) {
        for (size_t i = 0; i < flux_history.size(); i++) {
            normalized_flux[i] = flux_history[i] / max_flux;
        }
    } else {
        return 0.0f; // No significant flux, can't detect tempo
    }    // Apply a threshold to get a binary array of beats
    std::vector<float> beat_array(normalized_flux.size(), 0.0f);
    for (size_t i = 0; i < normalized_flux.size(); i++) {
        if (normalized_flux[i] > Listeningway::ConfigurationManager::Snapshot().beat.spectralFluxThreshold) { // Thread-safe for tempo analysis thread
            beat_array[i] = 1.0f;
        }
    }
    
    // Calculate autocorrelation to find periodicity
    std::vector<float> autocorr(beat_array.size() / 2);
    for (size_t lag = 0; lag < autocorr.size(); lag++) {
        float sum = 0.0f;
        for (size_t i = 0; i < beat_array.size() - lag; i++) {
            sum += beat_array[i] * beat_array[i + lag];
        }
        autocorr[lag] = sum / (beat_array.size() - lag);
    }
    
    // Find peaks in autocorrelation
    std::vector<size_t> peaks;
    for (size_t i = 2; i < autocorr.size() - 2; i++) {
        if (autocorr[i] > autocorr[i-1] && autocorr[i] > autocorr[i-2] &&
            autocorr[i] > autocorr[i+1] && autocorr[i] > autocorr[i+2] &&
            autocorr[i] > 0.1f) {  // Threshold to avoid noise
            peaks.push_back(i);
        }
    }
    
    if (peaks.empty()) {
        return 0.0f;  // No clear periodicity
    }
    
    // Convert peaks to BPM (assuming 43.1 Hz sampling rate - 1024 samples @ 44.1kHz)
    const float SAMPLE_RATE = 43.1f;
    const float SECONDS_PER_SAMPLE = 1.0f / SAMPLE_RATE;
    
    std::vector<float> peak_bpms;
    for (size_t peak : peaks) {
        float period_seconds = peak * SECONDS_PER_SAMPLE;
        float bpm = 60.0f / period_seconds;
        
        // Only consider peaks in our target BPM range
        if (bpm >= MIN_TEMPO_BPM && bpm <= MAX_TEMPO_BPM) {
            peak_bpms.push_back(bpm);
        }
    }
    
    if (peak_bpms.empty()) {
        return 0.0f;  // No peaks in valid BPM range
    }
    
    // Sort peaks by autocorrelation strength
    std::sort(peak_bpms.begin(), peak_bpms.end(), 
              [&](float a, float b) {
                  size_t idx_a = static_cast<size_t>(60.0f / a / SECONDS_PER_SAMPLE);
                  size_t idx_b = static_cast<size_t>(60.0f / b / SECONDS_PER_SAMPLE);
                  return autocorr[idx_a] > autocorr[idx_b];
              });
    
    // Account for octave errors (half/double tempo)
    // Check if we have a strong peak at double/half the tempo
    float primary_bpm = peak_bpms[0];
    float half_bpm = primary_bpm / 2.0f;
    float double_bpm = primary_bpm * 2.0f;
    
    // Check if half or double is in our valid range
    if (half_bpm >= MIN_TEMPO_BPM) {
        for (float bpm : peak_bpms) {
            if (std::abs(bpm - half_bpm) < 2.0f) {
                // We found a peak at half tempo                // Weight the decision
                size_t primary_idx = static_cast<size_t>(60.0f / primary_bpm / SECONDS_PER_SAMPLE);
                size_t half_idx = static_cast<size_t>(60.0f / half_bpm / SECONDS_PER_SAMPLE);
                
                if (autocorr[half_idx] > autocorr[primary_idx] * config.beat.octaveErrorWeight) {
                    // Half tempo is significantly stronger
                    primary_bpm = half_bpm;
                }
                break;
            }
        }
    }
    
    if (double_bpm <= MAX_TEMPO_BPM) {
        for (float bpm : peak_bpms) {
            if (std::abs(bpm - double_bpm) < 4.0f) {
                // We found a peak at double tempo                // Weight the decision
                size_t primary_idx = static_cast<size_t>(60.0f / primary_bpm / SECONDS_PER_SAMPLE);
                size_t double_idx = static_cast<size_t>(60.0f / double_bpm / SECONDS_PER_SAMPLE);
                
                if (autocorr[double_idx] > autocorr[primary_idx] * config.beat.octaveErrorWeight) {
                    // Double tempo is significantly stronger
                    primary_bpm = double_bpm;
                }
                break;
            }
        }
    }
    
    return primary_bpm;
}

void SpectralFluxAutoBeatDetector::UpdateBeatPhase(float dt) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_tempo_bpm_ <= 0.0f) {
        // No tempo detected, no phase to update
        beat_phase_ = 0.0f;
        return;
    }
    
    // Calculate phase based on time since last beat reset
    float beat_period = 60.0f / current_tempo_bpm_;
    beat_phase_ += dt / beat_period;
    
    // Keep phase in [0, 1] range
    while (beat_phase_ >= 1.0f) {
        beat_phase_ -= 1.0f;
    }
}