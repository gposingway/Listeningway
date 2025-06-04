#include "ConfigurationManager.h"
#include "../logging.h"
#include "../constants.h"
#include <algorithm>

// Singleton instance
ConfigurationManager& ConfigurationManager::Instance() {
    static ConfigurationManager instance;
    return instance;
}

ConfigurationManager::ConfigurationManager() : m_settings() {
    // Initialize with defaults
    LoadSettingsFromFile();
}

// Configuration access (read-only interface)
float ConfigurationManager::GetPanSmoothing() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.pan_smoothing;
}

float ConfigurationManager::GetSpectralFluxThreshold() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.spectral_flux_threshold;
}

float ConfigurationManager::GetTempoChangeThreshold() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.tempo_change_threshold;
}

float ConfigurationManager::GetBeatInductionWindow() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.beat_induction_window;
}

float ConfigurationManager::GetOctaveErrorWeight() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.octave_error_weight;
}

float ConfigurationManager::GetSpectralFluxDecayMultiplier() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.spectral_flux_decay_multiplier;
}

bool ConfigurationManager::IsLogScaleEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.band_log_scale;
}

float ConfigurationManager::GetBandMinFreq() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.band_min_freq;
}

float ConfigurationManager::GetBandMaxFreq() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.band_max_freq;
}

float ConfigurationManager::GetBandLogStrength() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.band_log_strength;
}

float ConfigurationManager::GetEqualizerBand(int band) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    switch (band) {
        case 0: return m_settings.equalizer_band1;
        case 1: return m_settings.equalizer_band2;
        case 2: return m_settings.equalizer_band3;
        case 3: return m_settings.equalizer_band4;
        case 4: return m_settings.equalizer_band5;
        default: return 0.0f;
    }
}

float ConfigurationManager::GetEqualizerWidth() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.equalizer_width;
}

int ConfigurationManager::GetBeatDetectionAlgorithm() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.beat_detection_algorithm;
}

int ConfigurationManager::GetAudioCaptureProvider() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.audio_capture_provider;
}

bool ConfigurationManager::IsAudioAnalysisEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.audio_analysis_enabled.load();
}

bool ConfigurationManager::IsDebugEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.debug_enabled;
}

const ListeningwaySettings& ConfigurationManager::GetAllSettings() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings;
}

// Configuration modification with validation
bool ConfigurationManager::SetPanSmoothing(float value) {
    return SetValue(m_settings.pan_smoothing, value, "pan_smoothing", 
        [this](float v) { return ValidatePanSmoothing(v); });
}

bool ConfigurationManager::SetSpectralFluxThreshold(float value) {
    return SetValue(m_settings.spectral_flux_threshold, value, "spectral_flux_threshold",
        [this](float v) { return ValidateSpectralFluxThreshold(v); });
}

bool ConfigurationManager::SetTempoChangeThreshold(float value) {
    return SetValue(m_settings.tempo_change_threshold, value, "tempo_change_threshold",
        [this](float v) { return ValidateTempoChangeThreshold(v); });
}

bool ConfigurationManager::SetBeatInductionWindow(float value) {
    return SetValue(m_settings.beat_induction_window, value, "beat_induction_window",
        [this](float v) { return ValidateBeatInductionWindow(v); });
}

bool ConfigurationManager::SetLogScaleEnabled(bool enabled) {
    return SetValue(m_settings.band_log_scale, enabled, "band_log_scale");
}

bool ConfigurationManager::SetBandMinFreq(float value) {
    return SetValue(m_settings.band_min_freq, value, "band_min_freq",
        [this](float v) { return ValidateBandMinFreq(v); });
}

bool ConfigurationManager::SetBandMaxFreq(float value) {
    return SetValue(m_settings.band_max_freq, value, "band_max_freq",
        [this](float v) { return ValidateBandMaxFreq(v); });
}

