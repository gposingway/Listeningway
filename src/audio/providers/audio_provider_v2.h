#pragma once
#include "../core/result.h"
#include "../core/configuration/configuration_manager.h"
#include "audio_analysis.h"
#include "raii_wrappers.h"
#include <atomic>
#include <thread>
#include <mutex>

namespace Listeningway::Audio {

/**
 * @brief Modernized audio capture provider interface using Result<T, Error> pattern
 * Replaces inconsistent error handling across providers
 */
class IAudioCaptureProviderV2 {
public:
    virtual ~IAudioCaptureProviderV2() = default;
    
    /**
     * @brief Initialize the provider (setup COM, device enumeration, etc.)
     */
    virtual Result<void> Initialize() = 0;
    
    /**
     * @brief Uninitialize and cleanup resources
     */
    virtual void Uninitialize() = 0;
    
    /**
     * @brief Check if capture is available/possible
     */
    virtual Result<bool> IsAvailable() const = 0;
    
    /**
     * @brief Start audio capture with standardized error handling
     */
    virtual Result<void> StartCapture(const Configuration& config, 
                                     std::atomic_bool& running, 
                                     std::thread& thread, 
                                     std::mutex& data_mutex, 
                                     AudioAnalysisData& data) = 0;
    
    /**
     * @brief Stop capture with guaranteed cleanup
     */
    virtual Result<void> StopCapture(std::atomic_bool& running, std::thread& thread) = 0;
    
    /**
     * @brief Check if restart is needed (device changes, etc.)
     */
    virtual bool ShouldRestart() const = 0;
    
    /**
     * @brief Reset restart flags
     */
    virtual void ResetRestartFlags() = 0;
    
    /**
     * @brief Get provider information
     */
    virtual AudioProviderInfo GetInfo() const = 0;
    
protected:
    // Common error logging that respects the Result pattern
    void LogError(const Error& error) const;
    void LogSuccess(const std::string& operation) const;
};

/**
 * @brief Base implementation with common WASAPI error handling patterns
 */
class WasapiProviderBase : public IAudioCaptureProviderV2 {
public:
    Result<void> Initialize() override;
    void Uninitialize() override;
    Result<void> StopCapture(std::atomic_bool& running, std::thread& thread) override;
    
protected:
    ComPtr<IMMDeviceEnumerator> device_enumerator_;
    std::atomic_bool device_change_pending_{false};
    
    // Common WASAPI operations with standardized error handling
    Result<ComPtr<IMMDevice>> GetDefaultAudioDevice();
    Result<void> SetupDeviceNotifications();
    void CleanupDeviceNotifications();
    
    // Template method for capture thread - derived classes implement CaptureLoop
    void CaptureThreadTemplate(const Configuration& config,
                              std::atomic_bool& running,
                              std::mutex& data_mutex,
                              AudioAnalysisData& data);
    
    virtual Result<WasapiResourceManager> CreateCaptureResources(const Configuration& config) = 0;
    virtual Result<void> CaptureLoop(WasapiResourceManager& resources,
                                    const Configuration& config,
                                    std::atomic_bool& running,
                                    std::mutex& data_mutex,
                                    AudioAnalysisData& data) = 0;
    
private:
    class DeviceNotificationClient;
    std::unique_ptr<DeviceNotificationClient> notification_client_;
    
    Result<void> InitializeComRuntime();
    void CleanupComRuntime();
};

/**
 * @brief System audio provider with modernized error handling
 */
class SystemAudioProviderV2 : public WasapiProviderBase {
public:
    Result<bool> IsAvailable() const override;
    Result<void> StartCapture(const Configuration& config, 
                             std::atomic_bool& running, 
                             std::thread& thread, 
                             std::mutex& data_mutex, 
                             AudioAnalysisData& data) override;
    bool ShouldRestart() const override;
    void ResetRestartFlags() override;
    AudioProviderInfo GetInfo() const override;
    
protected:
    Result<WasapiResourceManager> CreateCaptureResources(const Configuration& config) override;
    Result<void> CaptureLoop(WasapiResourceManager& resources,
                            const Configuration& config,
                            std::atomic_bool& running,
                            std::mutex& data_mutex,
                            AudioAnalysisData& data) override;
};

/**
 * @brief Process audio provider with modernized error handling
 */
class ProcessAudioProviderV2 : public WasapiProviderBase {
public:
    Result<bool> IsAvailable() const override;
    Result<void> StartCapture(const Configuration& config, 
                             std::atomic_bool& running, 
                             std::thread& thread, 
                             std::mutex& data_mutex, 
                             AudioAnalysisData& data) override;
    bool ShouldRestart() const override;
    void ResetRestartFlags() override;
    AudioProviderInfo GetInfo() const override;
    
protected:
    Result<WasapiResourceManager> CreateCaptureResources(const Configuration& config) override;
    Result<void> CaptureLoop(WasapiResourceManager& resources,
                            const Configuration& config,
                            std::atomic_bool& running,
                            std::mutex& data_mutex,
                            AudioAnalysisData& data) override;
    
private:
    DWORD target_process_id_ = 0;
    Result<ComPtr<IAudioSessionControl>> FindProcessAudioSession(DWORD pid = 0);
};

} // namespace Listeningway::Audio
