// ---------------------------------------------
// Spectral Flux Auto Beat Detector
// Uses spectral flux analysis and autocorrelation for tempo detection
// ---------------------------------------------
#pragma once
#include "beat_detector.h"
#include "../../core/configuration/configuration_manager.h"
#include <mutex>
#include <vector>
#include <deque>
#include <thread>
#include <atomic>
#include <chrono>

/**
 * @brief Advanced beat detector using spectral flux and autocorrelation
 * 
 * This detector improves upon the SimpleEnergyBeatDetector by:
 * 1. Using autocorrelation to detect the tempo of the music
 * 2. Adjusting beat timing based on detected tempo
 * 3. Providing a beat phase based on detected tempo
 */
class SpectralFluxAutoBeatDetector : public IBeatDetector {
public:
    /**
     * @brief Constructor
     */
    SpectralFluxAutoBeatDetector();
    
    /**
     * @brief Destructor
     */
    ~SpectralFluxAutoBeatDetector() override;
    
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
    /**
     * @brief Analyze collected flux data to detect tempo
     * Runs in a separate thread to avoid blocking audio processing
     */
    void TempoAnalysisThread();
    
    /**
     * @brief Perform autocorrelation to find the tempo
     * @param flux_history Vector of spectral flux values
     * @return Detected tempo in BPM
     */
    float DetectTempo(const std::vector<float>& flux_history);
    
    /**
     * @brief Update the beat phase based on current time and detected tempo
     * @param dt Time delta since last frame
     */
    void UpdateBeatPhase(float dt);
    
    mutable std::mutex mutex_;
    BeatDetectorResult result_;
    
    // Thread control variables
    std::atomic_bool is_running_{false};
    std::atomic_bool analysis_pending_{false};
    std::thread analysis_thread_;
    
    // Beat detection state
    std::deque<float> flux_history_;
    float flux_threshold_ = 0.0f;
    float beat_value_ = 0.0f;
    
    // Tempo tracking state
    float current_tempo_bpm_ = 0.0f;
    float tempo_confidence_ = 0.0f;
    float beat_phase_ = 0.0f;
    float time_since_last_analysis_ = 0.0f;
    float time_since_last_beat_ = 0.0f;
    float total_time_ = 0.0f;
    
    // Timestamps for timing measurement
    std::chrono::steady_clock::time_point last_beat_time_;
};