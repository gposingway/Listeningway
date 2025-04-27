// ---------------------------------------------
// Overlay Module Implementation
// Provides a debug ImGui overlay for real-time audio analysis data
// ---------------------------------------------
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#define ImTextureID ImU64
#include <imgui.h>
#include <reshade.hpp>
#include "overlay.h"
#include "audio_analysis.h"
#include "constants.h"
#include "settings.h"
#include "logging.h"
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <chrono>
#include <cmath>

extern std::atomic_bool g_audio_analysis_enabled;
extern bool g_listeningway_debug_enabled;

// Helper: Draw toggles (audio analysis, debug logging)
static void DrawToggles() {
    bool enabled = g_settings.audio_analysis_enabled;
    if (ImGui::Checkbox("Enable Audio Analysis", &enabled)) {
        SetAudioAnalysisEnabled(enabled);
        g_settings.audio_analysis_enabled.store(enabled);
        LOG_DEBUG(std::string("[Overlay] Audio Analysis toggled ") + (enabled ? "ON" : "OFF"));
    }
    if (ImGui::Checkbox("Enable Debug Logging", &g_settings.debug_enabled)) {
        SetDebugEnabled(g_settings.debug_enabled);
        LOG_DEBUG(std::string("[Overlay] Debug Logging toggled ") + (g_settings.debug_enabled ? "ON" : "OFF"));
    }
}

// Helper: Draw log file info
static void DrawLogInfo() {
    if (g_settings.debug_enabled) {
        ImGui::Text("Log file: ");
        ImGui::SameLine();
        std::string logPath = GetLogFilePath();
        if (ImGui::Selectable(logPath.c_str())) {
            ShellExecuteA(nullptr, "open", logPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        ImGui::Text("(Click to open log file)");
    }
}

// Helper: Draw website link
static void DrawWebsite() {
    ImGui::Text("Website:");
    ImGui::SameLine();
    if (ImGui::Selectable("https://github.com/gposingway/Listeningway")) {
        ShellExecuteA(nullptr, "open", "https://github.com/gposingway/Listeningway", nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
}

// Helper: Draw volume meter
static void DrawVolume(const AudioAnalysisData& data) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Volume:");
    ImGui::SameLine();
    ImGui::ProgressBar(data.volume, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
}

// Helper: Draw beat meter
static void DrawBeat(const AudioAnalysisData& data) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Beat:");
    ImGui::SameLine();
    ImGui::ProgressBar(data.beat, ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
}

// Helper: Draw frequency bands with view mode
static void DrawFrequencyBands(const AudioAnalysisData& data) {
    static int band_view_mode = 1; // 0 = Collapsed, 1 = 8-band, 2 = All bands
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Frequency Bands");
    ImGui::SameLine();
    if (ImGui::Button(band_view_mode == 0 ? "[Show 8 Bands]" : band_view_mode == 1 ? "[Show All Bands]" : "[Collapse]")) {
        band_view_mode = (band_view_mode + 1) % 3;
    }
    if (band_view_mode == 0) {
        ImGui::TextDisabled("(Panel collapsed)");
        return;
    }
    size_t band_count = data.freq_bands.size();
    size_t show_bands = (band_view_mode == 1) ? 8 : band_count;
    ImGui::BeginChild("FreqBandsChild", ImVec2(0, g_settings.freq_band_row_height * show_bands + 5), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
    for (size_t i = 0; i < band_count; ++i) {
        if (band_view_mode == 1 && i >= 8) break;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%zu:", i);
        ImGui::SameLine();
        ImGui::ProgressBar(data.freq_bands[i], ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
    }
    ImGui::EndChild();
}

// Helper: Draw time/phase info
static void DrawTimePhaseInfo() {
    // Calculate time since start (same as in listeningway_addon.cpp)
    static const auto start_time = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = now - start_time;
    float time_seconds = elapsed.count();
    float phase_60hz = std::fmod(time_seconds * 60.0f, 1.0f);
    float phase_120hz = std::fmod(time_seconds * 120.0f, 1.0f);
    float total_phases_60hz = time_seconds * 60.0f;
    float total_phases_120hz = time_seconds * 120.0f;
    ImGui::Text("Time/Phase Uniforms:");
    ImGui::Text("  Seconds: %.3f", time_seconds);
    ImGui::Text("  Phase 60Hz: %.3f", phase_60hz);
    ImGui::Text("  Phase 120Hz: %.3f", phase_120hz);
    ImGui::Text("  Total 60Hz cycles: %.1f", total_phases_60hz);
    ImGui::Text("  Total 120Hz cycles: %.1f", total_phases_120hz);
}

// Draws the Listeningway debug overlay using ImGui.
// Shows volume, beat, and frequency bands in real time.
void DrawListeningwayDebugOverlay(const AudioAnalysisData& data, std::mutex& data_mutex) {
    try {
        std::lock_guard<std::mutex> lock(data_mutex);
        DrawToggles();
        DrawLogInfo();
        ImGui::Separator();
        DrawWebsite();
        ImGui::Separator();
        DrawVolume(data);
        ImGui::Separator();
        DrawBeat(data);
        ImGui::Separator();
        DrawFrequencyBands(data);
        ImGui::Separator();
        DrawTimePhaseInfo();
        ImGui::Separator();
        ImGui::Text("Frequency Band Mapping:");
        ImGui::Checkbox("Logarithmic Bands", &g_settings.band_log_scale);
        ImGui::SameLine();
        ImGui::TextDisabled("(Log scale better matches hearing; linear is legacy)");
        if (g_settings.band_log_scale) {
            ImGui::SliderFloat("Log Strength (Bass Detail)", &g_settings.band_log_strength, 0.2f, 3.0f, "%.2f");
            ImGui::SliderFloat("Min Freq (Hz)", &g_settings.band_min_freq, 10.0f, 500.0f, "%.0f");
            ImGui::SliderFloat("Max Freq (Hz)", &g_settings.band_max_freq, 2000.0f, 22050.0f, "%.0f");
        }
        if (ImGui::Button("Save Band Mapping Settings")) SaveAllTunables();
        ImGui::Separator();
        
        // Band-limited Beat Detection Settings
        ImGui::Text("Band-Limited Beat Detection:");
        ImGui::TextDisabled("(Focus beat detection on specific frequency range, e.g., bass/kick drums)");
        ImGui::SliderFloat("Beat Min Freq (Hz)", &g_settings.beat_min_freq, 0.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Beat Max Freq (Hz)", &g_settings.beat_max_freq, 100.0f, 500.0f, "%.1f");
        ImGui::SliderFloat("Low Flux Smoothing", &g_settings.flux_low_alpha, 0.01f, 0.5f, "%.3f");
        ImGui::TextDisabled("(Lower value = smoother, higher = more responsive)");
        ImGui::SliderFloat("Low Flux Threshold", &g_settings.flux_low_threshold_multiplier, 1.0f, 3.0f, "%.2f");
        ImGui::TextDisabled("(Lower value = more sensitive, higher = less false positives)");
        if (ImGui::Button("Save Beat Detection Settings")) SaveAllTunables();
        ImGui::Separator();
    } catch (const std::exception& ex) {
        LOG_ERROR(std::string("[Overlay] Exception in DrawListeningwayDebugOverlay: ") + ex.what());
    } catch (...) {
        LOG_ERROR("[Overlay] Unknown exception in DrawListeningwayDebugOverlay.");
    }
}
