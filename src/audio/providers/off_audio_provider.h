#pragma once
#include "../audio_capture.h"
#include <string>

// Dummy provider for 'None (Audio Analysis Off)'
class OffAudioCaptureProvider : public IAudioCaptureProvider {
public:
    bool IsAvailable() const override;
    bool Initialize() override;
    void Uninitialize() override;
    bool StartCapture(const AudioAnalysisConfig&, std::atomic_bool& running, std::thread& thread, std::mutex&, AudioAnalysisData&) override;
    void StopCapture(std::atomic_bool& running, std::thread& thread) override;
    AudioProviderInfo GetProviderInfo() const override;
    AudioCaptureProviderType GetProviderType() const override;
    std::string GetProviderName() const override;
    bool ShouldRestart() override;
    void ResetRestartFlags() override;
};

extern "C" IAudioCaptureProvider* CreateOffAudioCaptureProvider();
