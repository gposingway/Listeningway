#pragma once

#include <array>
#include <string>

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
    } beat;

    // Frequency Band Settings
    struct FrequencyBands {
        bool logScaleEnabled = true;
        float logStrength = 1.0f;
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;
        
        // 5-band equalizer
        std::array<float, 5> equalizerBands = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
        float equalizerWidth = 0.2f;
        
        // Amplifier for visualization scaling
        float amplifier = 1.0f;
    } frequency;

    // Debug and Logging Settings
    struct Debug {
        bool debugEnabled = false;
        bool overlayEnabled = false;
    } debug;

    // Methods for persistence
    bool Save(const std::string& filename = "listeningway_config.json") const;
    bool Load(const std::string& filename = "listeningway_config.json");
    void ResetToDefaults();
    bool Validate();

    // Get default file path
    static std::string GetDefaultConfigPath();

private:
    bool SaveToJson(const std::string& filepath) const;
    bool LoadFromJson(const std::string& filepath);
};

}  // namespace Listeningway
