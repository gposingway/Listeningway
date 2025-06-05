#pragma once
#include "settings.h"
#include "configuration/ConfigurationManager.h"
#include <array>

/**
 * @brief Adapter to bridge ListeningwaySettings and ConfigurationManager for DRY settings persistence.
 */
class SettingsAdapter {
public:
    static void Load(ListeningwaySettings& settings) {
        auto& config = ConfigurationManager::Instance().GetConfig();
        // Map ListeningwaySettings fields to Configuration fields
        // NOTE: Some fields in ListeningwaySettings do not exist in Configuration and are not mapped
        settings.beat_min_freq = config.beat.minFreq;
        settings.beat_max_freq = config.beat.maxFreq;
        settings.flux_low_alpha = config.beat.fluxLowAlpha;
        settings.flux_low_threshold_multiplier = config.beat.fluxLowThresholdMultiplier;
        settings.beat_detection_algorithm = config.beat.algorithm;
        settings.beat_falloff_default = config.beat.falloffDefault;
        settings.beat_time_scale = config.beat.timeScale;
        settings.beat_time_initial = config.beat.timeInitial;
        settings.beat_time_min = config.beat.timeMin;
        settings.beat_time_divisor = config.beat.timeDivisor;
        settings.spectral_flux_threshold = config.beat.spectralFluxThreshold;
        settings.spectral_flux_decay_multiplier = config.beat.spectralFluxDecayMultiplier;
        settings.tempo_change_threshold = config.beat.tempoChangeThreshold;
        settings.beat_induction_window = config.beat.beatInductionWindow;
        settings.octave_error_weight = config.beat.octaveErrorWeight;
        settings.band_log_scale = config.frequency.logScaleEnabled;
        settings.band_log_strength = config.frequency.logStrength;
        settings.band_min_freq = config.frequency.minFreq;
        settings.band_max_freq = config.frequency.maxFreq;
        settings.equalizer_band1 = config.frequency.equalizerBands[0];
        settings.equalizer_band2 = config.frequency.equalizerBands[1];
        settings.equalizer_band3 = config.frequency.equalizerBands[2];
        settings.equalizer_band4 = config.frequency.equalizerBands[3];
        settings.equalizer_band5 = config.frequency.equalizerBands[4];
        settings.equalizer_width = config.frequency.equalizerWidth;
        settings.audio_capture_provider_code = config.audio.captureProviderCode;
        settings.pan_smoothing = config.audio.panSmoothing;
        settings.debug_enabled = config.debug.debugEnabled;
        settings.audio_analysis_enabled = config.audio.analysisEnabled;
    }

    static void Save(const ListeningwaySettings& settings) {
        auto& config = ConfigurationManager::Instance().GetConfig();
        config.beat.minFreq = settings.beat_min_freq;
        config.beat.maxFreq = settings.beat_max_freq;
        config.beat.fluxLowAlpha = settings.flux_low_alpha;
        config.beat.fluxLowThresholdMultiplier = settings.flux_low_threshold_multiplier;
        config.beat.algorithm = settings.beat_detection_algorithm;
        config.beat.falloffDefault = settings.beat_falloff_default;
        config.beat.timeScale = settings.beat_time_scale;
        config.beat.timeInitial = settings.beat_time_initial;
        config.beat.timeMin = settings.beat_time_min;
        config.beat.timeDivisor = settings.beat_time_divisor;
        config.beat.spectralFluxThreshold = settings.spectral_flux_threshold;
        config.beat.spectralFluxDecayMultiplier = settings.spectral_flux_decay_multiplier;
        config.beat.tempoChangeThreshold = settings.tempo_change_threshold;
        config.beat.beatInductionWindow = settings.beat_induction_window;
        config.beat.octaveErrorWeight = settings.octave_error_weight;
        config.frequency.logScaleEnabled = settings.band_log_scale;
        config.frequency.logStrength = settings.band_log_strength;
        config.frequency.minFreq = settings.band_min_freq;
        config.frequency.maxFreq = settings.band_max_freq;
        config.frequency.equalizerBands[0] = settings.equalizer_band1;
        config.frequency.equalizerBands[1] = settings.equalizer_band2;
        config.frequency.equalizerBands[2] = settings.equalizer_band3;
        config.frequency.equalizerBands[3] = settings.equalizer_band4;
        config.frequency.equalizerBands[4] = settings.equalizer_band5;
        config.frequency.equalizerWidth = settings.equalizer_width;
        config.audio.captureProviderCode = settings.audio_capture_provider_code;
        config.audio.panSmoothing = settings.pan_smoothing;
        config.debug.debugEnabled = settings.debug_enabled;
        config.audio.analysisEnabled = settings.audio_analysis_enabled;
        ConfigurationManager::Instance().Save();
    }

    static void ResetToDefaults(ListeningwaySettings& settings) {
        ConfigurationManager::Instance().ResetToDefaults();
        Load(settings);
    }
};
