#include "base_renderer.h"
#include <imgui.h>

namespace Listeningway::UI {

void BaseRenderer::RenderSliderWithTooltip(const char* id, float* value, float min, float max, 
                                          const char* format, const char* label, const char* tooltip) {
    // Extract repetitive ImGui slider pattern found 15+ times in overlay.cpp
    if (ImGui::SliderFloat(id, value, min, max, format)) {
        // Value changed - derived classes can override for specific handling
    }
    ImGui::SameLine();
    ImGui::Text("%s", label);
    if (ImGui::IsItemHovered(-1) && tooltip) {
        ImGui::SetTooltip("%s", tooltip);
    }
}

void BaseRenderer::RenderProgressBarWithLabel(const char* label, float value, float bar_width) {
    // Extract repetitive progress bar pattern from volume/beat rendering
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s:", label);
    ImGui::SameLine();
    
    if (bar_width <= 0.0f) {
        bar_width = ImGui::GetContentRegionAvail().x;
    }
    
    ImGui::ProgressBar(value, ImVec2(bar_width, 0.0f));
}

} // namespace Listeningway::UI
