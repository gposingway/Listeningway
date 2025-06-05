#pragma once
#include "base_renderer.h"

namespace Listeningway::UI {

/**
 * @brief Renderer for beat detection settings and controls
 * Extracts complex beat detection UI logic from overlay.cpp
 */
class BeatDetectionRenderer : public BaseRenderer {
public:
    void Render(const AudioAnalysisData& data) override;

private:
    void RenderAlgorithmSelection(const AudioAnalysisData& data);
    void RenderSimpleEnergySettings();
    void RenderSpectralFluxSettings(const AudioAnalysisData& data);
    void RenderBeatDecaySettings();
    void RenderTempoInfo(const AudioAnalysisData& data);
    
    // Extract repetitive beat parameter sliders (YAGNI review target)
    void RenderAdvancedParameter(const char* id, float* value, float min, float max,
                               const char* format, const char* label, const char* tooltip);
};

} // namespace Listeningway::UI