bool ConfigurationManager::SetEqualizerBand(int band, float value) {
    if (!ValidateEqualizerBand(band, value)) {
        m_lastError = "Invalid equalizer band index or value";
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    float oldValue = 0.0f;
    float* target = nullptr;
    std::string settingName = "equalizer_band" + std::to_string(band + 1);
    
    switch (band) {
        case 0: target = &m_settings.equalizer_band1; oldValue = m_settings.equalizer_band1; break;
        case 1: target = &m_settings.equalizer_band2; oldValue = m_settings.equalizer_band2; break;
        case 2: target = &m_settings.equalizer_band3; oldValue = m_settings.equalizer_band3; break;
        case 3: target = &m_settings.equalizer_band4; oldValue = m_settings.equalizer_band4; break;
        case 4: target = &m_settings.equalizer_band5; oldValue = m_settings.equalizer_band5; break;
        default: return false;
    }
    
    if (target && oldValue != value) {
        *target = value;
        NotifyListeners(settingName, ConfigValue(oldValue), ConfigValue(value));
        return true;
    }
    return false;
}

bool ConfigurationManager::SetBeatDetectionAlgorithm(int algorithm) {
    if (!ValidateBeatDetectionAlgorithm(algorithm)) {
        m_lastError = "Invalid beat detection algorithm";
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_settings.beat_detection_algorithm != algorithm) {
        int oldAlgorithm = m_settings.beat_detection_algorithm;
        m_settings.beat_detection_algorithm = algorithm;
        
        // Business logic integration
        OnBeatDetectionAlgorithmChanged(algorithm);
        
        NotifyListeners("beat_detection_algorithm", ConfigValue(oldAlgorithm), ConfigValue(algorithm));
        return true;
    }
    return false;
}

bool ConfigurationManager::SetAudioAnalysisEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool oldValue = m_settings.audio_analysis_enabled.load();
    if (oldValue != enabled) {
        m_settings.audio_analysis_enabled.store(enabled);
        
        // Business logic integration
        OnAudioAnalysisEnabledChanged(enabled);
        
        NotifyListeners("audio_analysis_enabled", ConfigValue(oldValue), ConfigValue(enabled));
        return true;
    }
    return false;
}

bool ConfigurationManager::SetDebugEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_settings.debug_enabled != enabled) {
        bool oldValue = m_settings.debug_enabled;
        m_settings.debug_enabled = enabled;
        
        // Business logic integration
        OnDebugEnabledChanged(enabled);
        
        NotifyListeners("debug_enabled", ConfigValue(oldValue), ConfigValue(enabled));
        return true;
    }
    return false;
}

// UI helper methods (for ImGui integration)
float* ConfigurationManager::GetPanSmoothingRef() {
    return &m_settings.pan_smoothing;
}

float* ConfigurationManager::GetSpectralFluxThresholdRef() {
    return &m_settings.spectral_flux_threshold;
}

bool* ConfigurationManager::GetLogScaleEnabledRef() {
    return &m_settings.band_log_scale;
}

float* ConfigurationManager::GetBandMinFreqRef() {
    return &m_settings.band_min_freq;
}

float* ConfigurationManager::GetBandMaxFreqRef() {
    return &m_settings.band_max_freq;
}

float* ConfigurationManager::GetEqualizerBandRef(int band) {
    switch (band) {
        case 0: return &m_settings.equalizer_band1;
        case 1: return &m_settings.equalizer_band2;
        case 2: return &m_settings.equalizer_band3;
        case 3: return &m_settings.equalizer_band4;
        case 4: return &m_settings.equalizer_band5;
        default: return nullptr;
    }
}

int* ConfigurationManager::GetBeatDetectionAlgorithmRef() {
    return &m_settings.beat_detection_algorithm;
}

// Bulk operations
bool ConfigurationManager::LoadFromFile() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return LoadSettingsFromFile();
}

bool ConfigurationManager::SaveToFile() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return SaveSettingsToFile();
}

