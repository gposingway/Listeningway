// ---------------------------------------------
// Audio Analysis Module Implementation
// Performs real-time audio feature extraction for Listeningway
// ---------------------------------------------
#include "audio_analysis.h"
#include "beat_detector.h"
#include "simple_energy_beat_detector.h"
#include "spectral_flux_auto_beat_detector.h"
#include "logging.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <mutex>
#include <kiss_fft.h>
#include <memory>
#include <complex>

// Global instance of AudioAnalyzer
AudioAnalyzer g_audio_analyzer;

// Standard standalone function to analyze audio buffers (used by audio_capture.cpp)
void AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, 
                        const AudioAnalysisConfig& config, AudioAnalysisData& out) {
    // Calculate volume (RMS)
    float sum_squares = 0.0f;
    for (size_t i = 0; i < numFrames * numChannels; i++) {
        sum_squares += data[i] * data[i];
    }
    float rms = std::sqrt(sum_squares / (numFrames * numChannels));
    out.volume = std::min(1.0f, rms * g_settings.volume_norm);
    
    // Resize frequency bands vector if needed
    if (out.freq_bands.size() != config.num_bands) {
        out.freq_bands.resize(config.num_bands, 0.0f);
        out.raw_freq_bands.resize(config.num_bands, 0.0f);
    }
    
    // Prepare FFT data
    const size_t fft_size = config.fft_size;
    const size_t half_fft_size = fft_size / 2;
    
    // Create FFT configuration
    kiss_fft_cfg fft_cfg = kiss_fft_alloc(fft_size, 0, nullptr, nullptr);
    if (!fft_cfg) {
        LOG_ERROR("[AudioAnalyzer] Failed to allocate FFT configuration");
        return;
    }
    
    // Prepare input and output arrays for FFT
    std::vector<kiss_fft_cpx> fft_in(fft_size);
    std::vector<kiss_fft_cpx> fft_out(fft_size);
    std::vector<float> magnitudes(half_fft_size);
    
    // Zero out the input array
    std::fill(fft_in.begin(), fft_in.end(), kiss_fft_cpx{0.0f, 0.0f});
    
    // Average all channels and copy to FFT input buffer with Hann window
    // Only copy up to fft_size frames, or pad with zeros if we have fewer
    const size_t frames_to_process = std::min(numFrames, fft_size);
    for (size_t i = 0; i < frames_to_process; i++) {
        float sample = 0.0f;
        
        // Average all channels
        for (size_t ch = 0; ch < numChannels; ch++) {
            sample += data[i * numChannels + ch];
        }
        sample /= numChannels;
        
        // Apply Hann window: 0.5 * (1 - cos(2π*n/(N-1)))
        const float window = 0.5f * (1.0f - std::cos(2.0f * 3.14159f * i / (fft_size - 1)));
        fft_in[i].r = sample * window;
        fft_in[i].i = 0.0f;
    }
    
    // Execute FFT
    kiss_fft(fft_cfg, fft_in.data(), fft_out.data());
    
    // Calculate magnitude of each FFT bin
    for (size_t i = 0; i < half_fft_size; i++) {
        // |c| = sqrt(real² + imag²)
        magnitudes[i] = std::sqrt(fft_out[i].r * fft_out[i].r + fft_out[i].i * fft_out[i].i);
    }
    
    // Calculate spectral flux (difference from previous magnitudes)
    float flux = 0.0f;
    float flux_low = 0.0f;
    const size_t low_freq_cutoff = half_fft_size / 4; // Bottom 25% of spectrum
    
    if (!out._prev_magnitudes.empty()) {
        for (size_t i = 0; i < half_fft_size; i++) {
            // Only count positive differences (increases in energy)
            float diff = std::max(0.0f, magnitudes[i] - out._prev_magnitudes[i]);
            flux += diff;
            
            // Also calculate flux for low frequencies only (for bass detection)
            if (i < low_freq_cutoff) {
                flux_low += diff;
            }
        }
        
        // Store the flux values for beat detection
        out._flux_avg = flux / half_fft_size;
        out._flux_low_avg = flux_low / low_freq_cutoff;
    }
    else {
        // Initialize previous magnitudes the first time
        out._prev_magnitudes.resize(half_fft_size);
        out._flux_avg = 0.0f;
        out._flux_low_avg = 0.0f;
    }
    
    // Store current magnitudes for next frame
    out._prev_magnitudes = magnitudes;
    
    // Populate frequency bands using proper mapping according to settings
    const size_t bands = config.num_bands;
    const bool use_log_scale = g_settings.band_log_scale;
    const float min_freq = g_settings.band_min_freq;
    const float max_freq = g_settings.band_max_freq;
    const float log_strength = g_settings.band_log_strength;
    const float nyquist_freq = config.sample_rate * 0.5f;
    
    // Ensure min and max are within sensible ranges for the FFT size
    const float effective_min_freq = std::max(20.0f, std::min(min_freq, nyquist_freq * 0.5f));
    const float effective_max_freq = std::min(nyquist_freq, std::max(max_freq, effective_min_freq * 2.0f));
    
    // Pre-calculate bin frequencies
    std::vector<float> bin_freqs(half_fft_size);
    for (size_t i = 0; i < half_fft_size; i++) {
        bin_freqs[i] = static_cast<float>(i) * nyquist_freq / half_fft_size;
    }
    
    // Clear the frequency bands before populating
    std::fill(out.freq_bands.begin(), out.freq_bands.end(), 0.0f);
    
    // FIXED IMPLEMENTATION: Addressing band gaps with more uniform distribution
    // Calculate band boundaries that ensure every FFT bin contributes to at least one band
    std::vector<float> band_edges(bands + 1);
    
    if (use_log_scale) {
        // Calculate logarithmic frequency bands with more uniform energy distribution
        // Use a simpler and more reliable logarithmic mapping to avoid gaps

        // Log-space boundaries ensure more even distribution
        const float log_min = std::log10(effective_min_freq);
        const float log_max = std::log10(effective_max_freq);
        const float log_range = log_max - log_min;

        for (size_t i = 0; i <= bands; i++) {
            // Apply power function for customizable log scaling
            float t = static_cast<float>(i) / bands;
            float adjusted_t = std::pow(t, log_strength);
            
            // Convert to frequency
            band_edges[i] = std::pow(10.0f, log_min + adjusted_t * log_range);
        }
    } else {
        // Linear frequency distribution - equally spaced bands
        for (size_t i = 0; i <= bands; i++) {
            band_edges[i] = effective_min_freq + 
                (effective_max_freq - effective_min_freq) * static_cast<float>(i) / bands;
        }
    }
    
    // Ensure band edges are monotonically increasing and within valid range
    for (size_t i = 0; i <= bands; i++) {
        band_edges[i] = std::max(effective_min_freq, std::min(effective_max_freq, band_edges[i]));
        if (i > 0) {
            band_edges[i] = std::max(band_edges[i], band_edges[i-1] + 1.0f);
        }
    }
    
    // Create a mapping of which FFT bins contribute to which bands
    // This is critical to ensure no gaps in the visualization
    std::vector<std::vector<size_t>> band_bins(bands);
    
    // First, determine which band each FFT bin belongs to primarily
    for (size_t i = 1; i < half_fft_size; i++) {
        float bin_freq = bin_freqs[i];
        
        // Skip if below minimum frequency
        if (bin_freq < band_edges[0]) {
            continue;
        }
        
        // Find the appropriate band for this bin
        for (size_t b = 0; b < bands; b++) {
            if (bin_freq >= band_edges[b] && bin_freq < band_edges[b + 1]) {
                band_bins[b].push_back(i);
                break;
            }
        }
    }
    
    // Ensure every band has at least one bin (fix gaps)
    for (size_t b = 0; b < bands; b++) {
        if (band_bins[b].empty()) {
            // This band has no bins assigned - find the closest bin
            float band_center = (band_edges[b] + band_edges[b + 1]) * 0.5f;
            size_t closest_bin = 1;
            float min_distance = std::numeric_limits<float>::max();
            
            for (size_t i = 1; i < half_fft_size; i++) {
                float distance = std::abs(bin_freqs[i] - band_center);
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_bin = i;
                }
            }
            
            // Assign this bin to the band
            band_bins[b].push_back(closest_bin);
        }
    }
    
    // Process each frequency band
    for (size_t band = 0; band < bands; band++) {
        // Calculate energy for this band from its assigned bins
        float energy_sum = 0.0f;
        
        // Sum energy from all bins assigned to this band
        for (size_t bin_idx : band_bins[band]) {
            energy_sum += magnitudes[bin_idx];
        }
        
        // Average the energy across all contributing bins
        float band_value = 0.0f;
        if (!band_bins[band].empty()) {
            band_value = energy_sum / band_bins[band].size();
        }
        
        // Store the raw (unmodified) band value for beat detection
        out.raw_freq_bands[band] = std::min(1.0f, band_value * g_settings.band_norm);
        
        // Calculate equalizer multiplier for visualization
        float equalizer_multiplier = 1.0f;
        
        // Calculate band center frequency - needed for both logarithmic and linear modes
        float band_center = std::sqrt(band_edges[band] * band_edges[band + 1]);
        
        // Apply 5-band equalizer system
        // Map band to normalized position from 0.0 to 1.0 across the frequency range
        float normalized_pos = static_cast<float>(band) / (bands - 1);
        
        // Calculate contributions from each modifier based on position
        // Apply bell curve (Gaussian) weighting to each modifier's influence
        
        // Bell curve centers for 5 bands (evenly distributed)
        const float centers[5] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
        
        // Width of the bell curve from user settings (smaller = sharper peaks, larger = more overlap)
        const float bell_width = g_settings.equalizer_width;
        
        // Apply bell curves from all 5 modifiers with position-based weighting
        float total_weight = 0.0f;
        float weighted_modifier = 0.0f;
        
        // Calculate weighted contribution from each modifier
        for (int i = 0; i < 5; i++) {
            // Calculate Gaussian weight: e^(-(x^2)/(2*sigma^2))
            float distance = normalized_pos - centers[i];
            float bell_value = std::exp(-(distance * distance) / (2.0f * bell_width * bell_width));
            
            // Get the corresponding modifier value
            float modifier_value = 1.0f; // Default
            switch (i) {
                case 0: modifier_value = g_settings.equalizer_band1; break;
                case 1: modifier_value = g_settings.equalizer_band2; break;
                case 2: modifier_value = g_settings.equalizer_band3; break;
                case 3: modifier_value = g_settings.equalizer_band4; break;
                case 4: modifier_value = g_settings.equalizer_band5; break;
            }
            
            weighted_modifier += bell_value * modifier_value;
            total_weight += bell_value;
        }
        
        // Normalize the weighted modifier if we have any weight
        if (total_weight > 0.0f) {
            equalizer_multiplier = weighted_modifier / total_weight;
        }
        
        // Apply the equalizer multiplier to the raw band value for visualization
        // When equalizer is set to 0, the band will be close to 0 (true multiplier)
        out.freq_bands[band] = out.raw_freq_bands[band] * equalizer_multiplier;
    }
    
    // --- Audio Spatialization: Calculate left/right volume and pan ---
    // Calculate per-channel RMS for left/right (and pan)
    float sum_left = 0.0f, sum_right = 0.0f, sum_center = 0.0f;
    float sum_side_left = 0.0f, sum_side_right = 0.0f;
    float sum_rear_left = 0.0f, sum_rear_right = 0.0f;
    float sum_total = 0.0f;
    size_t count_left = 0, count_right = 0, count_center = 0;
    size_t count_side_left = 0, count_side_right = 0, count_rear_left = 0, count_rear_right = 0;
    // Channel mapping: FL=0, FR=1, C=2, LFE=3, SL=4, SR=5, RL=6, RR=7 (ITU-R BS.775)
    for (size_t i = 0; i < numFrames; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            float sample = data[i * numChannels + ch];
            sum_total += sample * sample;
            switch (numChannels) {
                case 1: // Mono
                    sum_left += sample * sample;
                    sum_right += sample * sample;
                    count_left++;
                    count_right++;
                    break;
                case 2: // Stereo
                    if (ch == 0) { sum_left += sample * sample; count_left++; }
                    else if (ch == 1) { sum_right += sample * sample; count_right++; }
                    break;
                case 6: // 5.1 Surround (FL, FR, C, LFE, SL, SR)
                    if (ch == 0) { sum_left += sample * sample; count_left++; }
                    else if (ch == 1) { sum_right += sample * sample; count_right++; }
                    else if (ch == 2) { sum_center += sample * sample; count_center++; }
                    else if (ch == 4) { sum_side_left += sample * sample; count_side_left++; }
                    else if (ch == 5) { sum_side_right += sample * sample; count_side_right++; }
                    break;
                case 8: // 7.1 Surround (FL, FR, C, LFE, RL, RR, SL, SR)
                    if (ch == 0) { sum_left += sample * sample; count_left++; }
                    else if (ch == 1) { sum_right += sample * sample; count_right++; }
                    else if (ch == 2) { sum_center += sample * sample; count_center++; }
                    else if (ch == 4) { sum_rear_left += sample * sample; count_rear_left++; }
                    else if (ch == 5) { sum_rear_right += sample * sample; count_rear_right++; }
                    else if (ch == 6) { sum_side_left += sample * sample; count_side_left++; }
                    else if (ch == 7) { sum_side_right += sample * sample; count_side_right++; }
                    break;
                default:
                    // Fallback: treat first two channels as L/R
                    if (ch == 0) { sum_left += sample * sample; count_left++; }
                    else if (ch == 1) { sum_right += sample * sample; count_right++; }
                    break;
            }
        }
    }
    // Calculate RMS for each channel group
    float rms_left = count_left ? std::sqrt(sum_left / count_left) : 0.0f;
    float rms_right = count_right ? std::sqrt(sum_right / count_right) : 0.0f;
    float rms_center = count_center ? std::sqrt(sum_center / count_center) : 0.0f;
    float rms_side_left = count_side_left ? std::sqrt(sum_side_left / count_side_left) : 0.0f;
    float rms_side_right = count_side_right ? std::sqrt(sum_side_right / count_side_right) : 0.0f;
    float rms_rear_left = count_rear_left ? std::sqrt(sum_rear_left / count_rear_left) : 0.0f;    float rms_rear_right = count_rear_right ? std::sqrt(sum_rear_right / count_rear_right) : 0.0f;
      // --- Calculate pan value in [-1, +1] for uniform ---
    float pan_norm = 0.0f;
    
    // Set audio format for uniform exposure
    out.audio_format = static_cast<float>(numChannels);
    
    if (numChannels == 1) {
        pan_norm = 0.0f;    } else if (numChannels == 2) {
        // Use enhanced difference-based pan calculation with exponential scaling
        float l = rms_left;
        float r = rms_right;        if (l + r > 0.0001f) {            // Simple difference-based pan calculation that should give full range
            // When one channel is silent and other has signal, this should give ±1.0
            float basic_pan = (r - l) / (l + r);
            
            // For debugging: if one channel is nearly zero, force to extreme
            if (l < 0.001f && r > 0.01f) {
                pan_norm = 1.0f; // Right dominant, pan right
            } else if (r < 0.001f && l > 0.01f) {
                pan_norm = -1.0f;  // Left dominant, pan left
            } else {
                // Use the calculated value with slight amplification for subtle differences
                pan_norm = std::clamp(basic_pan * 1.5f, -1.0f, 1.0f);
            }
        } else {
            pan_norm = 0.0f;
        }} else if (numChannels == 6 || numChannels == 8) {
        // Check if this is effectively stereo content (only FL/FR have significant energy)
        float front_left_right_energy = rms_left + rms_right;
        float other_channels_energy = rms_center + rms_side_left + rms_side_right + rms_rear_left + rms_rear_right;
        float total_energy = front_left_right_energy + other_channels_energy;
        
        // If 95% or more of the energy is in FL/FR channels, treat as stereo
        bool is_effectively_stereo = (total_energy > 0.001f) && 
                                   ((front_left_right_energy / total_energy) >= 0.95f);
          if (is_effectively_stereo) {
            // Use stereo calculation for full [-1, +1] range
            float l = rms_left;
            float r = rms_right;
              if (l + r > 0.0001f) {
                float basic_pan = (r - l) / (l + r);
                
                // For debugging: if one channel is nearly zero, force to extreme
                if (l < 0.001f && r > 0.01f) {
                    pan_norm = 1.0f; // Right dominant, pan right
                } else if (r < 0.001f && l > 0.01f) {
                    pan_norm = -1.0f;  // Left dominant, pan left
                } else {
                    // Use the calculated value with slight amplification for subtle differences
                    pan_norm = std::clamp(basic_pan * 1.5f, -1.0f, 1.0f);
                }
            } else {
                pan_norm = 0.0f;
            }
        } else {
            // True surround content - use vector sum with ITU-R BS.775 angles
            // Angles (degrees): FL=-30, FR=+30, C=0, SL=-90, SR=+90, RL=-150, RR=+150
            struct ChanAngle { float rms; float deg; } chans[7] = {
                { rms_left, -30.0f },
                { rms_right, +30.0f },
                { rms_center, 0.0f },
                { rms_side_left, -90.0f },
                { rms_side_right, +90.0f },
                { rms_rear_left, -150.0f },
                { rms_rear_right, +150.0f }
            };
            float x = 0.0f, y = 0.0f;
            for (const auto& c : chans) {
                float rad = c.deg * 3.14159265f / 180.0f;
                x += c.rms * std::cos(rad);
                y += c.rms * std::sin(rad);
            }
            float pan_deg = 0.0f;
            if (x != 0.0f || y != 0.0f) {
                pan_deg = std::atan2(y, x) * 180.0f / 3.14159265f;
            }
            pan_norm = std::clamp(pan_deg / 90.0f, -1.0f, 1.0f); // -1 to +1
        }
    } else {
        pan_norm = 0.0f;    }
    // Now normalize for display only
    out.volume_left = std::min(1.0f, rms_left * g_settings.volume_norm);
    out.volume_right = std::min(1.0f, rms_right * g_settings.volume_norm);
    
    // Apply pan smoothing if enabled
    static float smoothed_pan = 0.0f;
    static bool pan_initialized = false;
    
    if (g_settings.pan_smoothing > 0.0f) {
        if (!pan_initialized) {
            smoothed_pan = pan_norm;
            pan_initialized = true;
        } else {
            // Exponential moving average: smoothed = (1-alpha) * previous + alpha * current
            // Higher smoothing value = more smoothing (slower response)
            float alpha = 1.0f / (1.0f + g_settings.pan_smoothing * 10.0f); // Scale smoothing factor
            smoothed_pan = (1.0f - alpha) * smoothed_pan + alpha * pan_norm;
        }
        out.audio_pan = smoothed_pan;
    } else {
        // No smoothing, use raw value
        out.audio_pan = pan_norm;
        pan_initialized = false; // Reset for when smoothing is re-enabled
    }
    
    // Free FFT configuration
    kiss_fft_free(fft_cfg);
    
    // If audio analysis is disabled, reset all values
    if (!g_settings.audio_analysis_enabled) {
        out.volume = 0.0f;
        std::fill(out.freq_bands.begin(), out.freq_bands.end(), 0.0f);
        out.beat = 0.0f;
    }
}

