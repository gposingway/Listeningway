#include "frequency_band_renderer.h"
#include <imgui.h>
#include <algorithm>

namespace Listeningway::UI {

void FrequencyBandRenderer::Render(const AudioAnalysisData& data) {
    RenderFrequencyBands(data);
    RenderEqualizerSettings();
    RenderFrequencyMapping();
}

void FrequencyBandRenderer::RenderFrequencyBands(const AudioAnalysisData& data) {
    auto& config = GetConfig();
    float amp = config.frequency.amplifier;
    
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Frequency Bands");
    
    // Render frequency band visualization
    for (size_t i = 0; i < data.freq_bands.size() && i < config.frequency.bands; ++i) {
        float band_value = std::clamp(data.freq_bands[i] * amp, 0.0f, 1.0f);
        
        // Use consistent progress bar rendering
        std::string label = "Band " + std::to_string(i + 1);
        RenderProgressBarWithLabel(label.c_str(), band_value);
    }
}

void FrequencyBandRenderer::RenderEqualizerSettings() {
    if (ImGui::CollapsingHeader("Frequency Boost Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Equalizer");
        
        // BEFORE: 5 identical blocks of repetitive code (DRY violation)
        // AFTER: Loop-based approach eliminates duplication
        for (int i = 0; i < 5; ++i) {
            RenderEqualizerBand(i, BAND_LABELS[i], BAND_TOOLTIPS[i]);
        }
        
        ImGui::PopID();
    }
}

void FrequencyBandRenderer::RenderEqualizerBand(int band_index, const char* label, const char* tooltip) {
    auto& config = GetConfig();
    
    // Extract the repetitive equalizer band pattern
    std::string slider_id = "##band" + std::to_string(band_index + 1);
    float& band_value = config.frequency.equalizerBands[band_index];
    
    RenderSliderWithTooltip(slider_id.c_str(), &band_value, 0.0f, 4.0f, "%.2f", label, tooltip);
}

void FrequencyBandRenderer::RenderFrequencyMapping() {
    auto& config = GetConfig();
    
    if (ImGui::CollapsingHeader("Frequency Band Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Extract repetitive frequency mapping sliders
        RenderSliderWithTooltip("##minFreq", &config.frequency.minFreq, 10.0f, 500.0f, "%.0f Hz", 
                               "Min Frequency", "Lowest frequency to analyze");
        
        RenderSliderWithTooltip("##maxFreq", &config.frequency.maxFreq, 2000.0f, 22050.0f, "%.0f Hz", 
                               "Max Frequency", "Highest frequency to analyze");
        
        RenderSliderWithTooltip("##amplifier", &config.frequency.amplifier, 1.0f, 11.0f, "%.1fx", 
                               "Amplifier", "Multiplier for all frequency data");
    }
}

} // namespace Listeningway::UI
