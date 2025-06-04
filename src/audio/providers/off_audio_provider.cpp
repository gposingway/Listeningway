// ---------------------------------------------
// Dummy (Off) Audio Provider Implementation
// Represents the 'None' or 'Off' selection in the provider dropdown
// ---------------------------------------------
#include "off_audio_provider.h"
#include <string>

bool OffAudioCaptureProvider::IsAvailable() const { return true; }
bool OffAudioCaptureProvider::Initialize() { return true; }
void OffAudioCaptureProvider::Uninitialize() {}
bool OffAudioCaptureProvider::StartCapture(const AudioAnalysisConfig&, std::atomic_bool& running, std::thread& thread, std::mutex&, AudioAnalysisData&) {
    running = false;
    return true;
}
void OffAudioCaptureProvider::StopCapture(std::atomic_bool& running, std::thread& thread) {
    running = false;
    if (thread.joinable()) thread.join();
}
AudioProviderInfo OffAudioCaptureProvider::GetProviderInfo() const {
    return AudioProviderInfo{
        "off", // code as string
        "None (Audio Analysis Off)", // name
        false, // is_default
        0 // order
    };
}
AudioCaptureProviderType OffAudioCaptureProvider::GetProviderType() const { return AudioCaptureProviderType::SYSTEM_AUDIO; } // or a new OFF type
std::string OffAudioCaptureProvider::GetProviderName() const { return "None (Audio Analysis Off)"; }
bool OffAudioCaptureProvider::ShouldRestart() { return false; }
void OffAudioCaptureProvider::ResetRestartFlags() {}

// Factory function for registration
extern "C" IAudioCaptureProvider* CreateOffAudioCaptureProvider() {
    return new OffAudioCaptureProvider();
}
