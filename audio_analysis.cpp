// ---------------------------------------------
// Audio Analysis Module Implementation
// Performs real-time audio feature extraction for Listeningway
// ---------------------------------------------
#include "audio_analysis.h"
#include "constants.h"
#include <kiss_fftr.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <chrono>

// Analyze a buffer of audio samples and update the analysis data structure.
// This function extracts volume, frequency bands, and beat information.
void AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, const AudioAnalysisConfig& config, AudioAnalysisData& out) {
    // --- 1. Calculate RMS volume ---
    double sumSquares = 0.0;
    size_t numSamples = numFrames * numChannels;
    for (size_t i = 0; i < numSamples; ++i)
        sumSquares += data[i] * data[i];
    float rms = numSamples > 0 ? static_cast<float>(sqrt(sumSquares / numSamples)) : 0.0f;

    // --- 2. Prepare mono buffer for FFT (use only the first channel) ---
    size_t fftSize = config.fft_size;
    std::vector<float> monoBuffer(fftSize, 0.0f);
    size_t copySamples = std::min(numFrames, fftSize);
    for (size_t i = 0; i < copySamples; ++i)
        monoBuffer[i] = data[i * numChannels];

    // --- 3. Perform FFT ---
    std::vector<kiss_fft_scalar> fft_in(fftSize, 0.0f);
    std::vector<kiss_fft_cpx> fft_out(fftSize / 2 + 1);
    for (size_t i = 0; i < fftSize; ++i)
        fft_in[i] = monoBuffer[i];
    kiss_fftr_cfg cfg = kiss_fftr_alloc((int)fftSize, 0, nullptr, nullptr);
    if (cfg) {
        kiss_fftr(cfg, fft_in.data(), fft_out.data());
        free(cfg);
    }

    // --- 4. Calculate magnitude for each FFT bin ---
    std::vector<float> magnitudes(fftSize / 2 + 1, 0.0f);
    for (size_t i = 0; i < magnitudes.size(); ++i)
        magnitudes[i] = sqrtf(fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i);

    // --- 5. Spectral Flux Beat Detection ---
    float flux = 0.0f;
    if (!out._prev_magnitudes.empty() && out._prev_magnitudes.size() == magnitudes.size()) {
        for (size_t i = 0; i < magnitudes.size(); ++i) {
            float diff = magnitudes[i] - out._prev_magnitudes[i];
            if (diff > 0) flux += diff;
        }
    }
    // Update moving average for threshold
    const float flux_alpha = LISTENINGWAY_FLUX_ALPHA; // Smoothing factor for moving average
    if (out._flux_avg == 0.0f) out._flux_avg = flux;
    else out._flux_avg = (1.0f - flux_alpha) * out._flux_avg + flux_alpha * flux;
    float threshold = out._flux_avg * LISTENINGWAY_FLUX_THRESHOLD_MULTIPLIER; // Dynamic threshold

    // --- 6. Adaptive beat fade-out ---
    using clock = std::chrono::steady_clock;
    static auto last_call = clock::now();
    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last_call).count();
    last_call = now;

    // Detect beat (if flux exceeds threshold)
    bool is_beat = (flux > threshold) && (flux > LISTENINGWAY_BEAT_FLUX_MIN);
    if (is_beat) {
        // Set falloff rate based on time since last beat
        float time_since_last = (out._last_beat_time > 0.0f) ? (now.time_since_epoch().count() - out._last_beat_time) * LISTENINGWAY_BEAT_TIME_SCALE : LISTENINGWAY_BEAT_TIME_INITIAL;
        out._falloff_rate = (time_since_last > LISTENINGWAY_BEAT_TIME_MIN) ? (1.0f / std::max(LISTENINGWAY_BEAT_TIME_DIVISOR, time_since_last)) : LISTENINGWAY_BEAT_FALLOFF_DEFAULT;
        out.beat = 1.0f;
        out._last_beat_time = now.time_since_epoch().count();
    } else {
        // Fade out beat value
        out.beat -= out._falloff_rate * dt;
        if (out.beat < 0.0f) out.beat = 0.0f;
    }
    out._prev_magnitudes = magnitudes;

    // --- 7. Map FFT bins to frequency bands ---
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

    // --- 8. Normalize volume and bands ---
    out.volume = std::min(rms * LISTENINGWAY_VOLUME_NORM, 1.0f);
    for (auto& b : out.freq_bands) b = std::min(b * LISTENINGWAY_BAND_NORM, 1.0f);
}
