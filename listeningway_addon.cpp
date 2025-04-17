#include <algorithm> // Required for std::min, std::copy
#include <unordered_map>
#include <string_view>

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

#define LISTENINGWAY_NUM_BANDS 8

// --- Global State ---
static std::atomic_bool g_addon_enabled = false;
static std::atomic_bool g_audio_thread_running = false;
static std::thread g_audio_thread;
static std::mutex g_audio_data_mutex;
static AudioAnalysisData g_audio_data;
static AudioAnalysisConfig g_audio_config = { LISTENINGWAY_NUM_BANDS, 512 };

// --- Uniform variable cache ---
static std::vector<reshade::api::effect_uniform_variable> g_volume_uniforms;
static std::vector<reshade::api::effect_uniform_variable> g_freq_bands_uniforms;

// --- Uniform Update Callback ---
static void UpdateShaderUniforms(reshade::api::effect_runtime* runtime) {
    float volume_to_set;
    std::vector<float> freq_bands_to_set;
    {
        std::lock_guard<std::mutex> lock(g_audio_data_mutex);
        volume_to_set = g_audio_data.volume;
        freq_bands_to_set = g_audio_data.freq_bands;
    }
    // Cache miss: build the cache if empty
    if (g_volume_uniforms.empty() || g_freq_bands_uniforms.empty()) {
        runtime->enumerate_uniform_variables(nullptr, [&](reshade::api::effect_runtime*, reshade::api::effect_uniform_variable var_handle) {
            char name[256] = {};
            runtime->get_uniform_variable_name(var_handle, name);
            if (std::string_view(name) == "Listeningway_Volume") {
                g_volume_uniforms.push_back(var_handle);
            } else if (std::string_view(name) == "Listeningway_FreqBands") {
                g_freq_bands_uniforms.push_back(var_handle);
            }
        });
    }
    // Set the uniform for all cached handles
    for (auto var_handle : g_volume_uniforms) {
        runtime->set_uniform_value_float(var_handle, &volume_to_set, 1);
    }
    for (auto var_handle : g_freq_bands_uniforms) {
        if (!freq_bands_to_set.empty())
            runtime->set_uniform_value_float(var_handle, freq_bands_to_set.data(), static_cast<uint32_t>(freq_bands_to_set.size()));
    }
}

// --- Effect reload event: clear cache ---
static void OnReloadedEffects(reshade::api::effect_runtime*) {
    g_volume_uniforms.clear();
    g_freq_bands_uniforms.clear();
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
                reshade::register_event<reshade::addon_event::reshade_reloaded_effects>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_reloaded_effects>::decl)OnReloadedEffects);
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
                reshade::unregister_event<reshade::addon_event::reshade_reloaded_effects>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_reloaded_effects>::decl)OnReloadedEffects);
                StopAudioCaptureThread(g_audio_thread_running, g_audio_thread);
                CloseLogFile();
                g_addon_enabled = false;
            }
            break;
    }
    return TRUE;
}

