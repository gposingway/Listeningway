// ---------------------------------------------
// Audio Analysis Module
// Provides real-time audio feature extraction for Listeningway
// ---------------------------------------------
#pragma once
#include <vector>
#include <cstddef>
#include <memory>
#include <mutex>
#include "constants.h"
#include "settings.h"
#include "beat_detector.h"

/**
 * @brief Holds the results of audio analysis for one frame.
 * Contains normalized volume, frequency bands, and beat value.
 */
struct AudioAnalysisData {
    float volume = 0.0f;                // Normalized RMS volume [0,1]
    std::vector<float> freq_bands;      // Normalized frequency band magnitudes (with equalizer applied)
    std::vector<float> raw_freq_bands;  // Raw frequency band values (without equalizer)
    float beat = 0.0f;                 // Beat detection value [0,1]
    
    // Advanced info from beat detection
    float tempo_bpm = 0.0f;            // Detected tempo in BPM (if available)
    float tempo_confidence = 0.0f;     // Confidence in tempo estimate [0,1]
    float beat_phase = 0.0f;           // Current phase in beat cycle [0,1]
    bool tempo_detected = false;       // Whether tempo has been detected

    // --- Internal state for analysis (not for API consumers) ---
    std::vector<float> _prev_magnitudes; // Previous FFT magnitudes (for spectral flux)
    float _flux_avg = 0.0f;             // Moving average of spectral flux
    float _flux_low_avg = 0.0f;         // Moving average of low-frequency spectral flux

    // Additional fields for stereo analysis
    float volume_left = 0.0f;         // Normalized RMS/peak volume for left channel(s)
    float volume_right = 0.0f;        // Normalized RMS/peak volume for right channel(s)
    float audio_pan = 0.0f;           // Calculated pan angle in degrees (-180 to +180)

    AudioAnalysisData(size_t bands = 8) : freq_bands(bands, 0.0f), raw_freq_bands(bands, 0.0f) {}
};

/**
 * @brief Configuration for audio analysis (FFT size, bands, etc.).
 */
struct AudioAnalysisConfig {
    size_t num_bands;
    size_t fft_size;
    int beat_algorithm;
    float sample_rate;

    AudioAnalysisConfig(const ListeningwaySettings& settings)
        : num_bands(settings.num_bands), 
          fft_size(settings.fft_size),
          beat_algorithm(settings.beat_detection_algorithm),
          sample_rate(44100.0f) {}  // Assuming 44.1kHz by default
};

/**
 * @brief Analyzer for audio data, using beat detection algorithms
 */
class AudioAnalyzer {
public:
    /**
     * @brief Constructor
     */
    AudioAnalyzer();
    
    /**
     * @brief Destructor
     */
    ~AudioAnalyzer();
    
    /**
     * @brief Set the beat detection algorithm to use
     * @param algorithm Algorithm index (0=SimpleEnergy, 1=SpectralFluxAuto)
     */
    void SetBeatDetectionAlgorithm(int algorithm);
    
    /**
     * @brief Get the current beat detection algorithm
     * @return Current algorithm index
     */
    int GetBeatDetectionAlgorithm() const;
    
    /**
     * @brief Start audio analysis
     */
    void Start();
    
    /**
     * @brief Stop audio analysis
     */
    void Stop();
    
    /**
     * @brief Analyze a buffer of audio samples and update analysis data.
     * @param data Pointer to interleaved float samples.
     * @param numFrames Number of frames (samples per channel).
     * @param numChannels Number of channels (e.g. 2 for stereo).
     * @param config Analysis configuration.
     * @param out Analysis results (updated in-place).
     */
    void AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, 
                           const AudioAnalysisConfig& config, AudioAnalysisData& out);

private:
    mutable std::mutex mutex_;
    std::unique_ptr<IBeatDetector> beat_detector_;
    int current_algorithm_ = 0;
    bool is_running_ = false;
};

// Global instance of the audio analyzer (accessible to all modules)
extern AudioAnalyzer g_audio_analyzer;
