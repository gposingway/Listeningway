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
#include <numeric>

// Define M_PI if not already defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Detects beats using the simple energy method (original algorithm)
 */
void DetectBeatsSimpleEnergy(const std::vector<float>& magnitudes, const AudioAnalysisConfig& config, 
                           AudioAnalysisData& out, float flux_low, float dt) {
    using clock = std::chrono::steady_clock;
    static auto last_call = clock::now();
    auto now = clock::now();
    
    // Calculate threshold
    float threshold = out._flux_low_avg * g_settings.flux_low_threshold_multiplier;
    
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
}

/**
 * Computes autocorrelation of the spectral flux buffer for tempo detection
 */
std::pair<float, float> ComputeTempoFromAutocorrelation(const std::deque<float>& flux_buffer, 
                                                      float sample_rate, size_t hop_size) {
    // We need a substantial buffer for meaningful autocorrelation
    if (flux_buffer.size() < 256) {
        return {120.0f, 0.0f}; // Default 120 BPM with zero confidence
    }
    
    // Create a vector from the deque for easier computation
    std::vector<float> buffer(flux_buffer.begin(), flux_buffer.end());
    
    // Calculate frame rate of our ODF (depends on hop size)
    float frame_rate = sample_rate / hop_size;
    
    // Convert BPM range to lag range in frames
    size_t max_lag = static_cast<size_t>(60.0f * frame_rate / MIN_BPM);
    size_t min_lag = static_cast<size_t>(60.0f * frame_rate / MAX_BPM);
    
    // Clamp lag values to valid range
    max_lag = std::min(max_lag, buffer.size() / 2);
    min_lag = std::max(min_lag, size_t(2));
    
    // Calculate autocorrelation for each lag
    std::vector<float> autocorr(max_lag + 1, 0.0f);
    
    // Normalize the buffer
    float sum = 0.0f;
    for (float val : buffer) sum += val;
    float mean = sum / buffer.size();
    
    std::vector<float> normalized(buffer.size());
    for (size_t i = 0; i < buffer.size(); ++i) {
        normalized[i] = buffer[i] - mean;
    }
    
    // Calculate autocorrelation for each lag in our range of interest
    for (size_t lag = min_lag; lag <= max_lag; ++lag) {
        float acf = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;
        
        for (size_t i = 0; i < buffer.size() - lag; ++i) {
            acf += normalized[i] * normalized[i + lag];
            norm1 += normalized[i] * normalized[i];
            norm2 += normalized[i + lag] * normalized[i + lag];
        }
        
        // Normalize autocorrelation
        autocorr[lag] = acf / std::sqrt(norm1 * norm2 + 1e-6f);
    }
    
    // Find peaks in autocorrelation function
    std::vector<size_t> peaks;
    for (size_t i = min_lag + 1; i < max_lag; ++i) {
        if (autocorr[i] > autocorr[i-1] && autocorr[i] > autocorr[i+1] && autocorr[i] > 0.1f) {
            peaks.push_back(i);
        }
    }
    
    // No peaks found
    if (peaks.empty()) {
        return {120.0f, 0.0f}; 
    }
    
    // Sort peaks by correlation value
    std::sort(peaks.begin(), peaks.end(), [&autocorr](size_t a, size_t b) {
        return autocorr[a] > autocorr[b];
    });
    
    // Get the highest peak
    size_t best_lag = peaks[0]; 
    float confidence = autocorr[best_lag];
    
    // Handle octave errors by checking for related peaks
    for (size_t i = 1; i < std::min(peaks.size(), size_t(5)); ++i) {
        size_t lag = peaks[i];
        
        // Check if this peak is a multiple/divisor of our best lag
        for (int multiple = 2; multiple <= 4; ++multiple) {
            // Check if lag is close to a multiple of best_lag
            float multiple_ratio = static_cast<float>(lag) / best_lag;
            if (std::abs(multiple_ratio - multiple) < 0.1f || 
                std::abs(multiple_ratio - 1.0f/multiple) < 0.1f) {
                
                // Adjust our best lag based on octave error weight
                float weight = g_settings.octave_error_weight;
                if (multiple_ratio < 1.0f) {
                    // This peak suggests a faster tempo (smaller lag)
                    best_lag = static_cast<size_t>(best_lag * ((1.0f - weight) + weight * multiple_ratio));
                } else {
                    // This peak suggests a slower tempo (larger lag)
                    best_lag = static_cast<size_t>(best_lag * (weight + (1.0f - weight) * (1.0f / multiple_ratio)));
                }
                
                // Only consider one related peak
                break;
            }
        }
    }
    
    // Convert lag to BPM
    float bpm = 60.0f * frame_rate / best_lag;
    
    // Clamp BPM to reasonable range
    bpm = std::max(MIN_BPM, std::min(MAX_BPM, bpm));
    
    return {bpm, confidence};
}

/**
 * Detects beats using spectral flux + autocorrelation
 */
