// ---------------------------------------------
// Audio Capture Manager
// Manages audio capture providers and handles provider selection
// ---------------------------------------------
#pragma once
#include "providers/audio_capture_provider.h"
#include "audio_analysis.h"
#include <memory>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>

/**
 * @brief Audio capture manager that handles provider selection and management
 */
class AudioCaptureManager {
private:
    std::vector<std::unique_ptr<IAudioCaptureProvider>> providers_;
    std::unique_ptr<IAudioCaptureProvider> current_provider_;
    AudioCaptureProviderType preferred_provider_type_;
    
    bool initialized_;

public:
    AudioCaptureManager();
    ~AudioCaptureManager();

    /**
     * @brief Initializes the audio capture manager and registers providers
     * @return true if initialization succeeded
     */
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
    bool StartCapture(const AudioAnalysisConfig& config, 
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
    void CheckAndRestartCapture(const AudioAnalysisConfig& config, 
                               std::atomic_bool& running, 
                               std::thread& thread, 
                               std::mutex& data_mutex, 
                               AudioAnalysisData& data);

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