bool ConfigurationManager::ResetToDefaults() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Store old settings for notification
    ListeningwaySettings oldSettings = m_settings;
    
    // Reset to defaults (using constants from constants.h)
    m_settings.pan_smoothing = DEFAULT_PAN_SMOOTHING;
    m_settings.spectral_flux_threshold = DEFAULT_SPECTRAL_FLUX_THRESHOLD;
    m_settings.tempo_change_threshold = DEFAULT_TEMPO_CHANGE_THRESHOLD;
    m_settings.beat_induction_window = DEFAULT_BEAT_INDUCTION_WINDOW;
    m_settings.octave_error_weight = DEFAULT_OCTAVE_ERROR_WEIGHT;
    m_settings.spectral_flux_decay_multiplier = DEFAULT_SPECTRAL_FLUX_DECAY_MULTIPLIER;
    m_settings.band_log_scale = DEFAULT_BAND_LOG_SCALE;
    m_settings.band_min_freq = DEFAULT_BAND_MIN_FREQ;
    m_settings.band_max_freq = DEFAULT_BAND_MAX_FREQ;
    m_settings.band_log_strength = DEFAULT_BAND_LOG_STRENGTH;
    m_settings.equalizer_band1 = DEFAULT_EQUALIZER_BAND1;
    m_settings.equalizer_band2 = DEFAULT_EQUALIZER_BAND2;
    m_settings.equalizer_band3 = DEFAULT_EQUALIZER_BAND3;
    m_settings.equalizer_band4 = DEFAULT_EQUALIZER_BAND4;
    m_settings.equalizer_band5 = DEFAULT_EQUALIZER_BAND5;
    m_settings.equalizer_width = DEFAULT_EQUALIZER_WIDTH;
    m_settings.beat_detection_algorithm = DEFAULT_BEAT_DETECTION_ALGORITHM;
    m_settings.audio_analysis_enabled.store(DEFAULT_AUDIO_ANALYSIS_ENABLED);
    m_settings.debug_enabled = DEFAULT_DEBUG_ENABLED;
    
    // Notify listeners of all changes
    NotifyListeners("settings_reset", ConfigValue("old"), ConfigValue("defaults"));
    
    return true;
}

// Change notification system
void ConfigurationManager::AddChangeListener(IConfigurationChangeListener* listener) {
    if (listener) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_listeners.push_back(listener);
    }
}

void ConfigurationManager::RemoveChangeListener(IConfigurationChangeListener* listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.erase(std::remove(m_listeners.begin(), m_listeners.end(), listener), m_listeners.end());
}

// Validation methods
bool ConfigurationManager::ValidatePanSmoothing(float value) const {
    return value >= 0.0f && value <= 1.0f;
}

bool ConfigurationManager::ValidateSpectralFluxThreshold(float value) const {
    return value >= 0.01f && value <= 0.2f;
}

bool ConfigurationManager::ValidateTempoChangeThreshold(float value) const {
    return value >= 0.1f && value <= 0.5f;
}

bool ConfigurationManager::ValidateBeatInductionWindow(float value) const {
    return value >= 0.05f && value <= 0.2f;
}

bool ConfigurationManager::ValidateOctaveErrorWeight(float value) const {
    return value >= 0.0f && value <= 1.0f;
}

bool ConfigurationManager::ValidateSpectralFluxDecayMultiplier(float value) const {
    return value >= 0.5f && value <= 5.0f;
}

bool ConfigurationManager::ValidateBandMinFreq(float value) const {
    return value >= 10.0f && value <= 500.0f;
}

bool ConfigurationManager::ValidateBandMaxFreq(float value) const {
    return value >= 2000.0f && value <= 22050.0f;
}

bool ConfigurationManager::ValidateBandLogStrength(float value) const {
    return value >= 0.2f && value <= 3.0f;
}

bool ConfigurationManager::ValidateEqualizerBand(int band, float value) const {
    return band >= 0 && band <= 4 && value >= 0.0f && value <= 4.0f;
}

bool ConfigurationManager::ValidateEqualizerWidth(float value) const {
    return value >= 0.05f && value <= 0.5f;
}

