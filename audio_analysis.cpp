// ---------------------------------------------
// Audio Analysis Module Implementation
// Performs real-time audio feature extraction for Listeningway
// ---------------------------------------------
#include "audio_analysis.h"
#include "constants.h"
#include "settings.h"
#include <kiss_fftr.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <chrono>

// Define M_PI if not already defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    for (size_t i = 0; i < fftSize; ++i) {
        // Apply Hann window to input sample to reduce spectral leakage
        float hann_multiplier = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (fftSize - 1)));
        fft_in[i] = monoBuffer[i] * hann_multiplier;
    }
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
    // Use configurable frequency range for beat detection from settings
    float min_beat_freq = g_settings.beat_min_freq;
    float max_beat_freq = g_settings.beat_max_freq;
    
    // Calculate sample rate from FFT size - assuming 44.1kHz if not available
    float sample_rate = 44100.0f; // Default assumption
    float freq_resolution = sample_rate / (float)config.fft_size;
    
    // Calculate bin indices for beat frequency range
    size_t bin_low = (size_t)(min_beat_freq / freq_resolution);
    size_t bin_high = (size_t)(max_beat_freq / freq_resolution);
    
    // Clamp indices to valid range
    bin_high = std::min(bin_high, magnitudes.size() - 1);
    
    float flux_low = 0.0f; // Band-limited flux for beat detection
    
    if (!out._prev_magnitudes.empty() && out._prev_magnitudes.size() == magnitudes.size()) {
        // Calculate full-spectrum flux (for backward compatibility)
        for (size_t i = 0; i < magnitudes.size(); ++i) {
            float diff = magnitudes[i] - out._prev_magnitudes[i];
            if (diff > 0) flux += diff;
        }
        
        // Calculate band-limited flux (low frequencies)
        for (size_t i = bin_low; i <= bin_high; ++i) {
            float diff = magnitudes[i] - out._prev_magnitudes[i];
            if (diff > 0) flux_low += diff;
        }
    }
    
    // Update moving averages for thresholds
    // Use separate smoothing factors for full-spectrum and band-limited flux
    const float flux_alpha = g_settings.flux_alpha;
    const float flux_low_alpha = g_settings.flux_low_alpha;
    
    // Full-spectrum flux (kept for backward compatibility)
    if (out._flux_avg == 0.0f) out._flux_avg = flux;
    else out._flux_avg = (1.0f - flux_alpha) * out._flux_avg + flux_alpha * flux;
    
    // Band-limited flux with its own smoothing factor
    if (out._flux_low_avg == 0.0f) out._flux_low_avg = flux_low;
    else out._flux_low_avg = (1.0f - flux_low_alpha) * out._flux_low_avg + flux_low_alpha * flux_low;
    
    // Dynamic threshold based on band-limited flux with its own threshold multiplier
    float threshold = out._flux_low_avg * g_settings.flux_low_threshold_multiplier;

    // --- 6. Adaptive beat fade-out ---
    using clock = std::chrono::steady_clock;
    static auto last_call = clock::now();
    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last_call).count();
    last_call = now;

    // Detect beat (if flux exceeds threshold)
    bool is_beat = (flux_low > threshold) && (flux_low > g_settings.beat_flux_min);
    if (is_beat) {
        // Set falloff rate based on time since last beat
        float time_since_last = (out._last_beat_time > 0.0f) ? (now.time_since_epoch().count() - out._last_beat_time) * g_settings.beat_time_scale : g_settings.beat_time_initial;
        out._falloff_rate = (time_since_last > g_settings.beat_time_min) ? (1.0f / std::max(g_settings.beat_time_divisor, time_since_last)) : g_settings.beat_falloff_default;
        out.beat = 1.0f;
        out._last_beat_time = now.time_since_epoch().count();
    } else {
        // Fade out beat value
        out.beat -= out._falloff_rate * dt;
        if (out.beat < 0.0f) out.beat = 0.0f;
    }
    out._prev_magnitudes = magnitudes;

    // --- 7. Map FFT bins to frequency bands (log or linear) ---
    out.freq_bands.resize(config.num_bands, 0.0f);
    if (g_settings.band_log_scale) {
        // Logarithmic mapping with tunable strength
        float min_freq = std::max(1.0f, g_settings.band_min_freq);
        float max_freq = std::min((float)config.fft_size / 2.0f, g_settings.band_max_freq);
        float nyquist = (float)config.fft_size / 2.0f;
        size_t num_bins = magnitudes.size();
        float log_strength = std::max(0.01f, g_settings.band_log_strength); // Prevent zero or negative
        for (size_t band = 0; band < config.num_bands; ++band) {
            // Apply log_strength to band fraction
            float band_frac_low = powf((float)band / config.num_bands, log_strength);
            float band_frac_high = powf((float)(band + 1) / config.num_bands, log_strength);
            float freq_low = min_freq * powf(max_freq / min_freq, band_frac_low);
            float freq_high = min_freq * powf(max_freq / min_freq, band_frac_high);
            // Convert frequency to bin indices
            size_t bin_low = (size_t)std::floor(freq_low / max_freq * (num_bins - 1));
            size_t bin_high = (size_t)std::ceil(freq_high / max_freq * (num_bins - 1));
            bin_high = std::min(bin_high, num_bins - 1);
            float sum = 0.0f;
            size_t count = 0;
            for (size_t j = bin_low; j <= bin_high; ++j) {
                sum += magnitudes[j];
                ++count;
            }
            out.freq_bands[band] = (count > 0) ? sum / count : 0.0f;
        }
    } else {
        // Linear mapping (legacy)
        size_t binsPerBand = magnitudes.size() / config.num_bands;
        for (size_t band = 0; band < config.num_bands; ++band) {
            float sum = 0.0f;
            size_t start = band * binsPerBand;
            size_t end = (band + 1) * binsPerBand;
            if (band == config.num_bands - 1) end = magnitudes.size();
            for (size_t j = start; j < end; ++j) sum += magnitudes[j];
            out.freq_bands[band] = (end > start) ? sum / (end - start) : 0.0f;
        }
    }

    // --- 8. Normalize volume and bands ---
    out.volume = std::min(rms * g_settings.volume_norm, 1.0f);
    for (auto& b : out.freq_bands) b = std::min(b * g_settings.band_norm, 1.0f);
}
