// ---------------------------------------------
// Beat Detector Interface
// Provides an abstraction for different beat detection algorithms
// ---------------------------------------------
#pragma once
#include <vector>
#include <memory>
#include <string>
#include <cstddef>

/**
 * @brief Result data from beat detection algorithms
 */
struct BeatDetectorResult {
    float beat = 0.0f;                  // Beat detection value [0,1]
    float tempo_bpm = 0.0f;             // Detected tempo in BPM (if available)
    float confidence = 0.0f;            // Confidence in tempo estimate [0,1]
    float beat_phase = 0.0f;            // Current phase in beat cycle [0,1]
    bool tempo_detected = false;        // Whether tempo has been detected
};

/**
 * @brief Interface for beat detectors
 */
class IBeatDetector {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IBeatDetector() = default;
    
    /**
     * @brief Start the beat detector processing
     */
    virtual void Start() = 0;
    
    /**
     * @brief Stop the beat detector processing
     */
    virtual void Stop() = 0;
    
    /**
     * @brief Process audio data and update beat detection
     * @param magnitudes FFT magnitudes
     * @param flux Spectral flux value
     * @param flux_low Low-frequency band limited flux
     * @param dt Time delta since last frame
     */
    virtual void Process(const std::vector<float>& magnitudes, float flux, float flux_low, float dt) = 0;
    
    /**
     * @brief Get the current beat detection result
     * @return The current beat detection result
     */
    virtual BeatDetectorResult GetResult() const = 0;
    
    /**
     * @brief Factory method to create a beat detector
     * @param algorithm Algorithm index (0=SimpleEnergy, 1=SpectralFluxAuto)
     * @return A new beat detector instance
     */
    static std::unique_ptr<IBeatDetector> Create(int algorithm);
};