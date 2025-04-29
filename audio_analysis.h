// ---------------------------------------------
// Audio Analysis Module
// Provides real-time audio feature extraction for Listeningway
// ---------------------------------------------
#pragma once
#include <vector>
#include <cstddef>
#include <deque>
#include "constants.h"
#include "settings.h"

// Define constants for beat detection
constexpr size_t SPECTRAL_FLUX_BUFFER_SIZE = 1024; // Buffer size for ODF history
constexpr float MIN_BPM = 40.0f;      // Minimum tempo in BPM
constexpr float MAX_BPM = 240.0f;     // Maximum tempo in BPM

/**
 * @brief Beat detection algorithms available in Listeningway
 */
enum class BeatDetectionAlgorithm {
    SimpleEnergy,           // Original simple energy-based detection
    SpectralFluxAuto        // Advanced spectral flux with autocorrelation
};

/**
 * @brief Holds the results of audio analysis for one frame.
 * Contains normalized volume, frequency bands, and beat value.
 */
struct AudioAnalysisData {
    float volume = 0.0f;                // Normalized RMS volume [0,1]
    std::vector<float> freq_bands;      // Normalized frequency band magnitudes
    float beat = 0.0f;                 // Beat detection value [0,1]

    // --- Internal state for analysis (not for API consumers) ---
    std::vector<float> _prev_magnitudes; // Previous FFT magnitudes (for spectral flux)
    float _flux_avg = 0.0f;             // Moving average of spectral flux
    float _flux_low_avg = 0.0f;         // Moving average of low-frequency spectral flux
    float _last_beat_time = 0.0f;       // Last beat timestamp (for adaptive falloff)
    float _falloff_rate = 1.0f;         // Adaptive beat falloff rate
    
    // --- Additional state for Spectral Flux + Autocorrelation ---
    std::deque<float> _spectral_flux_buffer;   // Buffer of spectral flux values
    float _current_tempo_bpm = 120.0f;         // Current detected tempo in BPM
    float _tempo_confidence = 0.0f;            // Confidence in tempo estimate [0,1]
    float _beat_phase = 0.0f;                  // Current phase in beat cycle [0,1]
    float _last_frame_time = 0.0f;             // Time of last frame for phase tracking
    bool _autocorr_initialized = false;        // Flag for initialization of autocorrelation

    AudioAnalysisData(size_t bands = 8) : freq_bands(bands, 0.0f) {
        _spectral_flux_buffer.resize(SPECTRAL_FLUX_BUFFER_SIZE, 0.0f);
    }
};

/**
 * @brief Configuration for audio analysis (FFT size, bands, etc.).
 */
struct AudioAnalysisConfig {
    size_t num_bands;
    size_t fft_size;
    BeatDetectionAlgorithm beat_algorithm;
    float sample_rate;

    AudioAnalysisConfig(const ListeningwaySettings& settings)
        : num_bands(settings.num_bands), 
          fft_size(settings.fft_size),
          beat_algorithm(static_cast<BeatDetectionAlgorithm>(settings.beat_detection_algorithm)),
          sample_rate(44100.0f) {}  // Assuming 44.1kHz by default
};

/**
 * @brief Analyze a buffer of audio samples and update analysis data.
 * @param data Pointer to interleaved float samples.
 * @param numFrames Number of frames (samples per channel).
 * @param numChannels Number of channels (e.g. 2 for stereo).
 * @param config Analysis configuration.
 * @param out Analysis results (updated in-place).
 */
void AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, const AudioAnalysisConfig& config, AudioAnalysisData& out);

/**
 * @brief Detects beats using the simple energy method (original algorithm)
 * @param magnitudes FFT magnitudes
 * @param config Analysis configuration
 * @param out Analysis results (updated in-place)
 * @param flux_low Low-frequency band limited flux
 * @param dt Time delta since last frame
 */
void DetectBeatsSimpleEnergy(const std::vector<float>& magnitudes, const AudioAnalysisConfig& config, 
                            AudioAnalysisData& out, float flux_low, float dt);

/**
 * @brief Detects beats using spectral flux + autocorrelation
 * @param magnitudes FFT magnitudes
 * @param config Analysis configuration
 * @param out Analysis results (updated in-place)
 * @param flux Spectral flux value
 * @param dt Time delta since last frame
 */
void DetectBeatsSpectralFluxAuto(const std::vector<float>& magnitudes, const AudioAnalysisConfig& config, 
                               AudioAnalysisData& out, float flux, float dt);

/**
 * @brief Computes autocorrelation of the spectral flux buffer for tempo detection
 * @param flux_buffer Buffer of spectral flux values
 * @param sample_rate Audio sample rate
 * @param hop_size Hop size between FFT frames
 * @return Pair of (tempo BPM, confidence)
 */
std::pair<float, float> ComputeTempoFromAutocorrelation(const std::deque<float>& flux_buffer, 
                                                       float sample_rate, size_t hop_size);
