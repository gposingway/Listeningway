// ---------------------------------------------
// @file uniform_manager.h
// @brief Uniform management for Listeningway ReShade addon
// ---------------------------------------------
#pragma once
#include <vector>
#include <string>
#include <reshade.hpp>

/**
 * @brief Manages caching and updating of ReShade effect uniforms for Listeningway audio data.
 */
class UniformManager {
public:
    /**
     * @brief Cache all Listeningway_* uniforms in all loaded effects.
     * @param runtime The ReShade effect runtime.
     */
    void cache_uniforms(reshade::api::effect_runtime* runtime);

    /**
     * @brief Update all cached uniforms with the latest audio data.
     * @param runtime The ReShade effect runtime.
     * @param volume The current audio volume.
     * @param freq_bands The current frequency band data.
     * @param beat The current beat value.
     */
    void update_uniforms(reshade::api::effect_runtime* runtime, float volume, const std::vector<float>& freq_bands, float beat);

    /**
     * @brief Clear all cached uniform handles (call on effect reload).
     */
    void clear();

private:
    std::vector<reshade::api::effect_uniform_variable> m_volume_uniforms;
    std::vector<reshade::api::effect_uniform_variable> m_freq_bands_uniforms;
    std::vector<reshade::api::effect_uniform_variable> m_beat_uniforms;
};
