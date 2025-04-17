#include "audio_analysis.h"
#include <kiss_fftr.h>
#include <cmath>
#include <vector>

void AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, const AudioAnalysisConfig& config, AudioAnalysisData& out) {
    // Calculate RMS volume
    double sumSquares = 0.0;
    size_t numSamples = numFrames * numChannels;
    for (size_t i = 0; i < numSamples; ++i) sumSquares += data[i] * data[i];
    float rms = numSamples > 0 ? static_cast<float>(sqrt(sumSquares / numSamples)) : 0.0f;
    // Prepare FFT (use only the first channel for analysis)
    size_t fftSize = config.fft_size;
    std::vector<float> monoBuffer(fftSize, 0.0f);
    size_t copySamples = std::min(numFrames, fftSize);
    for (size_t i = 0; i < copySamples; ++i) monoBuffer[i] = data[i * numChannels];
    // Perform FFT
    std::vector<kiss_fft_scalar> fft_in(fftSize, 0.0f);
    std::vector<kiss_fft_cpx> fft_out(fftSize / 2 + 1);
    for (size_t i = 0; i < fftSize; ++i) fft_in[i] = monoBuffer[i];
    kiss_fftr_cfg cfg = kiss_fftr_alloc((int)fftSize, 0, nullptr, nullptr);
    if (cfg) {
        kiss_fftr(cfg, fft_in.data(), fft_out.data());
        free(cfg);
    }
    // Calculate magnitude for each FFT bin
    std::vector<float> magnitudes(fftSize / 2 + 1, 0.0f);
    for (size_t i = 0; i < magnitudes.size(); ++i)
        magnitudes[i] = sqrtf(fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i);
    // Map FFT bins to frequency bands
    out.freq_bands.resize(config.num_bands, 0.0f);
    size_t binsPerBand = magnitudes.size() / config.num_bands;
    for (size_t band = 0; band < config.num_bands; ++band) {
        float sum = 0.0f;
        size_t start = band * binsPerBand;
        size_t end = (band + 1) * binsPerBand;
        if (band == config.num_bands - 1) end = magnitudes.size();
        for (size_t j = start; j < end; ++j) sum += magnitudes[j];
        out.freq_bands[band] = (end > start) ? sum / (end - start) : 0.0f;
    }
    // Normalize volume and bands (optional)
    out.volume = std::min(rms * 2.0f, 1.0f);
    for (auto& b : out.freq_bands) b = std::min(b * 0.1f, 1.0f);
}
