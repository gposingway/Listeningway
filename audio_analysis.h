// ---------------------------------------------
// Audio Analysis Module
// Provides real-time audio feature extraction for Listeningway
// ---------------------------------------------
#pragma once
#include <vector>
#include <cstddef>
#include "constants.h"
#include "settings.h"

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

    AudioAnalysisData(size_t bands = 8) : freq_bands(bands, 0.0f) {}
};

/**
 * @brief Configuration for audio analysis (FFT size, bands, etc.).
 */
struct AudioAnalysisConfig {
    size_t num_bands;
    size_t fft_size;

    AudioAnalysisConfig(const ListeningwaySettings& settings)
        : num_bands(settings.num_bands), fft_size(settings.fft_size) {}
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