// Implementation of AudioAnalyzer

AudioAnalyzer::AudioAnalyzer() : beat_detector_(nullptr), current_algorithm_(0), is_running_(false) {
    LOG_DEBUG("[AudioAnalyzer] Constructed");
}

AudioAnalyzer::~AudioAnalyzer() {
    Stop();
    LOG_DEBUG("[AudioAnalyzer] Destroyed");
}

void AudioAnalyzer::SetBeatDetectionAlgorithm(int algorithm) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (current_algorithm_ == algorithm && beat_detector_ != nullptr) {
        // No change needed
        return;
    }
    
    // Stop current detector if running
    if (beat_detector_) {
        LOG_DEBUG("[AudioAnalyzer] Stopping current beat detector");
        beat_detector_->Stop();
    }
    
    // Create new detector
    LOG_DEBUG("[AudioAnalyzer] Creating new beat detector with algorithm: " + std::to_string(algorithm));
    beat_detector_ = IBeatDetector::Create(algorithm);
    current_algorithm_ = algorithm;
    
    // Start new detector if analyzer is running
    if (is_running_ && beat_detector_) {
        LOG_DEBUG("[AudioAnalyzer] Starting new beat detector");
        beat_detector_->Start();
    }
}

int AudioAnalyzer::GetBeatDetectionAlgorithm() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_algorithm_;
}

