#include <algorithm> // Required for std::min, std::copy

// --- ImGui Integration ---
// Define necessary macros before including imgui.h when using ReShade's ImGui instance
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#define ImTextureID ImU64 // Use ReShade's resource view handle as texture ID
#include <imgui.h> // <<< Include imgui.h BEFORE reshade.hpp
// --- End ImGui Integration ---

#include <reshade.hpp> // <<< Include reshade.hpp AFTER imgui.h
#include <windows.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream> // For logging
#include <string>  // For logging
#include <chrono>  // For sleep and logging timestamp
#include <cmath>   // For audio analysis later
#include <combaseapi.h> // For COM
#include <mmdeviceapi.h> // For WASAPI
#include <audioclient.h> // For WASAPI

#include "audio_capture.h"
#include "audio_analysis.h"
#include "overlay.h"
#include "logging.h"

// --- Global State ---
static std::atomic_bool g_addon_enabled = false;
static std::atomic_bool g_audio_thread_running = false;
static std::thread g_audio_thread;
static std::mutex g_audio_data_mutex;
static AudioAnalysisData g_audio_data;
static AudioAnalysisConfig g_audio_config = { 16, 512 };
static reshade::api::effect_uniform_variable g_freq_bands_uniform = { 0 }; // Add global handle for the uniform
static reshade::api::effect_uniform_variable g_volume_uniform = { 0 }; // Add global handle for the volume uniform

// --- Uniform Update Callback ---
static void UpdateShaderUniforms(reshade::api::effect_runtime* runtime) {
    if (g_freq_bands_uniform == 0) {
        g_freq_bands_uniform = runtime->find_uniform_variable("Listeningway.fx", "fListeningwayFreqBands");
        if (g_freq_bands_uniform == 0) {
            LogToFile("UpdateShaderUniforms: Failed to find uniform 'fListeningwayFreqBands'");
            return;
        }
    }
    if (g_volume_uniform == 0) {
        g_volume_uniform = runtime->find_uniform_variable("Listeningway.fx", "fListeningwayVolume");
        if (g_volume_uniform == 0) {
            LogToFile("UpdateShaderUniforms: Failed to find uniform 'fListeningwayVolume'");
            // Don't return, bands can still be set
        }
    }
    std::lock_guard<std::mutex> lock(g_audio_data_mutex);
    if (!g_audio_data.freq_bands.empty()) {
        runtime->set_uniform_value_float(g_freq_bands_uniform, g_audio_data.freq_bands.data(), g_audio_data.freq_bands.size());
    }
    // Always set volume if handle is valid
    if (g_volume_uniform != 0) {
        float volume = g_audio_data.volume;
        runtime->set_uniform_value_float(g_volume_uniform, &volume, 1);
    }
}

// --- Overlay callback ---
static void OverlayCallback(reshade::api::effect_runtime*) {
    DrawListeningwayDebugOverlay(g_audio_data, g_audio_data_mutex);
}

// --- DllMain and registration ---
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    UNREFERENCED_PARAMETER(lpReserved);
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            if (reshade::register_addon(hModule)) {
                OpenLogFile("listeningway.log");
                LogToFile("Addon loaded and log file opened.");
                reshade::register_overlay(nullptr, &OverlayCallback);
                // Register the function using the correct event enum
                reshade::register_event<reshade::addon_event::reshade_begin_effects>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_begin_effects>::decl)UpdateShaderUniforms);
                StartAudioCaptureThread(g_audio_config, g_audio_thread_running, g_audio_thread, g_audio_data_mutex, g_audio_data);
                g_addon_enabled = true;
            }
            break;
        case DLL_PROCESS_DETACH:
            if (g_addon_enabled.load()) {
                reshade::unregister_overlay(nullptr, &OverlayCallback);
                // Unregister the function using the correct event enum
                reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_begin_effects>::decl)UpdateShaderUniforms);
                StopAudioCaptureThread(g_audio_thread_running, g_audio_thread);
                CloseLogFile();
                g_addon_enabled = false;
                g_freq_bands_uniform = { 0 }; // Reset uniform handle
            }
            break;
    }
    return TRUE;
}

