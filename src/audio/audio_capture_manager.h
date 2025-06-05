#pragma once
#include "providers/audio_capture_provider.h"
#include "audio_analysis.h"
#include "configuration/configuration_manager.h"
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

// Manages audio capture providers and provider selection
class AudioCaptureManager {
private:
    std::vector<std::unique_ptr<IAudioCaptureProvider>> providers_;
    IAudioCaptureProvider* current_provider_;
    AudioCaptureProviderType preferred_provider_type_;
    
    bool initialized_;

public:
    AudioCaptureManager();
    ~AudioCaptureManager();

    // Initialize manager and register providers
    bool Initialize();

    /**
     * @brief Uninitializes the audio capture manager
     */
    void Uninitialize();

    /**
     * @brief Gets all available provider types
     * @return Vector of available provider types
     */
    std::vector<AudioCaptureProviderType> GetAvailableProviders() const;

    /**
     * @brief Gets the name of a provider type
     * @param type Provider type
     * @return Human-readable provider name
     */
    std::string GetProviderName(AudioCaptureProviderType type) const;

    /**
     * @brief Sets the preferred provider type
     * @param type Provider type to prefer
     * @return true if the provider is available and was set successfully
     */
    bool SetPreferredProvider(AudioCaptureProviderType type);

    /**
     * @brief Sets the preferred provider by code string
     * @param providerCode Provider code string (e.g., "system", "game", "off")
     * @return true if the provider is available and was set successfully
     */
    bool SetPreferredProviderByCode(const std::string& providerCode);

    /**
     * @brief Gets the current preferred provider type
     * @return Current preferred provider type
     */
    AudioCaptureProviderType GetPreferredProvider() const {
        return preferred_provider_type_;
    }

    /**
     * @brief Gets the currently active provider type
     * @return Currently active provider type, or SYSTEM_AUDIO if none active
     */
    AudioCaptureProviderType GetCurrentProvider() const;

    /**
     * @brief Starts audio capture using the preferred provider
     * @param config Analysis configuration
     * @param running Atomic flag to control thread lifetime
     * @param thread Thread object (will be started)
     * @param data_mutex Mutex protecting the analysis data
     * @param data Analysis data to be updated by the thread
     * @return true if capture started successfully
     */
    bool StartCapture(const Listeningway::Configuration& config, 
                     std::atomic_bool& running, 
                     std::thread& thread, 
                     std::mutex& data_mutex, 
                     AudioAnalysisData& data);

    /**
     * @brief Stops audio capture
     * @param running Atomic flag to signal thread to stop
     * @param thread Thread object (will be joined)
     */
    void StopCapture(std::atomic_bool& running, std::thread& thread);

    /**
     * @brief Checks if capture needs to be restarted and restarts if necessary
     * @param config Analysis configuration
     * @param running Atomic flag to control thread lifetime
     * @param thread Thread object (will be restarted if needed)
     * @param data_mutex Mutex protecting the analysis data
     * @param data Analysis data to be updated by the thread
     */
    void CheckAndRestartCapture(const Listeningway::Configuration& config, 
                               std::atomic_bool& running, 
                               std::thread& thread, 
                               std::mutex& data_mutex, 
                               AudioAnalysisData& data);

    /**
     * @brief Switches provider and restarts capture thread if running
     * @param type Provider type to switch to
     * @param config Analysis configuration
     * @param running Atomic flag to control thread lifetime
     * @param thread Thread object (will be stopped/restarted)
     * @param data_mutex Mutex protecting the analysis data
     * @param data Analysis data to be updated by the thread
     * @return true if switch and restart succeeded
     */
    bool SwitchProviderAndRestart(AudioCaptureProviderType type, const Listeningway::Configuration& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);

    /**
     * @brief Switches provider by code and restarts capture thread if running
     * @param providerCode Provider code string (e.g., "off", "system", "game")
     * @param config Analysis configuration
     * @param running Atomic flag to control thread lifetime
     * @param thread Thread object (will be stopped/restarted)
     * @param data_mutex Mutex protecting the analysis data
     * @param data Analysis data to be updated by the thread
     * @return true if switch and restart succeeded
     */
    bool SwitchProviderByCodeAndRestart(const std::string& providerCode, const Listeningway::Configuration& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data);

    /**
     * @brief Gets all available provider infos (for UI and config)
     * @return Vector of AudioProviderInfo for available providers
     */
    std::vector<AudioProviderInfo> GetAvailableProviderInfos() const;

private:
    /**
     * @brief Registers all available providers
     */
    void RegisterProviders();

    /**
     * @brief Finds a provider by type
     * @param type Provider type to find
     * @return Pointer to provider or nullptr if not found
     */
    IAudioCaptureProvider* FindProvider(AudioCaptureProviderType type) const;

    /**
     * @brief Selects the best available provider based on preferences
     * @return Pointer to selected provider or nullptr if none available
     */
    IAudioCaptureProvider* SelectBestProvider();
};
