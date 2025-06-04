#pragma once
#include <vector>
#include <memory>
#include <string>
#include <cstddef>

// Beat detection result data
struct BeatDetectorResult {
    float beat = 0.0f;                  // Beat detection value [0,1]
    float tempo_bpm = 0.0f;             // Detected tempo in BPM
    float confidence = 0.0f;            // Confidence in tempo estimate [0,1]
    float beat_phase = 0.0f;            // Current phase in beat cycle [0,1]
    bool tempo_detected = false;        // Whether tempo has been detected
};

// Interface for beat detection algorithms
class IBeatDetector {
public:
    virtual ~IBeatDetector() = default;
    
    virtual void Start() = 0;
    virtual void Stop() = 0;
    
    // Process audio data and update beat detection
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