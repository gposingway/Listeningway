// ---------------------------------------------
// @file listeningway_addon.cpp
// @brief Main entry point for the Listeningway ReShade addon
// ---------------------------------------------
#include <algorithm>
#include <unordered_map>
#include <string_view>
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#define ImTextureID ImU64
#include <imgui.h>
#include <reshade.hpp>
#include <windows.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <fstream>
#include <string>
#include <chrono>
#include <cmath>
#include <combaseapi.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include "audio_capture.h"
#include "audio_analysis.h"
#include "overlay.h"
#include "logging.h"
#include "uniform_manager.h"

#define LISTENINGWAY_NUM_BANDS 8
static std::atomic_bool g_addon_enabled = false;
static std::atomic_bool g_audio_thread_running = false;
static std::atomic_bool g_effects_enabled = false;
static std::wstring g_current_preset_path;
static std::thread g_audio_thread;
static std::mutex g_audio_data_mutex;
static AudioAnalysisData g_audio_data;
static AudioAnalysisConfig g_audio_config = { LISTENINGWAY_NUM_BANDS, 512 };
static UniformManager g_uniform_manager;

/**
 * @brief Updates all Listeningway_* uniforms in all loaded effects.
 * @param runtime The ReShade effect runtime.
 */
static void UpdateShaderUniforms(reshade::api::effect_runtime* runtime) {
    float volume_to_set;
    std::vector<float> freq_bands_to_set;
    float beat_to_set;
    {
        std::lock_guard<std::mutex> lock(g_audio_data_mutex);
        volume_to_set = g_audio_data.volume;
        freq_bands_to_set = g_audio_data.freq_bands;
        beat_to_set = g_audio_data.beat;
    }
    g_uniform_manager.update_uniforms(runtime, volume_to_set, freq_bands_to_set, beat_to_set);
}

/**
 * @brief Helper: Called when ReShade global effects state changes.
 * @param runtime The ReShade effect runtime.
 * @param enabled Whether the global effects state is enabled.
 */
static void OnSetEffectsState(reshade::api::effect_runtime* runtime, bool enabled) {
    g_effects_enabled = enabled;
    bool should_run = g_effects_enabled && g_uniform_manager.has_any_uniforms();
    if (should_run && !g_audio_thread_running.load()) {
        StartAudioCaptureThread(g_audio_config, g_audio_thread_running, g_audio_thread, g_audio_data_mutex, g_audio_data);
    } else if (!should_run && g_audio_thread_running.load()) {
        StopAudioCaptureThread(g_audio_thread_running, g_audio_thread);
    }
}

/**
 * @brief Helper: Called when uniforms are (re)cached.
 */
static void OnUniformsChanged() {
    bool should_run = g_effects_enabled && g_uniform_manager.has_any_uniforms();
    if (should_run && !g_audio_thread_running.load()) {
        StartAudioCaptureThread(g_audio_config, g_audio_thread_running, g_audio_thread, g_audio_data_mutex, g_audio_data);
    } else if (!should_run && g_audio_thread_running.load()) {
        StopAudioCaptureThread(g_audio_thread_running, g_audio_thread);
    }
}

/**
 * @brief Caches all Listeningway_* uniforms on effect reload.
 * @param runtime The ReShade effect runtime.
 */
static void OnReloadedEffects(reshade::api::effect_runtime* runtime) {
    g_uniform_manager.clear();
    g_uniform_manager.cache_uniforms(runtime);
    OnUniformsChanged();
}

/**
 * @brief Overlay callback for ReShade. Draws the debug overlay.
 * @param runtime The ReShade effect runtime.
 */
static void OverlayCallback(reshade::api::effect_runtime*) {
    DrawListeningwayDebugOverlay(g_audio_data, g_audio_data_mutex, g_audio_thread_running.load());
}

/**
 * @brief DLL entry point and addon registration for Listeningway.
 * Handles ReShade addon lifecycle and event registration.
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    UNREFERENCED_PARAMETER(lpReserved);
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            if (reshade::register_addon(hModule)) {
                OpenLogFile("listeningway.log");
                LogToFile("Addon loaded and log file opened.");
                reshade::register_overlay(nullptr, &OverlayCallback);
                reshade::register_event<reshade::addon_event::reshade_begin_effects>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_begin_effects>::decl)UpdateShaderUniforms);
                reshade::register_event<reshade::addon_event::reshade_reloaded_effects>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_reloaded_effects>::decl)OnReloadedEffects);
                reshade::register_event<reshade::addon_event::reshade_set_effects_state>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_set_effects_state>::decl)OnSetEffectsState);
                g_addon_enabled = true;
            }
            break;
        case DLL_PROCESS_DETACH:
            if (g_addon_enabled.load()) {
                reshade::unregister_overlay(nullptr, &OverlayCallback);
                reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_begin_effects>::decl)UpdateShaderUniforms);
                reshade::unregister_event<reshade::addon_event::reshade_reloaded_effects>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_reloaded_effects>::decl)OnReloadedEffects);
                reshade::unregister_event<reshade::addon_event::reshade_set_effects_state>(
                    (reshade::addon_event_traits<reshade::addon_event::reshade_set_effects_state>::decl)OnSetEffectsState);
                StopAudioCaptureThread(g_audio_thread_running, g_audio_thread);
                CloseLogFile();
                g_addon_enabled = false;
            }
            break;
    }
    return TRUE;
}

/**
 * @mainpage Listeningway ReShade Addon
 *
 * @section intro_sec Introduction
 * Listeningway is a modular audio analysis and visualization addon for ReShade. It captures system audio, analyzes it in real time, and exposes the results to shaders via uniforms.
 *
 * @section extend_sec How to Extend
 * - Add new analysis features in audio_analysis.*
 * - Add new uniforms in uniform_manager.* and update logic
 * - Add new overlay elements in overlay.*
 * - Use logging for debugging and diagnostics
 *
 * The flow is: Audio Capture (thread) -> Analysis -> Uniform Update -> Shader/Overlay
 */

