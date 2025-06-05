// filepath: f:\Replica\NAS\Files\repo\github\Listeningway\src\core\settings_new.cpp
// Modern settings wrapper using new ConfigurationManager
#include "constants.h"
#include "settings.h"
#include "audio_analysis.h"
#include "logging.h"
#include "configuration/configuration_manager.h"
#include <windows.h>
#include <mutex>
#include <string>
#include <atomic>

// Backward compatibility: provide g_settings as a reference to the new configuration
extern AudioAnalyzer g_audio_analyzer;

/**
 * @brief Gets the current configuration from ConfigurationManager
 * @return Reference to the current configuration
 */
static const Listeningway::Configuration& GetConfig() {
    return Listeningway::ConfigurationManager::Snapshot();
}

/**
 * @brief Wrapper to provide legacy g_settings interface
 * This class provides backward compatibility by mapping the old g_settings 
 * struct fields to the new configuration system
 */
class LegacySettingsWrapper {
public:
    // Audio analysis settings
    size_t num_bands() const { return GetConfig().numBands; }
    size_t fft_size() const { return GetConfig().fftSize; }
    float flux_alpha() const { return GetConfig().fluxAlpha; }
    float flux_threshold_multiplier() const { return GetConfig().fluxThresholdMultiplier; }
    
    // Beat detection settings
    float beat_min_freq() const { return GetConfig().beatMinFreq; }
    float beat_max_freq() const { return GetConfig().beatMaxFreq; }
    float flux_low_alpha() const { return GetConfig().fluxLowAlpha; }
    float flux_low_threshold_multiplier() const { return GetConfig().fluxLowThresholdMultiplier; }
    float beat_flux_min() const { return GetConfig().beatFluxMin; }
    float beat_falloff_default() const { return GetConfig().beatFalloffDefault; }
    float beat_time_scale() const { return GetConfig().beatTimeScale; }
    float beat_time_initial() const { return GetConfig().beatTimeInitial; }
    float beat_time_min() const { return GetConfig().beatTimeMin; }
    float beat_time_divisor() const { return GetConfig().beatTimeDivisor; }
    float volume_norm() const { return GetConfig().volumeNorm; }
    float band_norm() const { return GetConfig().bandNorm; }
    float capture_stale_timeout() const { return GetConfig().captureStaleTimeout; }
    
    // Beat detection algorithm
    size_t beat_detection_algorithm() const { return GetConfig().beatDetectionAlgorithm; }
    
    // Advanced spectral flux settings
    float spectral_flux_threshold() const { return GetConfig().spectralFluxThreshold; }
    float tempo_change_threshold() const { return GetConfig().tempoChangeThreshold; }
    float beat_induction_window() const { return GetConfig().beatInductionWindow; }
    float octave_error_weight() const { return GetConfig().octaveErrorWeight; }
    float spectral_flux_decay_multiplier() const { return GetConfig().spectralFluxDecayMultiplier; }
    
    // General settings
    bool audio_analysis_enabled() const { return GetConfig().audioAnalysisEnabled; }
    bool debug_enabled() const { return GetConfig().debugEnabled; }
    bool band_log_scale() const { return GetConfig().bandLogScale; }
    float band_min_freq() const { return GetConfig().bandMinFreq; }
    float band_max_freq() const { return GetConfig().bandMaxFreq; }
    float band_log_strength() const { return GetConfig().bandLogStrength; }
    
    // Equalizer settings
    float equalizer_band1() const { return GetConfig().equalizerBand1; }
    float equalizer_band2() const { return GetConfig().equalizerBand2; }
    float equalizer_band3() const { return GetConfig().equalizerBand3; }
    float equalizer_band4() const { return GetConfig().equalizerBand4; }
    float equalizer_band5() const { return GetConfig().equalizerBand5; }
    float equalizer_width() const { return GetConfig().equalizerWidth; }
    
    // Audio capture provider
    const std::string& audio_capture_provider_code() const { return GetConfig().audioCaptureProviderCode; }
    
    // Pan smoothing
    float pan_smoothing() const { return GetConfig().panSmoothing; }
};

// Create a single instance that provides the legacy interface
static LegacySettingsWrapper g_settings_wrapper;

