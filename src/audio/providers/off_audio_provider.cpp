// ---------------------------------------------
// Dummy (Off) Audio Provider Implementation
// Represents the 'None' or 'Off' selection in the provider dropdown
// ---------------------------------------------
#include "off_audio_provider.h"
#include "../../core/thread_safety_manager.h"
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

bool OffAudioCaptureProvider::IsAvailable() const { return true; }
bool OffAudioCaptureProvider::Initialize() { return true; }
void OffAudioCaptureProvider::Uninitialize() {}
bool OffAudioCaptureProvider::StartCapture(const Listeningway::Configuration& config, std::atomic_bool& running, std::thread& thread, AudioAnalysisData& data) {
    // Keep the thread running but provide dummy/zero data
    running = true;
    thread = std::thread([&, config]() {
        while (running.load()) {
            // Provide zero/dummy audio data
            {
                LOCK_AUDIO_DATA();
                data.volume = 0.0f;
                std::fill(data.freq_bands.begin(), data.freq_bands.end(), 0.0f);
                data.beat = 0.0f;
            }
            // Sleep to avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
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
        0, // order
        false // activates_capture
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
