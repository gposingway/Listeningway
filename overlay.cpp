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

// Draws the Listeningway debug overlay using ImGui.
// Shows volume, beat, and frequency bands in real time.
void DrawListeningwayDebugOverlay(const AudioAnalysisData& data, std::mutex& data_mutex) {
    std::lock_guard<std::mutex> lock(data_mutex);
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
    ImGui::BeginChild("FreqBandsChild", ImVec2(0, LISTENINGWAY_FREQ_BAND_ROW_HEIGHT * data.freq_bands.size()), true, ImGuiWindowFlags_HorizontalScrollbar);
    const float item_width = ImGui::GetContentRegionAvail().x * LISTENINGWAY_UI_PROGRESS_WIDTH;
    for (size_t i = 0; i < data.freq_bands.size(); ++i) {
        ImGui::Text("%zu:", i);
        ImGui::SameLine();
        ImGui::ProgressBar(data.freq_bands[i], ImVec2(item_width, 0.0f));
    }
    ImGui::EndChild();
}
