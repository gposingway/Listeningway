// ---------------------------------------------
// @file uniform_manager.h
// @brief Uniform management for Listeningway ReShade addon
// ---------------------------------------------
#pragma once
#include <vector>
#include <string>
#include <reshade.hpp>

/**
 * @brief Manages updating of ReShade effect uniforms for Listeningway audio data.
 */
class UniformManager {
public:
    /**
     * @brief Update all Listeningway_* uniforms with the latest audio data.
     * @param runtime The ReShade effect runtime.
     * @param volume The current audio volume.
     * @param freq_bands The current frequency band data.
     * @param beat The current beat value.
     */
    void update_uniforms(reshade::api::effect_runtime* runtime, float volume, const std::vector<float>& freq_bands, float beat);
};
