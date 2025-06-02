// ---------------------------------------------
// Process Audio Capture Provider
// Captures audio only from the current process using WASAPI Session Management
// ---------------------------------------------
#pragma once
#include "audio_capture_provider.h"
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <atomic>

/**
 * @brief Process-specific audio capture provider using WASAPI Session Management
 */
class ProcessAudioCaptureProvider : public IAudioCaptureProvider {
private:
    static std::atomic_bool device_change_pending_;
    static IMMDeviceEnumerator* device_enumerator_;
    class DeviceNotificationClient;
    static DeviceNotificationClient* notification_client_;
    
    DWORD current_process_id_;
    
    /**
     * @brief Finds the audio session for the current process
     * @param pAudioClient Output audio client for the process session
     * @return true if process audio session was found
     */
    bool FindProcessAudioSession(IAudioClient** pAudioClient);

public:
    ProcessAudioCaptureProvider();
    ~ProcessAudioCaptureProvider() override = default;

    // IAudioCaptureProvider implementation
    AudioCaptureProviderType GetProviderType() const override {
        return AudioCaptureProviderType::PROCESS_AUDIO;
    }

    std::string GetProviderName() const override {
        return "Process Audio (WASAPI Session)";
    }

    bool IsAvailable() const override;
    
    bool StartCapture(const AudioAnalysisConfig& config, 
                     std::atomic_bool& running, 
                     std::thread& thread, 
                     std::mutex& data_mutex, 
                     AudioAnalysisData& data) override;

    void StopCapture(std::atomic_bool& running, std::thread& thread) override;

    bool ShouldRestart() override {
        return device_change_pending_.load();
    }

    void ResetRestartFlags() override {
        device_change_pending_ = false;
    }

    bool Initialize() override;
    void Uninitialize() override;

    // Static method for device change notification
    static void SetDeviceChangePending() {
        device_change_pending_ = true;
    }
};
