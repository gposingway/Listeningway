#pragma once

#include "Configuration.h"
#include "IConfigurationChangeListener.h"
#include <memory>
#include <mutex>
#include <vector>

/**
 * @brief Modern configuration manager with direct object access
 * 
 * Provides centralized access to application configuration with:
 * - Thread-safe operations using mutex
 * - Direct access to configuration object
 * - Change notification system for listeners
 * - File persistence (save/load)
 * - Automatic validation
 */
class ConfigurationManager {
public:
    static ConfigurationManager& Instance();
    
    // Disable copy constructor and assignment operator
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;
    
    // Direct access to configuration object (thread-safe)
    const Listeningway::Configuration& GetConfig() const;
    Listeningway::Configuration& GetConfig();
    
    // Notify listeners when configuration changes
    void NotifyConfigurationChanged();
    
    // File operations (delegates to Configuration object)
    bool Save(const std::string& filename = "") const;
    bool Load(const std::string& filename = "");
    void ResetToDefaults();
    
    // Change notification
    void AddChangeListener(std::shared_ptr<IConfigurationChangeListener> listener);
    void RemoveChangeListener(std::shared_ptr<IConfigurationChangeListener> listener);
    
    // Legacy compatibility methods (for gradual migration)
    // These delegate to the configuration object properties
    bool IsAudioAnalysisEnabled() const { return GetConfig().audio.analysisEnabled; }
    bool SetAudioAnalysisEnabled(bool enabled) { 
        GetConfig().audio.analysisEnabled = enabled; 
        NotifyConfigurationChanged(); 
        return true; 
    }
    
    // New provider code string API
    std::string GetAudioCaptureProviderCode() const { return GetConfig().audio.captureProviderCode; }
    bool SetAudioCaptureProviderCode(const std::string& code) {
        GetConfig().audio.captureProviderCode = code;
        NotifyConfigurationChanged();
        return true;
    }
    // Legacy int-based API (commented out)
    /*
    int GetAudioCaptureProvider() const { return GetConfig().audio.captureProvider; }
    bool SetAudioCaptureProvider(int provider) { 
        GetConfig().audio.captureProvider = provider; 
        NotifyConfigurationChanged(); 
        return true; 
    }
    */
    
    float GetPanSmoothing() const { return GetConfig().audio.panSmoothing; }
    bool SetPanSmoothing(float value) { 
        GetConfig().audio.panSmoothing = value; 
        GetConfig().Validate(); 
        NotifyConfigurationChanged(); 
        return true; 
    }
    
    int GetBeatDetectionAlgorithm() const { return GetConfig().beat.algorithm; }
    bool SetBeatDetectionAlgorithm(int algorithm) { 
        GetConfig().beat.algorithm = algorithm; 
        GetConfig().Validate(); 
        NotifyConfigurationChanged(); 
        return true; 
    }
    
    bool IsLogScaleEnabled() const { return GetConfig().frequency.logScaleEnabled; }
    bool SetLogScaleEnabled(bool enabled) { 
        GetConfig().frequency.logScaleEnabled = enabled; 
        NotifyConfigurationChanged(); 
        return true; 
    }

private:
    ConfigurationManager();
    ~ConfigurationManager() = default;
    
    mutable std::mutex m_mutex;
    mutable Listeningway::Configuration m_config;
    std::vector<std::weak_ptr<IConfigurationChangeListener>> m_listeners;
    
    void NotifyListeners();
};