void AudioAnalyzer::Start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_running_) {
        LOG_DEBUG("[AudioAnalyzer] Already running, ignoring Start() call");
        return;
    }
    
    // Create detector if needed
    if (!beat_detector_) {
        LOG_DEBUG("[AudioAnalyzer] Creating default beat detector");
        beat_detector_ = IBeatDetector::Create(current_algorithm_);
    }
    
    // Start the detector
    if (beat_detector_) {
        LOG_DEBUG("[AudioAnalyzer] Starting beat detector");
        beat_detector_->Start();
    }
    
    is_running_ = true;
    LOG_DEBUG("[AudioAnalyzer] Started");
}

void AudioAnalyzer::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_running_) {
        LOG_DEBUG("[AudioAnalyzer] Already stopped, ignoring Stop() call");
        return;
    }
    
    // Stop the detector
    if (beat_detector_) {
        LOG_DEBUG("[AudioAnalyzer] Stopping beat detector");
        beat_detector_->Stop();
    }
    
    is_running_ = false;
    LOG_DEBUG("[AudioAnalyzer] Stopped");
}

void AudioAnalyzer::AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, 
                                      const AudioAnalysisConfig& config, AudioAnalysisData& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_running_ || !beat_detector_) {
        // If not running, just zero out the data
        out.volume = 0.0f;
        std::fill(out.freq_bands.begin(), out.freq_bands.end(), 0.0f);
        out.beat = 0.0f;
        return;
    }
    
    // Call the standalone AnalyzeAudioBuffer function to perform the actual analysis
    ::AnalyzeAudioBuffer(data, numFrames, numChannels, config, out);
    
    // Update beat analysis
    if (beat_detector_) {
        // Feed the data to the beat detector - extract flux and other values from the analysis data
        // Since the detector needs magnitudes, flux, and time, we need to compute these from our data
        float dt = 1.0f / config.sample_rate * numFrames; // Time delta based on sample rate and frames
        
        // Process this frame with the beat detector
        // Important: We use raw audio analysis data for beat detection rather than
        // the visualized/equalized data to ensure consistent beat detection
        beat_detector_->Process(out._prev_magnitudes, out._flux_avg, out._flux_low_avg, dt);
        
        // Get beat information from the detector
        BeatDetectorResult result = beat_detector_->GetResult();
        
        // Update the audio analysis data with the beat detection results
        out.beat = result.beat;
        out.tempo_bpm = result.tempo_bpm;
        out.tempo_confidence = result.confidence;
        out.beat_phase = result.beat_phase;
        out.tempo_detected = result.tempo_detected;
    }
}