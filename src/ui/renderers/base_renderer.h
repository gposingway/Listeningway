#pragma once
#include "../audio_analysis.h"
#include "../configuration/configuration_manager.h"

namespace Listeningway::UI {

/**
 * @brief Base class for UI renderers implementing common patterns
 */
class BaseRenderer {
public:
    virtual ~BaseRenderer() = default;
    virtual void Render(const AudioAnalysisData& data) = 0;

protected:
    static auto& GetConfig() { return ConfigurationManager::Instance().GetConfig(); }
    
    // Common ImGui patterns to reduce DRY violations
    static void RenderSliderWithTooltip(const char* id, float* value, float min, float max, 
                                       const char* format, const char* label, const char* tooltip);
    static void RenderProgressBarWithLabel(const char* label, float value, float bar_width = 0.0f);
};

} // namespace Listeningway::UI
