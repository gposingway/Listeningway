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
#include <windows.h>
#include <shellapi.h>
#include <string>

extern std::atomic_bool g_audio_analysis_enabled;
extern bool g_listeningway_debug_enabled;

// Draws the Listeningway debug overlay using ImGui.
// Shows volume, beat, and frequency bands in real time.
void DrawListeningwayDebugOverlay(const AudioAnalysisData& data, std::mutex& data_mutex) {
    std::lock_guard<std::mutex> lock(data_mutex);
    bool enabled = GetAudioAnalysisEnabled();
    if (ImGui::Checkbox("Enable Audio Analysis", &enabled)) {
        SetAudioAnalysisEnabled(enabled);
        g_audio_analysis_enabled.store(enabled);
    }
    ImGui::Separator();
    if (ImGui::Checkbox("Enable Debug Logging", &g_listeningway_debug_enabled)) {
        SetDebugEnabled(g_listeningway_debug_enabled);
    }
    if (g_listeningway_debug_enabled) {
        ImGui::Text("Log file: ");
        ImGui::SameLine();
        std::string logPath = GetLogFilePath();
        if (ImGui::Selectable(logPath.c_str())) {
            ShellExecuteA(nullptr, "open", logPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        ImGui::Text("(Click to open log file)");
    }
    ImGui::Separator();
    // --- Volume meter ---
    ImGui::Text("Volume:");
    ImGui::SameLine();
    ImGui::ProgressBar(data.volume, ImVec2(-1.0f, 0.0f));
    ImGui::Separator();
    // --- Beat meter ---
    ImGui::Text("Beat:");
    ImGui::SameLine();
    ImGui::ProgressBar(data.beat, ImVec2(-1.0f, 0.0f));
    ImGui::Separator();
    // --- Frequency bands ---
    ImGui::Text("Frequency Bands (%zu):", data.freq_bands.size());
    // Make the child window tall enough for all bands
    ImGui::BeginChild("FreqBandsChild", ImVec2(0, g_listeningway_freq_band_row_height * data.freq_bands.size()), true, ImGuiWindowFlags_HorizontalScrollbar);
    const float item_width = ImGui::GetContentRegionAvail().x * g_listeningway_ui_progress_width;
    for (size_t i = 0; i < data.freq_bands.size(); ++i) {
        ImGui::Text("%zu:", i);
        ImGui::SameLine();
        ImGui::ProgressBar(data.freq_bands[i], ImVec2(item_width, 0.0f));
    }
    ImGui::EndChild();
    ImGui::Separator();
    ImGui::Text("Listeningway Addon");
    ImGui::Text("Project: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "https://github.com/gposingway/Listeningway");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
}
