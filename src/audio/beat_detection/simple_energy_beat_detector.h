// ---------------------------------------------
// Simple Energy Beat Detector
// Uses basic energy threshold algorithm for beat detection
// ---------------------------------------------
#pragma once
#include "beat_detector.h"
#include "settings.h"
#include <mutex>
#include <vector>
#include <chrono>

/**
 * @brief Simple energy-based beat detector implementation
 * 
 * This detector uses the original algorithm from Listeningway which is based
 * on detecting flux spikes above a certain threshold.
 */
class SimpleEnergyBeatDetector : public IBeatDetector {
public:
    /**
     * @brief Constructor
     */
    SimpleEnergyBeatDetector();
    
    /**
     * @brief Destructor
     */
    ~SimpleEnergyBeatDetector() override;
    
    /**
     * @brief Start processing
     */
    void Start() override;
    
    /**
     * @brief Stop processing
     */
    void Stop() override;
    
    /**
     * @brief Process audio data for beat detection
     * @param magnitudes FFT magnitudes
     * @param flux Spectral flux value
     * @param flux_low Low-frequency band limited flux
     * @param dt Time delta since last frame
     */
    void Process(const std::vector<float>& magnitudes, float flux, float flux_low, float dt) override;
    
    /**
     * @brief Get the current beat detection result
     * @return The current beat detection result
     */
    BeatDetectorResult GetResult() const override;
    
private:
    mutable std::mutex mutex_;
    BeatDetectorResult result_;
    bool is_running_ = false;
    
    // Beat tracking variables
    float last_beat_time_ = 0.0f;
    float total_time_ = 0.0f;
    float beat_value_ = 0.0f;
    std::chrono::steady_clock::time_point last_beat_timestamp_;
    
    // Beat detection parameters
    float flux_threshold_ = 0.0f;
    float beat_falloff_ = 0.0f;
};