// ---------------------------------------------
// Audio Capture Module Implementation
// Manages audio capture using different providers (System/Process)
// ---------------------------------------------
#include "audio_capture.h"
#include "audio_capture_manager.h"
#include "providers/audio_capture_provider.h"
#include "../utils/logging.h"
#include "settings.h"
#include <memory>

// Global audio capture manager instance
std::unique_ptr<AudioCaptureManager> g_audio_capture_manager;

// Initialize the audio capture manager and select provider based on settings
void InitAudioCapture() {
    if (!g_audio_capture_manager) {
        g_audio_capture_manager = std::make_unique<AudioCaptureManager>();
        g_audio_capture_manager->Initialize();
        
        // Set preferred provider from settings if specified
        if (!g_settings.audio_capture_provider_code.empty()) {
            g_audio_capture_manager->SetPreferredProviderByCode(g_settings.audio_capture_provider_code);
        }
        // If no provider code is set, let the manager use its default selection logic
    }
}

// Initialize audio capture - for external API compatibility
bool InitializeAudioCapture() {
    InitAudioCapture();
    return g_audio_capture_manager != nullptr;
}

void UninitAudioCapture() {
    if (g_audio_capture_manager) {
        g_audio_capture_manager->Uninitialize();
        g_audio_capture_manager.reset();
    }
}

// Starts a background thread that captures audio and updates analysis data using the selected provider.
void StartAudioCaptureThread(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data) {
    InitAudioCapture();
    if (g_audio_capture_manager) {
        g_audio_capture_manager->StartCapture(config, running, thread, data_mutex, data);
    }
}

// Signals the capture thread to stop and joins it using the selected provider.
void StopAudioCaptureThread(std::atomic_bool& running, std::thread& thread) {
    if (g_audio_capture_manager) {
        g_audio_capture_manager->StopCapture(running, thread);
    } else {
        running = false;
        if (thread.joinable()) thread.join();
    }
}

// Helper to restart audio capture if provider signals restart is needed
void CheckAndRestartAudioCapture(const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data) {
    if (g_audio_capture_manager) {
        g_audio_capture_manager->CheckAndRestartCapture(config, running, thread, data_mutex, data);
    }
}

// Overlay API: Set the preferred audio capture provider (0 = System, 1 = Process)
bool SetAudioCaptureProvider(int providerType) {
    if (!g_audio_capture_manager) {
        InitAudioCapture();
    }
    if (!g_audio_capture_manager) return false;
    if (providerType < 0) return false; // -1 is "None (off)"
    return g_audio_capture_manager->SetPreferredProvider(static_cast<AudioCaptureProviderType>(providerType));
}

// Overlay API: Get the current audio capture provider type (0 = System, 1 = Process)
int GetAudioCaptureProvider() {
    if (!g_audio_capture_manager) {
        InitAudioCapture();
    }
    if (!g_audio_capture_manager) return -1;
    return static_cast<int>(g_audio_capture_manager->GetCurrentProvider());
}

// Overlay API: Get available audio capture providers (vector of ints)
std::vector<AudioProviderInfo> GetAvailableAudioCaptureProviders() {
    if (!g_audio_capture_manager) {
        InitializeAudioCapture();
    }
    std::vector<AudioProviderInfo> result;
    if (g_audio_capture_manager) {
        result = g_audio_capture_manager->GetAvailableProviderInfos();
    }
    return result;
}

// Overlay API: Get the name of an audio capture provider by type
std::string GetAudioCaptureProviderName(int providerType) {
    if (!g_audio_capture_manager) {
        InitAudioCapture();
    }
    if (!g_audio_capture_manager) return "Unknown";
    return g_audio_capture_manager->GetProviderName(static_cast<AudioCaptureProviderType>(providerType));
}

// Overlay API: Get the name of an audio capture provider by code
std::string GetAudioCaptureProviderName(const std::string& providerCode) {
    if (!g_audio_capture_manager) {
        InitAudioCapture();
    }
    if (!g_audio_capture_manager) return "Unknown";
    for (const auto& info : g_audio_capture_manager->GetAvailableProviderInfos()) {
        if (info.code == providerCode) return info.name;
    }
    return "Unknown";
}

// Overlay API: Switch provider and restart capture thread if running
bool SwitchAudioCaptureProviderAndRestart(int providerType, const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data) {
    if (!g_audio_capture_manager) {
        InitAudioCapture();
    }
    if (!g_audio_capture_manager) return false;
    if (providerType < 0) return false; // -1 is "None (off)"
    return g_audio_capture_manager->SwitchProviderAndRestart(static_cast<AudioCaptureProviderType>(providerType), config, running, thread, data_mutex, data);
}

// Overlay API: Switch provider by code and restart capture thread if running
bool SwitchAudioCaptureProviderByCodeAndRestart(const std::string& providerCode, const AudioAnalysisConfig& config, std::atomic_bool& running, std::thread& thread, std::mutex& data_mutex, AudioAnalysisData& data) {
    if (!g_audio_capture_manager) {
        InitAudioCapture();
    }
    if (!g_audio_capture_manager) return false;
    return g_audio_capture_manager->SwitchProviderByCodeAndRestart(providerCode, config, running, thread, data_mutex, data);
}