bool ConfigurationManager::ValidateBeatDetectionAlgorithm(int algorithm) const {
    return algorithm >= 0 && algorithm <= 1; // SimpleEnergy(0) or SpectralFluxAuto(1)
}

bool ConfigurationManager::ValidateAudioCaptureProvider(int provider) const {
    return provider >= 0 && provider <= 1; // SystemAudio(0) or ProcessAudio(1)
}

std::string ConfigurationManager::GetLastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

// Private helper methods
void ConfigurationManager::NotifyListeners(const std::string& settingName, 
                                         const ConfigValue& oldValue, 
                                         const ConfigValue& newValue) {
    // Note: Mutex should already be locked by caller
    for (auto* listener : m_listeners) {
        if (listener) {
            try {
                listener->OnConfigurationChanged(settingName, oldValue.ToString(), newValue.ToString());
            } catch (const std::exception& e) {
                LOG_ERROR("[ConfigurationManager] Exception in change listener: " + std::string(e.what()));
            }
        }
    }
}

template<typename T>
bool ConfigurationManager::SetValue(T& target, T newValue, const std::string& settingName, 
                                   std::function<bool(T)> validator) {
    if (validator && !validator(newValue)) {
        m_lastError = "Validation failed for setting: " + settingName;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    if (target != newValue) {
        T oldValue = target;
        target = newValue;
        NotifyListeners(settingName, ConfigValue(oldValue), ConfigValue(newValue));
        return true;
    }
    return false;
}

// File I/O helpers (delegating to existing settings functions)
bool ConfigurationManager::LoadSettingsFromFile() {
    // Delegate to existing LoadAllTunables function
    extern void LoadAllTunables();
    extern ListeningwaySettings g_settings;
    
    try {
        LoadAllTunables();
        m_settings = g_settings; // Copy loaded settings
        return true;
    } catch (const std::exception& e) {
        m_lastError = "Failed to load settings: " + std::string(e.what());
        LOG_ERROR("[ConfigurationManager] " + m_lastError);
        return false;
    }
}

bool ConfigurationManager::SaveSettingsToFile() {
    // Update global settings and delegate to existing SaveAllTunables function
    extern void SaveAllTunables();
    extern ListeningwaySettings g_settings;
    
    try {
        g_settings = m_settings; // Copy current settings to global
        SaveAllTunables();
        return true;
    } catch (const std::exception& e) {
        m_lastError = "Failed to save settings: " + std::string(e.what());
        LOG_ERROR("[ConfigurationManager] " + m_lastError);
        return false;
    }
}

// Business logic integration methods
void ConfigurationManager::OnAudioAnalysisEnabledChanged(bool enabled) {
    // Delegate to existing SetAudioAnalysisEnabled function for business logic
    extern void SetAudioAnalysisEnabled(bool enabled);
    SetAudioAnalysisEnabled(enabled);
}

void ConfigurationManager::OnDebugEnabledChanged(bool enabled) {
    // Delegate to existing SetDebugEnabled function for business logic
    extern void SetDebugEnabled(bool enabled);
    SetDebugEnabled(enabled);
}

void ConfigurationManager::OnBeatDetectionAlgorithmChanged(int algorithm) {
    // Update audio analyzer if audio analysis is enabled
    if (m_settings.audio_analysis_enabled.load()) {
        // Access audio analyzer and update algorithm
        extern class AudioAnalyzer g_audio_analyzer;
        g_audio_analyzer.SetBeatDetectionAlgorithm(algorithm);
        LOG_DEBUG("[ConfigurationManager] Beat detection algorithm changed to: " + std::to_string(algorithm));
    }
}

void ConfigurationManager::OnAudioCaptureProviderChanged(int provider) {
    // Delegate to existing audio capture provider switching logic
    extern bool SwitchAudioProvider(int providerType, int timeout_ms);
    if (!SwitchAudioProvider(provider, 2000)) {
        LOG_ERROR("[ConfigurationManager] Failed to switch audio capture provider to: " + std::to_string(provider));
    }
}
