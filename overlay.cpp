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
    ImGui::Text("Volume:");
    ImGui::SameLine();
    ImGui::ProgressBar(data.volume, ImVec2(-1.0f, 0.0f));
}

// Helper: Draw beat meter
static void DrawBeat(const AudioAnalysisData& data) {
    ImGui::Text("Beat:");
    ImGui::SameLine();
    ImGui::ProgressBar(data.beat, ImVec2(-1.0f, 0.0f));
}

// Helper: Draw frequency bands
static void DrawFrequencyBands(const AudioAnalysisData& data) {
    ImGui::Text("Frequency Bands (%zu):", data.freq_bands.size());
    ImGui::BeginChild("FreqBandsChild", ImVec2(0, g_settings.freq_band_row_height * data.freq_bands.size() + 5), true, ImGuiWindowFlags_HorizontalScrollbar);
    const float item_width = ImGui::GetContentRegionAvail().x * g_settings.ui_progress_width;
    for (size_t i = 0; i < data.freq_bands.size(); ++i) {
        ImGui::Text("%zu:", i);
        ImGui::SameLine();
        ImGui::ProgressBar(data.freq_bands[i], ImVec2(item_width, 0.0f));
    }
    ImGui::EndChild();
}

// Draws the Listeningway debug overlay using ImGui.
// Shows volume, beat, and frequency bands in real time.
void DrawListeningwayDebugOverlay(const AudioAnalysisData& data, std::mutex& data_mutex) {
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
}
