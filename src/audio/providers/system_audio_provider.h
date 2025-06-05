#pragma once
#include "audio_capture_provider.h"
#include <mmdeviceapi.h>
#include <atomic>

// System-wide audio capture using WASAPI loopback
class SystemAudioCaptureProvider : public IAudioCaptureProvider {
private:
    class DeviceNotificationClient;

    static std::atomic_bool device_change_pending_;
    static IMMDeviceEnumerator* device_enumerator_;
    static DeviceNotificationClient* notification_client_;

public:
    SystemAudioCaptureProvider() = default;
    ~SystemAudioCaptureProvider() override = default;

    AudioProviderInfo GetProviderInfo() const override;

    AudioCaptureProviderType GetProviderType() const override {
        return AudioCaptureProviderType::SYSTEM_AUDIO;
    }

    std::string GetProviderName() const override {
        return "System Audio (WASAPI Loopback)";
    }    bool IsAvailable() const override;
    
    bool StartCapture(const Listeningway::Configuration& config, 
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
