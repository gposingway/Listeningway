// ---------------------------------------------
// Process Audio Capture Provider
// Captures audio from the game process using WASAPI Session Management
// ---------------------------------------------
#pragma once
#include "audio_capture_provider.h"
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <atomic>

/**
 * @brief Game process-specific audio capture provider using WASAPI Session Management
 */
class ProcessAudioCaptureProvider : public IAudioCaptureProvider {
private:
    static std::atomic_bool device_change_pending_;
    static IMMDeviceEnumerator* device_enumerator_;
    class DeviceNotificationClient;
    static DeviceNotificationClient* notification_client_;
    
    DWORD game_process_id_;
    
    /**
     * @brief Finds the audio session for the game process
     * @param ppSessionControl2 Output session control for the game process
     * @param pVolume Output volume level of the game session
     * @return true if game audio session was found
     */
    bool FindGameAudioSession(IAudioSessionControl2** ppSessionControl2, float* pVolume);

public:
    ProcessAudioCaptureProvider();
    ~ProcessAudioCaptureProvider() override = default;

    AudioProviderInfo GetProviderInfo() const override;

    // IAudioCaptureProvider implementation
    AudioCaptureProviderType GetProviderType() const override {
        return AudioCaptureProviderType::PROCESS_AUDIO;
    }

    std::string GetProviderName() const override {
        return "Game Audio (Process-Aware)";
    }    bool IsAvailable() const override;
    
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
