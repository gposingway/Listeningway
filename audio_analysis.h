#pragma once
#include <vector>
#include <cstddef>

constexpr size_t LISTENINGWAY_NUM_BANDS = 8;

struct AudioAnalysisData {
    float volume = 0.0f;
    std::vector<float> freq_bands;
    AudioAnalysisData(size_t bands = LISTENINGWAY_NUM_BANDS) : freq_bands(bands, 0.0f) {}
};

struct AudioAnalysisConfig {
    size_t num_bands = LISTENINGWAY_NUM_BANDS;
    size_t fft_size = 512;
};

void AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, const AudioAnalysisConfig& config, AudioAnalysisData& out);
