#pragma once
#include "base_renderer.h"

namespace Listeningway::UI {

/**
 * @brief Renderer for frequency band visualizations
 * Eliminates DRY violations in frequency band display logic
 */
class FrequencyBandRenderer : public BaseRenderer {
public:
    void Render(const AudioAnalysisData& data) override;

private:
    void RenderFrequencyBands(const AudioAnalysisData& data);
    void RenderEqualizerSettings();
    void RenderFrequencyMapping();
    
    // Extract repetitive equalizer band patterns
    void RenderEqualizerBand(int band_index, const char* label, const char* tooltip);
    
    // Band labels for DRY compliance
    static constexpr const char* BAND_LABELS[] = {
        "Low (Bass)", "Low-Mid", "Mid", "Mid-High", "High (Treble)"
    };
    static constexpr const char* BAND_TOOLTIPS[] = {
        "Boost for lowest frequency bands (bass)",
        "Boost for low-mid frequency bands", 
        "Boost for mid frequency bands",
        "Boost for mid-high frequency bands",
        "Boost for highest frequency bands (treble)"
    };
};

} // namespace Listeningway::UI
