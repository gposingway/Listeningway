#pragma once
#include <vector>
#include <cstddef>

constexpr size_t LISTENINGWAY_NUM_BANDS = 8;

struct AudioAnalysisData {
    float volume = 0.0f;
    std::vector<float> freq_bands;
    float beat = 0.0f; // New: Beat detection value (0.0-1.0)
    std::vector<float> _prev_magnitudes; // Internal: for spectral flux
    float _flux_avg = 0.0f; // Internal: moving average for threshold
    float _last_beat_time = 0.0f; // Internal: for adaptive falloff
    float _falloff_rate = 1.0f; // Internal: adaptive falloff
    AudioAnalysisData(size_t bands = LISTENINGWAY_NUM_BANDS) : freq_bands(bands, 0.0f) {}
};

struct AudioAnalysisConfig {
    size_t num_bands = LISTENINGWAY_NUM_BANDS;
    size_t fft_size = 512;
};

void AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, const AudioAnalysisConfig& config, AudioAnalysisData& out);
