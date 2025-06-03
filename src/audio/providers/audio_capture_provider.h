// ---------------------------------------------
// Audio Capture Provider Interface
// Abstract base class for different audio capture implementations
// ---------------------------------------------
#pragma once
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include "../audio_analysis.h"

/**
 * @brief Audio capture provider types
 */
enum class AudioCaptureProviderType {
    SYSTEM_AUDIO,    // System-wide audio capture (WASAPI loopback)
    PROCESS_AUDIO    // Process-specific audio capture
};

/**
 * @brief Abstract base class for audio capture providers
 */
class IAudioCaptureProvider {
public:
    virtual ~IAudioCaptureProvider() = default;

    /**
     * @brief Gets the provider type
     * @return The provider type
     */
    virtual AudioCaptureProviderType GetProviderType() const = 0;

    /**
     * @brief Gets the provider name
     * @return Human-readable provider name
     */
    virtual std::string GetProviderName() const = 0;

    /**
     * @brief Checks if this provider is available on the current system
     * @return true if the provider can be used
     */
    virtual bool IsAvailable() const = 0;

    /**
     * @brief Starts the audio capture thread
     * @param config Analysis configuration
     * @param running Atomic flag to control thread lifetime
     * @param thread Thread object (will be started)
     * @param data_mutex Mutex protecting the analysis data
     * @param data Analysis data to be updated by the thread
     * @return true if capture started successfully
     */
    virtual bool StartCapture(const AudioAnalysisConfig& config, 
                             std::atomic_bool& running, 
                             std::thread& thread, 
                             std::mutex& data_mutex, 
                             AudioAnalysisData& data) = 0;

    /**
     * @brief Stops the audio capture thread
     * @param running Atomic flag to signal thread to stop
     * @param thread Thread object (will be joined)
     */
    virtual void StopCapture(std::atomic_bool& running, std::thread& thread) = 0;

    /**
     * @brief Checks if capture needs to be restarted (e.g., device changes)
     * @return true if restart is needed
     */
    virtual bool ShouldRestart() = 0;

    /**
     * @brief Resets any restart flags
     */
    virtual void ResetRestartFlags() = 0;

    /**
     * @brief Initializes the provider
     * @return true if initialization succeeded
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Uninitializes the provider
     */
    virtual void Uninitialize() = 0;
};
