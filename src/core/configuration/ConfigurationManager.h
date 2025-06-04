#pragma once
#include "ConfigValue.h"
#include "IConfigurationChangeListener.h"
#include "../settings.h"
#include <vector>
#include <mutex>
#include <memory>

/**
 * @brief Centralized configuration manager that provides controlled access to settings
 * 
 * This class addresses separation of concerns by:
 * - Encapsulating direct settings access
 * - Providing validation for configuration changes
 * - Implementing change notification system
 * - Ensuring thread-safe access to configuration
 * - Abstracting persistence operations
 */
class ConfigurationManager {
public:
    static ConfigurationManager& Instance();
    
    // Disable copy constructor and assignment operator
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;
    
    // Configuration access (read-only interface)
    float GetPanSmoothing() const;
    float GetSpectralFluxThreshold() const;
    float GetTempoChangeThreshold() const;
    float GetBeatInductionWindow() const;
    float GetOctaveErrorWeight() const;
    float GetSpectralFluxDecayMultiplier() const;
    bool IsLogScaleEnabled() const;
    float GetBandMinFreq() const;
    float GetBandMaxFreq() const;
    float GetBandLogStrength() const;
    float GetEqualizerBand(int band) const; // band 0-4
    float GetEqualizerWidth() const;
    int GetBeatDetectionAlgorithm() const;
    int GetAudioCaptureProvider() const;
    bool IsAudioAnalysisEnabled() const;
    bool IsDebugEnabled() const;
    
    // Get all settings as read-only reference (for cases where full struct access is needed)
    const ListeningwaySettings& GetAllSettings() const;
    
    // Configuration modification (controlled interface with validation)
    bool SetPanSmoothing(float value);
    bool SetSpectralFluxThreshold(float value);
    bool SetTempoChangeThreshold(float value);
    bool SetBeatInductionWindow(float value);
    bool SetOctaveErrorWeight(float value);
    bool SetSpectralFluxDecayMultiplier(float value);
    bool SetLogScaleEnabled(bool enabled);
    bool SetBandMinFreq(float value);
    bool SetBandMaxFreq(float value);
    bool SetBandLogStrength(float value);
    bool SetEqualizerBand(int band, float value); // band 0-4
    bool SetEqualizerWidth(float value);
    bool SetBeatDetectionAlgorithm(int algorithm);
    bool SetAudioCaptureProvider(int provider);
    bool SetAudioAnalysisEnabled(bool enabled);
    bool SetDebugEnabled(bool enabled);
    
    // UI helper methods for ImGui integration (controlled access)
    // These return pointers for ImGui but changes are still validated
    float* GetPanSmoothingRef();
    float* GetSpectralFluxThresholdRef();
    float* GetTempoChangeThresholdRef();
    float* GetBeatInductionWindowRef();
    float* GetOctaveErrorWeightRef();
    float* GetSpectralFluxDecayMultiplierRef();
    bool* GetLogScaleEnabledRef();
    float* GetBandMinFreqRef();
    float* GetBandMaxFreqRef();
    float* GetBandLogStrengthRef();
    float* GetEqualizerBandRef(int band); // band 0-4
    float* GetEqualizerWidthRef();
    int* GetBeatDetectionAlgorithmRef();
    
    // Bulk operations
    bool LoadFromFile();
    bool SaveToFile();
    bool ResetToDefaults();
    
    // Change notification system
    void AddChangeListener(IConfigurationChangeListener* listener);
    void RemoveChangeListener(IConfigurationChangeListener* listener);
    
    // Validation
    bool ValidatePanSmoothing(float value) const;
    bool ValidateSpectralFluxThreshold(float value) const;
    bool ValidateTempoChangeThreshold(float value) const;
    bool ValidateBeatInductionWindow(float value) const;
    bool ValidateOctaveErrorWeight(float value) const;
    bool ValidateSpectralFluxDecayMultiplier(float value) const;
    bool ValidateBandMinFreq(float value) const;
    bool ValidateBandMaxFreq(float value) const;
    bool ValidateBandLogStrength(float value) const;
    bool ValidateEqualizerBand(int band, float value) const;
    bool ValidateEqualizerWidth(float value) const;
    bool ValidateBeatDetectionAlgorithm(int algorithm) const;
    bool ValidateAudioCaptureProvider(int provider) const;
    
    // Error handling
    std::string GetLastError() const;
    
private:
    ConfigurationManager();
    ~ConfigurationManager() = default;
    
    // Internal state
    mutable std::mutex m_mutex;
    ListeningwaySettings m_settings;
    std::vector<IConfigurationChangeListener*> m_listeners;
    std::string m_lastError;
    
    // Change notification helpers
    void NotifyListeners(const std::string& settingName, const ConfigValue& oldValue, const ConfigValue& newValue);
    
    // Internal validation and setting helpers
    template<typename T>
    bool SetValue(T& target, T newValue, const std::string& settingName, 
                  std::function<bool(T)> validator = nullptr);
    
    // Reference management for UI (tracks changes)
    void OnReferenceChanged(const std::string& settingName);
    
    // File I/O helpers
    bool LoadSettingsFromFile();
    bool SaveSettingsToFile();
    
    // Business logic integration (for settings that affect other systems)
    void OnAudioAnalysisEnabledChanged(bool enabled);
    void OnDebugEnabledChanged(bool enabled);
    void OnBeatDetectionAlgorithmChanged(int algorithm);
    void OnAudioCaptureProviderChanged(int provider);
};
