#pragma once

#include <array>
#include <string>
#include "../constants.h"

namespace Listeningway {

/**
 * @brief Modern configuration structure with direct property access
 * 
 * This structure contains all application settings organized into logical groups.
 * Properties can be accessed directly without getter/setter methods.
 * Use Save()/Load() methods for persistence.
 */
struct Configuration {
    // Audio Analysis Settings
    struct Audio {
        bool analysisEnabled = false;
        std::string captureProviderCode = "off"; // provider code string, e.g. "system", "process", "off"
        // int captureProvider = -1;  // (legacy, remove after migration)
        float panSmoothing = 0.1f;
    } audio;

    // Beat Detection Settings
    struct BeatDetection {
        int algorithm = 0;  // 0 = Simple Energy, 1 = Spectral Flux + Autocorrelation
        
        // Simple Energy Algorithm Settings
        float falloffDefault = 2.0f;
        float timeScale = 1e-9f;
        float timeInitial = 0.5f;
        float timeMin = 0.1f;
        float timeDivisor = 0.2f;
        
        // Spectral Flux + Autocorrelation Algorithm Settings
        float spectralFluxThreshold = 0.05f;
        float spectralFluxDecayMultiplier = 2.0f;
        float tempoChangeThreshold = 0.3f;
        float beatInductionWindow = 0.1f;
        float octaveErrorWeight = 0.7f;
          // Band-Limited Beat Detection
        float minFreq = 0.0f;
        float maxFreq = 500.0f;
        float fluxLowAlpha = 0.1f;
        float fluxLowThresholdMultiplier = 2.0f;
        float fluxMin = DEFAULT_BEAT_FLUX_MIN;
    } beat;

    // Frequency Band Settings
    struct FrequencyBands {
        bool logScaleEnabled = DEFAULT_BAND_LOG_SCALE;
        float logStrength = DEFAULT_BAND_LOG_STRENGTH;
        float minFreq = DEFAULT_BAND_MIN_FREQ;
        float maxFreq = DEFAULT_BAND_MAX_FREQ;
        std::array<float, 5> equalizerBands = { DEFAULT_EQUALIZER_BAND1, DEFAULT_EQUALIZER_BAND2, DEFAULT_EQUALIZER_BAND3, DEFAULT_EQUALIZER_BAND4, DEFAULT_EQUALIZER_BAND5 };
        float equalizerWidth = DEFAULT_EQUALIZER_WIDTH;
        float amplifier = DEFAULT_AMPLIFIER;
        size_t bands = DEFAULT_NUM_BANDS;
        size_t fftSize = DEFAULT_FFT_SIZE;
        float bandNorm = DEFAULT_BAND_NORM;
    } frequency;

    // Audio sample rate (Hz)
    float sample_rate = 48000.0f;

    // Debug and Logging Settings
    struct Debug {
        bool debugEnabled = false;
        bool overlayEnabled = false;
    } debug;

    // Methods for persistence
    bool Save() const;
    bool Load();
    void ResetToDefaults();
    bool Validate();

    // Returns the full path to the default config file (same dir as INI/log)
    static std::string GetDefaultConfigPath();

private:
    bool SaveToJson(const std::string& filepath) const;
    bool LoadFromJson(const std::string& filepath);
};

}  // namespace Listeningway
