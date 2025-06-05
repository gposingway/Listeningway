#pragma once

/**
 * @brief Compatibility header that replaces the old g_settings system
 * 
 * This header provides the same interface as the old ListeningwaySettings struct
 * but delegates all operations to the new GlobalConfiguration system.
 * 
 * This allows existing code to continue working without modification while
 * using the new unified configuration system underneath.
 */

#include <atomic>

/**
 * @brief Modern replacement for the old g_settings global variable
 * 
 * This struct provides the same interface as ListeningwaySettings but delegates
 * all operations to the GlobalConfiguration system. This ensures all modules
 * access the same centralized configuration.
 */
struct ConfigurationView {    // Audio analysis toggle - provides atomic interface for compatibility
    std::atomic<bool>& audio_analysis_enabled() {
        static std::atomic<bool> value{false};
        value.store(g_config->audio.analysisEnabled);
        return value;
    }
      // Direct access to configuration values through properties
    size_t num_bands() const { return g_config->audio.numBands; }
    size_t fft_size() const { return g_config->audio.fftSize; }    float flux_alpha() const { return g_config->audio.fluxAlpha; }
    float flux_threshold_multiplier() const { return g_config->audio.fluxThresholdMultiplier; }
      // Beat detection settings
    float beat_min_freq() const { return g_config->beat.minFreq; }
    float beat_max_freq() const { return g_config->beat.maxFreq; }
    float flux_low_alpha() const { return g_config->beat.fluxLowAlpha; }
    float flux_low_threshold_multiplier() const { return g_config->beat.fluxLowThresholdMultiplier; }
      float beat_flux_min() const { return g_config->beat.beatFluxMin; }
    float beat_falloff_default() const { return g_config->beat.falloffDefault; }
    float beat_time_scale() const { return g_config->beat.timeScale; }
    float beat_time_initial() const { return g_config->beat.timeInitial; }
    float beat_time_min() const { return g_config->beat.timeMin; }
    float beat_time_divisor() const { return g_config->beat.timeDivisor; }
    float volume_norm() const { return g_config->audio.volumeNorm; }
    float band_norm() const { return g_config->audio.bandNorm; }
    float capture_stale_timeout() const { return g_config->audio.captureStaleTimeout; }
    int beat_detection_algorithm() const { return g_config->beat.algorithm; }      // Spectral flux autocorrelation settings
    float spectral_flux_threshold() const { return g_config->beat.spectralFluxThreshold; }
    float tempo_change_threshold() const { return g_config->beat.tempoChangeThreshold; }
    float beat_induction_window() const { return g_config->beat.beatInductionWindow; }
    float octave_error_weight() const { return g_config->beat.octaveErrorWeight; }
    float spectral_flux_decay_multiplier() const { return g_config->beat.spectralFluxDecayMultiplier; }
    
    bool debug_enabled() const { return g_config->debug.debugEnabled; }    bool band_log_scale() const { return g_config->frequency.logScaleEnabled; }
    float band_min_freq() const { return g_config->frequency.minFreq; }
    float band_max_freq() const { return g_config->frequency.maxFreq; }
    float band_log_strength() const { return g_config->frequency.logStrength; }
    
    // 5-band equalizer settings
    float equalizer_band1() const { return g_config->frequency.equalizerBands[0]; }
    float equalizer_band2() const { return g_config->frequency.equalizerBands[1]; }
    float equalizer_band3() const { return g_config->frequency.equalizerBands[2]; }
    float equalizer_band4() const { return g_config->frequency.equalizerBands[3]; }
    float equalizer_band5() const { return g_config->frequency.equalizerBands[4]; }
    float equalizer_width() const { return g_config->frequency.equalizerWidth; }
      // Audio capture provider code
    std::string audio_capture_provider_code() const { return g_config->audio.captureProviderCode; }
    
    // Pan smoothing factor
    float pan_smoothing() const { return g_config->frequency.panSmoothing; }
};

// Global configuration view that provides the same interface as the old g_settings
extern ConfigurationView g_settings;
