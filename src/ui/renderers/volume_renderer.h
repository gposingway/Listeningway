#pragma once
#include "base_renderer.h"

namespace Listeningway::UI {

/**
 * @brief Renderer for volume, pan, and spatialization display
 * Extracts repetitive volume rendering logic from overlay.cpp
 */
class VolumeRenderer : public BaseRenderer {
public:
    void Render(const AudioAnalysisData& data) override;

private:
    void RenderMainVolume(const AudioAnalysisData& data);
    void RenderStereoChannels(const AudioAnalysisData& data);
    void RenderPanBar(const AudioAnalysisData& data);
    void RenderBeatMeter(const AudioAnalysisData& data);
    
    // Cached layout calculations
    float CalculateLabelWidth();
    float GetBarStartX(float label_width);
    float GetBarWidth(float bar_start_x);
    
    // Constants for visual consistency
    static constexpr float THIN_BAR_HEIGHT = 6.0f;
    static constexpr float SMALL_SPACING = 2.0f;
};

} // namespace Listeningway::UI
