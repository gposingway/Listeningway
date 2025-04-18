// ---------------------------------------------
// Audio Analysis Module
// Provides real-time audio feature extraction for Listeningway
// ---------------------------------------------
#pragma once
#include <vector>
#include <cstddef>
#include "constants.h"

// Holds the results of audio analysis for one frame
struct AudioAnalysisData {
    float volume = 0.0f;                // Normalized RMS volume [0,1]
    std::vector<float> freq_bands;      // Normalized frequency band magnitudes
    float beat = 0.0f;                 // Beat detection value [0,1]

    // --- Internal state for analysis (not for API consumers) ---
    std::vector<float> _prev_magnitudes; // Previous FFT magnitudes (for spectral flux)
    float _flux_avg = 0.0f;             // Moving average of spectral flux
    float _last_beat_time = 0.0f;       // Last beat timestamp (for adaptive falloff)
    float _falloff_rate = 1.0f;         // Adaptive beat falloff rate

    AudioAnalysisData(size_t bands = LISTENINGWAY_NUM_BANDS) : freq_bands(bands, 0.0f) {}
};

// Configuration for audio analysis
struct AudioAnalysisConfig {
    size_t num_bands = LISTENINGWAY_NUM_BANDS; // Number of frequency bands
    size_t fft_size = LISTENINGWAY_FFT_SIZE;   // FFT window size
};

// Main entry point: analyze a buffer of audio samples and update analysis data
//   - data: pointer to interleaved float samples
//   - numFrames: number of frames (samples per channel)
//   - numChannels: number of channels (e.g. 2 for stereo)
//   - config: analysis configuration
//   - out: analysis results (updated in-place)
void AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, const AudioAnalysisConfig& config, AudioAnalysisData& out);