// Legacy global settings pointer - points to wrapper
ListeningwaySettings g_settings = {
    .num_bands = GetConfig().numBands,
    .fft_size = GetConfig().fftSize,
    .flux_alpha = GetConfig().fluxAlpha,
    .flux_threshold_multiplier = GetConfig().fluxThresholdMultiplier,
    .beat_min_freq = GetConfig().beatMinFreq,
    .beat_max_freq = GetConfig().beatMaxFreq,
    .flux_low_alpha = GetConfig().fluxLowAlpha,
    .flux_low_threshold_multiplier = GetConfig().fluxLowThresholdMultiplier,
    .beat_flux_min = GetConfig().beatFluxMin,
    .beat_falloff_default = GetConfig().beatFalloffDefault,
    .beat_time_scale = GetConfig().beatTimeScale,
    .beat_time_initial = GetConfig().beatTimeInitial,
    .beat_time_min = GetConfig().beatTimeMin,
    .beat_time_divisor = GetConfig().beatTimeDivisor,
    .volume_norm = GetConfig().volumeNorm,
    .band_norm = GetConfig().bandNorm,
    .capture_stale_timeout = GetConfig().captureStaleTimeout,
    .beat_detection_algorithm = GetConfig().beatDetectionAlgorithm,
    .spectral_flux_threshold = GetConfig().spectralFluxThreshold,
    .tempo_change_threshold = GetConfig().tempoChangeThreshold,
    .beat_induction_window = GetConfig().beatInductionWindow,
    .octave_error_weight = GetConfig().octaveErrorWeight,
    .spectral_flux_decay_multiplier = GetConfig().spectralFluxDecayMultiplier,
    .audio_analysis_enabled = GetConfig().audioAnalysisEnabled,
    .debug_enabled = GetConfig().debugEnabled,
    .band_log_scale = GetConfig().bandLogScale,
    .band_min_freq = GetConfig().bandMinFreq,
    .band_max_freq = GetConfig().bandMaxFreq,
    .band_log_strength = GetConfig().bandLogStrength,
    .equalizer_band1 = GetConfig().equalizerBand1,
    .equalizer_band2 = GetConfig().equalizerBand2,
    .equalizer_band3 = GetConfig().equalizerBand3,
    .equalizer_band4 = GetConfig().equalizerBand4,
    .equalizer_band5 = GetConfig().equalizerBand5,
    .equalizer_width = GetConfig().equalizerWidth,
    .audio_capture_provider_code = GetConfig().audioCaptureProviderCode,
    .pan_smoothing = GetConfig().panSmoothing
};

/**
 * @brief Retrieves the path to the settings .ini file.
 * @return The full path to the Listeningway.ini file.
 */
std::string GetSettingsPath() {
    char dllPath[MAX_PATH] = {};
    HMODULE hModule = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetSettingsPath, &hModule);
    GetModuleFileNameA(hModule, dllPath, MAX_PATH);
    std::string path(dllPath);
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) path = path.substr(0, pos + 1);
    return path + "Listeningway.ini";
}

/**
 * @brief Retrieves the path to the log file.
 * @return The full path to the listeningway.log file.
 */
std::string GetLogFilePath() {
    std::string ini = GetSettingsPath();
    size_t pos = ini.find_last_of("\\/");
    std::string dir = (pos != std::string::npos) ? ini.substr(0, pos + 1) : "";
    return dir + "listeningway.log";
}

/**
 * @brief Loads settings using the new configuration system
 */
void LoadSettings() {
    LOG_DEBUG("[Settings] Loading settings using ConfigurationManager");
    Listeningway::ConfigurationManager::Instance().LoadFromFile();
}

/**
 * @brief Saves current settings using the new configuration system
 */
void SaveSettings() {
    LOG_DEBUG("[Settings] Saving settings using ConfigurationManager");
    Listeningway::ConfigurationManager::Instance().SaveToFile();
}

/**
 * @brief Gets the current state of audio analysis.
 * @return True if audio analysis is enabled, false otherwise.
 */
bool GetAudioAnalysisEnabled() {
    return GetConfig().audioAnalysisEnabled;
}

/**
 * @brief Sets the state of audio analysis and saves the setting.
 * @param enabled True to enable audio analysis, false to disable.
 */
void SetAudioAnalysisEnabled(bool enabled) {
    auto& config_manager = Listeningway::ConfigurationManager::Instance();
    auto config = config_manager.GetConfiguration();
    config.audioAnalysisEnabled = enabled;
    config_manager.SetConfiguration(config);
    
    LOG_DEBUG("[Settings] Audio analysis " + std::string(enabled ? "enabled" : "disabled"));
}

/**
 * @brief Gets the current state of debug mode.
 * @return True if debug mode is enabled, false otherwise.
 */
bool GetDebugEnabled() {
    return GetConfig().debugEnabled;
}

/**
 * @brief Sets the state of debug mode and saves the setting.
 * @param enabled True to enable debug mode, false to disable.
 */
void SetDebugEnabled(bool enabled) {
    auto& config_manager = Listeningway::ConfigurationManager::Instance();
    auto config = config_manager.GetConfiguration();
    config.debugEnabled = enabled;
    config_manager.SetConfiguration(config);
    
    LOG_DEBUG("[Settings] Debug mode " + std::string(enabled ? "enabled" : "disabled"));
}

/**
 * @brief Loads all tunable settings using the new configuration system
 */
void LoadAllTunables() {
    LOG_DEBUG("[Settings] Loading all tunable settings using ConfigurationManager");
    Listeningway::ConfigurationManager::Instance().LoadFromFile();
}

/**
 * @brief Saves all tunable settings using the new configuration system
 */
void SaveAllTunables() {
    LOG_DEBUG("[Settings] Saving all tunable settings using ConfigurationManager");
    Listeningway::ConfigurationManager::Instance().SaveToFile();
}

/**
 * @brief Resets all tunable settings to their default values using the new configuration system
 */
void ResetAllTunablesToDefaults() {
    LOG_DEBUG("[Settings] Resetting all settings to defaults using ConfigurationManager");
    Listeningway::ConfigurationManager::Instance().RestartAudioSystems();
}

/**
 * @brief Apply settings using the new configuration system
 */
void ApplySettings() {
    LOG_DEBUG("[Settings] Applying current configuration settings to live systems");
    Listeningway::ConfigurationManager::Instance().ApplyConfigToLiveSystems();
}