void DetectBeatsSpectralFluxAuto(const std::vector<float>& magnitudes, const AudioAnalysisConfig& config, 
                               AudioAnalysisData& out, float flux, float dt) {
    using clock = std::chrono::steady_clock;
    static auto last_call = clock::now();
    auto now = clock::now();
    
    // Calculate time since last frame for phase tracking
    float frame_time = now.time_since_epoch().count();
    float frame_dt = (out._last_frame_time > 0.0f) ? (frame_time - out._last_frame_time) : dt;
    out._last_frame_time = frame_time;
    
    // Store flux in our buffer for tempo analysis
    out._spectral_flux_buffer.push_back(flux);
    if (out._spectral_flux_buffer.size() > SPECTRAL_FLUX_BUFFER_SIZE) {
        out._spectral_flux_buffer.pop_front();
    }
    
    // Run autocorrelation tempo analysis after we have enough data
    if (out._spectral_flux_buffer.size() >= 256 && 
        (!out._autocorr_initialized || out._tempo_confidence < 0.4f)) {
        // Estimate FFT hop size based on frame rate
        size_t hop_size = config.fft_size / 4;  // A common hop size is FFT size / 4
        auto [bpm, confidence] = ComputeTempoFromAutocorrelation(out._spectral_flux_buffer, config.sample_rate, hop_size);
        
        // Only update tempo if confidence is high enough, or we don't have a tempo yet
        if (!out._autocorr_initialized || 
            confidence > out._tempo_confidence + g_settings.tempo_change_threshold) {
            out._current_tempo_bpm = bpm;
            out._tempo_confidence = confidence;
            out._autocorr_initialized = true;
        }
    }
    
    // Adaptive threshold for peak detection
    float threshold = out._flux_avg * 1.5f + g_settings.spectral_flux_threshold;
    
    // Beat phase tracking - adjust based on current tempo
    float beat_period = 60.0f / out._current_tempo_bpm;  // Period in seconds
    out._beat_phase += frame_dt / beat_period;
    while (out._beat_phase >= 1.0f) out._beat_phase -= 1.0f;
    
    // Detect onset (peaks in spectral flux)
    bool is_flux_peak = (flux > threshold);
    
    // Beat induction - if we detect a strong peak, adjust our phase to match it
    if (is_flux_peak && flux > g_settings.beat_flux_min) {
        // Determine beat induction window size
        float induction_window = g_settings.beat_induction_window;
        
        // Only adjust phase if we're in a reasonable range from expected beat
        if (out._beat_phase < induction_window || out._beat_phase > (1.0f - induction_window)) {
            // Reset phase to 0 (this was a beat)
            out._beat_phase = 0.0f;
            out.beat = 1.0f;
        }
    } else {
        // Generate beat events at phase 0 (with some confidence factor)
        float phase_trigger = 0.03f;  // Small window around phase 0
        
        if (out._beat_phase < phase_trigger && out._tempo_confidence > 0.3f) {
            // This is a predicted beat based on tempo
            out.beat = 1.0f * out._tempo_confidence;
        } else {
            // Fade out beat value
            out.beat -= (2.0f / beat_period) * dt;  // Rate based on tempo
            if (out.beat < 0.0f) out.beat = 0.0f;
        }
    }
}

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
    float sample_rate = config.sample_rate;
    float freq_resolution = sample_rate / (float)config.fft_size;
    
    // Calculate bin indices for beat frequency range
    size_t bin_low = (size_t)(min_beat_freq / freq_resolution);
    size_t bin_high = (size_t)(max_beat_freq / freq_resolution);
    
    // Clamp indices to valid range
    bin_high = std::min(bin_high, magnitudes.size() - 1);
    
    float flux_low = 0.0f; // Band-limited flux for beat detection
    
    if (!out._prev_magnitudes.empty() && out._prev_magnitudes.size() == magnitudes.size()) {
        // Calculate full-spectrum flux 
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
    
    // Full-spectrum flux
    if (out._flux_avg == 0.0f) out._flux_avg = flux;
    else out._flux_avg = (1.0f - flux_alpha) * out._flux_avg + flux_alpha * flux;
    
    // Band-limited flux with its own smoothing factor
    if (out._flux_low_avg == 0.0f) out._flux_low_avg = flux_low;
    else out._flux_low_avg = (1.0f - flux_low_alpha) * out._flux_low_avg + flux_low_alpha * flux_low;
    
    // --- 6. Beat Detection ---
    using clock = std::chrono::steady_clock;
    static auto last_call = clock::now();
    auto now = clock::now();
    float dt = std::chrono::duration<float>(now - last_call).count();
    last_call = now;
    
    // Choose beat detection algorithm based on configuration
    switch (config.beat_algorithm) {
        case BeatDetectionAlgorithm::SpectralFluxAuto:
            DetectBeatsSpectralFluxAuto(magnitudes, config, out, flux, dt);
            break;
            
        case BeatDetectionAlgorithm::SimpleEnergy:
        default:
            DetectBeatsSimpleEnergy(magnitudes, config, out, flux_low, dt);
            break;
    }
    
    // Keep track of current magnitudes for next frame
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