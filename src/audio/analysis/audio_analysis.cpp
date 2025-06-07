// ---------------------------------------------
// Audio Analysis Module Implementation
// Performs real-time audio feature extraction for Listeningway
// ---------------------------------------------
#include "audio_analysis.h"
#include "beat_detector.h"
#include "beat_detector_simple_energy.h"
#include "beat_detector_spectral_flux_auto.h"
#include "logging.h"
#include "configuration/configuration_manager.h"
#include "../core/audio_format_utils.h"
#include "../core/constants.h"
using Listeningway::ConfigurationManager;
#include <algorithm>
#include <numeric>
#include <cmath>
#include <mutex>
#include <kiss_fft.h>
#include <memory>
#include <complex>
#include <sstream>

// Simple format function for numeric values
std::string formatFloat(float value, int precision = 6) {
    std::ostringstream oss;
    oss.precision(precision);
    oss << std::fixed << value;
    return oss.str();
}

// Global instance of AudioAnalyzer
AudioAnalyzer g_audio_analyzer;

// Standard standalone function to analyze audio buffers (used by audio_capture.cpp)
void AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, AudioAnalysisData& out) {
    const auto config = Listeningway::ConfigurationManager::Snapshot(); // Thread-safe snapshot for audio capture threads
    
    // DEBUG: Validate input data
    static int input_debug_counter = 0;
    if (++input_debug_counter % 1000 == 0) {
        float sample_min = 1000.0f, sample_max = -1000.0f;
        size_t sample_count = std::min(numFrames * numChannels, size_t(100)); // Check first 100 samples
        for (size_t i = 0; i < sample_count; i++) {
            sample_min = std::min(sample_min, data[i]);
            sample_max = std::max(sample_max, data[i]);
        }        LOG_INFO("[AUDIO_INPUT_DEBUG] Frames=" + std::to_string(numFrames) + 
            ", Channels=" + std::to_string(numChannels) + 
            ", SampleRange=[" + std::to_string(sample_min) + ", " + std::to_string(sample_max) + "]");
    }
      // Calculate volume (RMS)
    float sum_squares = 0.0f;
    for (size_t i = 0; i < numFrames * numChannels; i++) {
        sum_squares += data[i] * data[i];
    }
    float rms = std::sqrt(sum_squares / (numFrames * numChannels));
    out.volume = std::min(1.0f, rms * config.frequency.amplifier); // Use amplifier for normalization
    
    // DEBUG: Check for potential numerical issues
    static int numerical_debug_counter = 0;
    if (++numerical_debug_counter % 500 == 0 && sum_squares > 0.0f) {
        float avg_square = sum_squares / (numFrames * numChannels);        LOG_INFO("[NUMERICAL_DEBUG] SumSquares=" + std::to_string(sum_squares) + 
            ", AvgSquare=" + std::to_string(avg_square) + 
            ", RMS=" + std::to_string(rms) + 
            ", FinalVol=" + std::to_string(out.volume) + 
            ", Amp=" + std::to_string(config.frequency.amplifier));
        
        // Check for extreme values
        if (rms > 1.0f) {
            LOG_INFO("[NUMERICAL_WARNING] RMS > 1.0: " + std::to_string(rms) + " (before amplifier)");
        }
        if (std::isnan(rms) || std::isinf(rms)) {
            LOG_INFO("[NUMERICAL_ERROR] Invalid RMS value: " + std::to_string(rms));
        }
    }
    
    // Resize frequency bands vector if needed
    if (out.freq_bands.size() != config.frequency.bands) {
        out.freq_bands.resize(config.frequency.bands, 0.0f);
        out.raw_freq_bands.resize(config.frequency.bands, 0.0f);
    }
    
    // Prepare FFT data
    const size_t fft_size = config.frequency.fftSize;
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
    const size_t bands = config.frequency.bands;
    const bool use_log_scale = config.frequency.logScaleEnabled;
    const float min_freq = config.frequency.minFreq;
    const float max_freq = config.frequency.maxFreq;
    const float log_strength = config.frequency.logStrength;
    // TODO: Add fftSize, bandNorm, sample_rate to Configuration if needed, or define locally/with constants
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
        // Always use equal log steps for band edges
        const float log_min = std::log10(effective_min_freq);
        const float log_max = std::log10(effective_max_freq);
        const float log_range = log_max - log_min;
        for (size_t i = 0; i <= bands; i++) {
            float t = static_cast<float>(i) / bands;
            band_edges[i] = std::pow(10.0f, log_min + t * log_range);
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
        out.raw_freq_bands[band] = std::min(1.0f, band_value * config.frequency.bandNorm);
        
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
        const float bell_width = config.frequency.equalizerWidth;
        
        // Apply bell curves from all 5 modifiers with position-based weighting
        float total_weight = 0.0f;
        float weighted_modifier = 0.0f;
        
        // Calculate weighted contribution from each modifier
        for (int i = 0; i < 5; i++) {
            // Calculate Gaussian weight: e^(-(x^2)/(2*sigma^2))
            float distance = normalized_pos - centers[i];
            float bell_value = std::exp(-(distance * distance) / (2.0f * bell_width * bell_width));
            
            // Get the corresponding modifier value
            float modifier_value = config.frequency.equalizerBands[i];
            
            weighted_modifier += bell_value * modifier_value;
            total_weight += bell_value;
        }
        
        // Normalize the weighted modifier if we have any weight
        if (total_weight > 0.0f) {
            equalizer_multiplier = weighted_modifier / total_weight;
        }
        
        // Apply the equalizer multiplier to the raw band value for visualization
        out.freq_bands[band] = out.raw_freq_bands[band] * equalizer_multiplier;
        // Apply logStrength as a logarithmic gain curve (only if log scale is enabled)
        if (use_log_scale && log_strength != 0.0f) {
            float gain = std::exp(static_cast<float>(band + 1) * (log_strength/3));
            out.freq_bands[band] *= gain;
        }
    }
    
    // --- Audio Spatialization: Calculate left/right volume and pan ---
    // Calculate per-channel RMS for left/right (and pan)
    float sum_left = 0.0f, sum_right = 0.0f, sum_center = 0.0f;
    float sum_side_left = 0.0f, sum_side_right = 0.0f;
    float sum_rear_left = 0.0f, sum_rear_right = 0.0f;
    float sum_total = 0.0f;
    size_t count_left = 0, count_right = 0, count_center = 0;
    size_t count_side_left = 0, count_side_right = 0, count_rear_left = 0, count_rear_right = 0;    // Get the audio format once outside the loop for efficiency
    AudioFormat format = AudioFormatUtils::IntToFormat(static_cast<int>(numChannels));
    
    // Channel mapping: FL=0, FR=1, C=2, LFE=3, SL=4, SR=5, RL=6, RR=7 (ITU-R BS.775)
    for (size_t i = 0; i < numFrames; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            float sample = data[i * numChannels + ch];
            sum_total += sample * sample;
            
            // Simplified and more reliable channel identification for stereo
            if (numChannels == 2) {
                // For stereo: channel 0 = left, channel 1 = right
                if (ch == 0) {
                    sum_left += sample * sample;
                    count_left++;
                } else if (ch == 1) {
                    sum_right += sample * sample;
                    count_right++;
                }
            } else if (numChannels == 1) {
                // For mono: single channel counts as both left and right
                sum_left += sample * sample;
                sum_right += sample * sample;
                count_left++;
                count_right++;
            } else {
                // For surround sound, use utility functions
                if (AudioFormatUtils::IsLeftChannel(format, ch)) {
                    sum_left += sample * sample;
                    count_left++;
                }
                if (AudioFormatUtils::IsRightChannel(format, ch)) {
                    sum_right += sample * sample;
                    count_right++;
                }
                if (AudioFormatUtils::IsCenterChannel(format, ch)) {
                    sum_center += sample * sample;
                    count_center++;
                }
                if (AudioFormatUtils::IsSideChannel(format, ch)) {
                    if (ch == 4 || ch == 6) { // SL channels
                        sum_side_left += sample * sample;
                        count_side_left++;
                    } else { // SR channels
                        sum_side_right += sample * sample;
                        count_side_right++;
                    }
                }
                if (AudioFormatUtils::IsRearChannel(format, ch)) {
                    if (ch == 4) { // RL
                        sum_rear_left += sample * sample;
                        count_rear_left++;
                    } else { // RR
                        sum_rear_right += sample * sample;
                        count_rear_right++;
                    }
                }
            }
        }
    }    // Calculate RMS for each channel group    // For stereo, we should have exactly numFrames samples per channel
    float rms_left, rms_right;
      if (numChannels == 2) {
        // For stereo, use a direct calculation to avoid any potential counting issues
        float sum_left_direct = 0.0f, sum_right_direct = 0.0f;
        for (size_t i = 0; i < numFrames; ++i) {
            float left_sample = data[i * 2 + 0];   // Channel 0
            float right_sample = data[i * 2 + 1];  // Channel 1
            sum_left_direct += left_sample * left_sample;
            sum_right_direct += right_sample * right_sample;
        }
        rms_left = std::sqrt(sum_left_direct / numFrames);
        rms_right = std::sqrt(sum_right_direct / numFrames);
        
        // DEBUG: Log direct RMS calculation details
        if (rms_left + rms_right > 0.001f) {
            static int rms_debug_counter = 0;
            if (++rms_debug_counter % 200 == 0) {  // Log every 200th frame
                float balance = (rms_right - rms_left) / std::max(rms_right + rms_left, 0.000001f);                LOG_INFO("[RMS_DEBUG_DIRECT] Frames=" + std::to_string(numFrames) + 
                    ", SumL=" + formatFloat(sum_left_direct, 8) + 
                    ", SumR=" + formatFloat(sum_right_direct, 8) + 
                    ", RMSL=" + formatFloat(rms_left, 8) + 
                    ", RMSR=" + formatFloat(rms_right, 8) + 
                    ", Balance=" + formatFloat(balance, 6));
                    
                // Log some sample values for inspection
                if (numFrames >= 4) {                    LOG_INFO("[RMS_DEBUG_SAMPLES] Sample0: L=" + formatFloat(data[0], 6) + 
                        ", R=" + formatFloat(data[1], 6) + " | Sample1: L=" + formatFloat(data[2], 6) + 
                        ", R=" + formatFloat(data[3], 6));
                }
            }
        }
    } else {
        // For other formats, use the counting approach
        rms_left = count_left ? std::sqrt(sum_left / count_left) : 0.0f;
        rms_right = count_right ? std::sqrt(sum_right / count_right) : 0.0f;
        
        // DEBUG: Log channel mapping for non-stereo formats
        static int mapping_debug_counter = 0;
        if (++mapping_debug_counter % 300 == 0) {            LOG_INFO("[RMS_DEBUG_MAPPING] Channels=" + std::to_string(numChannels) + 
                ", CountL=" + std::to_string(count_left) + 
                ", CountR=" + std::to_string(count_right) + 
                ", SumL=" + formatFloat(sum_left, 6) + 
                ", SumR=" + formatFloat(sum_right, 6) + 
                ", RMSL=" + formatFloat(rms_left, 6) + 
                ", RMSR=" + formatFloat(rms_right, 6));
        }
    }
    
    float rms_center = count_center ? std::sqrt(sum_center / count_center) : 0.0f;
    float rms_side_left = count_side_left ? std::sqrt(sum_side_left / count_side_left) : 0.0f;
    float rms_side_right = count_side_right ? std::sqrt(sum_side_right / count_side_right) : 0.0f;
    float rms_rear_left = count_rear_left ? std::sqrt(sum_rear_left / count_rear_left) : 0.0f;
    float rms_rear_right = count_rear_right ? std::sqrt(sum_rear_right / count_rear_right) : 0.0f;
    // --- Calculate pan value in [-1, +1] for uniform ---
    float pan_norm = 0.0f;

    // Set audio format for uniform exposure
    out.audio_format = static_cast<float>(numChannels);
    // --- Always set L/R volume here, before pan offset logic ---
    out.volume_left = std::min(1.0f, rms_left * config.frequency.amplifier);
    out.volume_right = std::min(1.0f, rms_right * config.frequency.amplifier);
    if (numChannels == 1) {
        // Mono audio should always be perfectly centered
        pan_norm = 0.0f;
    } else if (numChannels == 2) {
        // For stereo audio, calculate pan based on left/right RMS difference
        float l = rms_left;
        float r = rms_right;

        // DEBUG: Enhanced pan calculation debugging
        if (l + r > 0.001f) {  // Only log when there's significant audio
            static int debug_counter = 0;
            if (++debug_counter % 50 == 0) {  // Log every 50th frame for more frequent monitoring
                float sum = l + r;
                float diff = r - l;
                float abs_diff = std::abs(diff);
                float rel_diff = abs_diff / sum;
                LOG_INFO("[PAN_DEBUG_STEREO] Frame=" + std::to_string(debug_counter) +
                    ", L=" + formatFloat(l, 6) +
                    ", R=" + formatFloat(r, 6) +
                    ", Sum=" + formatFloat(sum, 6) +
                    ", Diff=" + formatFloat(diff, 6) +
                    ", AbsDiff=" + formatFloat(abs_diff, 6) +
                    ", RelDiff=" + formatFloat(rel_diff * 100.0f, 6) + "%");
                // Log threshold comparison
                LOG_INFO("[PAN_DEBUG_THRESHOLDS] BalanceDeadzone=" + formatFloat(DEFAULT_PAN_BALANCE_DEADZONE * 100.0f, 6) +
                    "%, SilenceThresh=" + formatFloat(DEFAULT_PAN_SILENCE_THRESHOLD, 8) +
                    ", SigThresh=" + formatFloat(DEFAULT_PAN_SIGNIFICANT_THRESHOLD, 6));
            }
        }
        // Only calculate pan if there's sufficient signal energy
        if (l + r > 0.0001f) {
            // Improved pan calculation with deadzone to handle balanced audio better
            const float balance_deadzone = DEFAULT_PAN_BALANCE_DEADZONE;
            const float silence_threshold = DEFAULT_PAN_SILENCE_THRESHOLD;
            const float significant_threshold = DEFAULT_PAN_SIGNIFICANT_THRESHOLD;

            // Calculate relative difference between channels
            float diff = std::abs(r - l);
            float sum = l + r;
            float relative_diff = diff / sum;

            // DEBUG: Log decision logic path
            static int decision_debug_counter = 0;
            bool should_log_decision = (++decision_debug_counter % 50 == 0);

            // If the difference is within the deadzone, consider it balanced (centered)
            if (relative_diff < balance_deadzone) {
                pan_norm = 0.0f;
                if (should_log_decision) {
                    LOG_INFO("[PAN_DECISION] DEADZONE: RelDiff=" + formatFloat(relative_diff * 100.0f, 6) +
                        "% < Deadzone=" + formatFloat(balance_deadzone * 100.0f, 6) + "% -> Pan=0.0");
                }
            }
            // Handle extreme cases where one channel is nearly silent
            else if (l < silence_threshold && r > significant_threshold) {
                pan_norm = 1.0f; // Right dominant, pan right
                if (should_log_decision) {
                    LOG_INFO("[PAN_DECISION] RIGHT_DOMINANT: L=" + formatFloat(l, 8) +
                        " < SilThresh=" + formatFloat(silence_threshold, 8) +
                        " && R=" + formatFloat(r, 6) +
                        " > SigThresh=" + formatFloat(significant_threshold, 6) + " -> Pan=+1.0");
                }
            } else if (r < silence_threshold && l > significant_threshold) {
                pan_norm = -1.0f;  // Left dominant, pan left
                if (should_log_decision) {
                    LOG_INFO("[PAN_DECISION] LEFT_DOMINANT: R=" + formatFloat(r, 8) +
                        " < SilThresh=" + formatFloat(silence_threshold, 8) +
                        " && L=" + formatFloat(l, 6) +
                        " > SigThresh=" + formatFloat(significant_threshold, 6) + " -> Pan=-1.0");
                }
            } else {
                // Standard difference-based pan calculation: (right - left) / (left + right)
                float basic_pan = (r - l) / (l + r);
                pan_norm = std::clamp(basic_pan, -1.0f, 1.0f);
                if (should_log_decision) {
                    LOG_INFO("[PAN_DECISION] NORMAL_CALC: BasicPan=(R-L)/(R+L)=(" + formatFloat(r, 6) +
                        "-" + formatFloat(l, 6) + ")/" + formatFloat(sum, 6) + "=" + formatFloat(basic_pan, 6) +
                        " -> FinalPan=" + formatFloat(pan_norm, 6));
                }
                // DEBUG: Log calculated pan for diagnosis
                static int pan_debug_counter = 0;
                if (++pan_debug_counter % 100 == 0) {
                    LOG_INFO("[PAN_DEBUG] Basic_Pan=" + formatFloat(basic_pan, 6) +
                        ", Final_Pan=" + formatFloat(pan_norm, 6));
                }
            }
        } else {
            pan_norm = 0.0f;
            static int silence_debug_counter = 0;
            if (++silence_debug_counter % 200 == 0) {  // Log silence state occasionally
                LOG_INFO("[PAN_DEBUG] SILENCE_STATE: L+R=" + formatFloat(l + r, 8) + " <= EnergyThresh=0.0001, Pan=0.0");
            }
        }
    } else if (numChannels == 6 || numChannels == 8) {
        // Check if this is effectively stereo content (only FL/FR have significant energy)
        float front_left_right_energy = rms_left + rms_right;
        float other_channels_energy = rms_center + rms_side_left + rms_side_right + rms_rear_left + rms_rear_right;
        float total_energy = front_left_right_energy + other_channels_energy;

        // DEBUG: Log surround sound analysis
        static int surround_debug_counter = 0;
        if (++surround_debug_counter % 100 == 0) {
            float stereo_ratio = (total_energy > 0.001f) ? (front_left_right_energy / total_energy) : 0.0f;
            LOG_INFO("[PAN_DEBUG_SURROUND] Channels=" + std::to_string(numChannels) +
                ", FrontLR=" + formatFloat(front_left_right_energy, 6) +
                ", Other=" + formatFloat(other_channels_energy, 6) +
                ", Total=" + formatFloat(total_energy, 6) +
                ", StereoRatio=" + formatFloat(stereo_ratio * 100.0f, 3) + "%");
        }

        // If 95% or more of the energy is in FL/FR channels, treat as stereo
        bool is_effectively_stereo = (total_energy > 0.001f) &&
                                   ((front_left_right_energy / total_energy) >= 0.95f);
        if (is_effectively_stereo) {
            // Use stereo calculation for full [-1, +1] range
            float l = rms_left;
            float r = rms_right;

            static int eff_stereo_debug_counter = 0;
            if (++eff_stereo_debug_counter % 100 == 0) {
                LOG_INFO("[PAN_DEBUG] EFFECTIVELY_STEREO: Using stereo calculation for " + std::to_string(numChannels) + "-ch audio");
            }

            if (l + r > 0.0001f) {
                // Use the same improved logic as pure stereo
                const float balance_deadzone = DEFAULT_PAN_BALANCE_DEADZONE;
                const float silence_threshold = DEFAULT_PAN_SILENCE_THRESHOLD;
                const float significant_threshold = DEFAULT_PAN_SIGNIFICANT_THRESHOLD;

                float diff = std::abs(r - l);
                float sum = l + r;
                float relative_diff = diff / sum;

                if (relative_diff < balance_deadzone) {
                    pan_norm = 0.0f;
                } else if (l < silence_threshold && r > significant_threshold) {
                    pan_norm = 1.0f; // Right dominant, pan right
                } else if (r < silence_threshold && l > significant_threshold) {
                    pan_norm = -1.0f;  // Left dominant, pan left
                } else {
                    float basic_pan = (r - l) / (l + r);
                    pan_norm = std::clamp(basic_pan, -1.0f, 1.0f);
                }
            } else {
                pan_norm = 0.0f;
            }
        } else {
            // True surround content - use vector sum with ITU-R BS.775 angles
            static int true_surround_debug_counter = 0;
            if (++true_surround_debug_counter % 100 == 0) {
                LOG_INFO("[PAN_DEBUG] TRUE_SURROUND: Using vector calculation for " + std::to_string(numChannels) +
                    "-ch audio. FL=" + formatFloat(rms_left, 4) +
                    ", FR=" + formatFloat(rms_right, 4) +
                    ", C=" + formatFloat(rms_center, 4) +
                    ", SL=" + formatFloat(rms_side_left, 4) +
                    ", SR=" + formatFloat(rms_side_right, 4) +
                    ", RL=" + formatFloat(rms_rear_left, 4) +
                    ", RR=" + formatFloat(rms_rear_right, 4));
            }

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

            // DEBUG: Log vector calculation details
            static int vector_debug_counter = 0;
            if (++vector_debug_counter % 100 == 0) {
                LOG_INFO("[PAN_DEBUG_VECTOR] X=" + formatFloat(x, 6) +
                    ", Y=" + formatFloat(y, 6) +
                    ", PanDeg=" + formatFloat(pan_deg, 2) +
                    ", PanNorm=" + formatFloat(pan_norm, 6));
            }
        }
    } else {
        pan_norm = 0.0f;
        static int unsupported_debug_counter = 0;
        if (++unsupported_debug_counter % 200 == 0) {
            LOG_INFO("[PAN_DEBUG] UNSUPPORTED_FORMAT: " + std::to_string(numChannels) + " channels, Pan=0.0");
        }
    }    // --- At this point, pan_norm is the detected pan value ---
    // Apply user pan offset (from config)
    float pan_offset = std::clamp(config.audio.panOffset, -1.0f, 1.0f);
    float pan_with_offset = std::clamp(pan_norm + pan_offset, -1.0f, 1.0f);
    if (std::abs(pan_offset) > 0.0001f) {
        LOG_INFO("[PAN_OFFSET] User panOffset=" + formatFloat(pan_offset, 4) + ", DetectedPan=" + formatFloat(pan_norm, 4) + ", PanWithOffset=" + formatFloat(pan_with_offset, 4));
    }
    // Use pan_with_offset for smoothing/output
    
    // Apply pan smoothing if enabled
    static float smoothed_pan = 0.0f;
    static bool pan_initialized = false;
    
    if (config.audio.panSmoothing > 0.0f) {
        if (!pan_initialized) {
            smoothed_pan = pan_with_offset;
            pan_initialized = true;
            LOG_INFO("[PAN_SMOOTHING] INITIALIZED: smoothed_pan=" + formatFloat(smoothed_pan, 6) + ", raw_pan=" + formatFloat(pan_with_offset, 6));
        } else {
            float alpha = 1.0f / (1.0f + config.audio.panSmoothing * 10.0f);
            float prev_smoothed = smoothed_pan;
            smoothed_pan = (1.0f - alpha) * smoothed_pan + alpha * pan_with_offset;
            static int smoothing_debug_counter = 0;
            if (++smoothing_debug_counter % 200 == 0) {
                float smoothing_strength = config.audio.panSmoothing;
                LOG_INFO("[PAN_SMOOTHING] Strength=" + formatFloat(smoothing_strength, 3) + ", Alpha=" + formatFloat(alpha, 6) + ", Raw=" + formatFloat(pan_with_offset, 6) + ", Prev=" + formatFloat(prev_smoothed, 6) + ", New=" + formatFloat(smoothed_pan, 6) + ", Delta=" + formatFloat(smoothed_pan - prev_smoothed, 6));
            }
        }
        out.audio_pan = smoothed_pan;
    } else {
        out.audio_pan = pan_with_offset;
        static int no_smoothing_debug_counter = 0;
        if (++no_smoothing_debug_counter % 500 == 0) {
            LOG_INFO("[PAN_SMOOTHING] DISABLED: Using raw pan value=" + formatFloat(pan_with_offset, 6));
        }
        pan_initialized = false;
    }
    
    // Free FFT configuration
    kiss_fft_free(fft_cfg);
    
    // If audio analysis is disabled, reset all values
    if (!config.audio.analysisEnabled) {
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

void AudioAnalyzer::AnalyzeAudioBuffer(const float* data, size_t numFrames, size_t numChannels, AudioAnalysisData& out) {
    std::lock_guard<std::mutex> lock(mutex_);
      if (!is_running_ || !beat_detector_) {
        out.volume = 0.0f;
        std::fill(out.freq_bands.begin(), out.freq_bands.end(), 0.0f);
        out.beat = 0.0f;
        return;
    }
    
    const auto config = Listeningway::ConfigurationManager::Snapshot(); // Thread-safe snapshot for audio analysis thread
    // Call the standalone AnalyzeAudioBuffer function to perform the actual analysis
    ::AnalyzeAudioBuffer(data, numFrames, numChannels, out);
    
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