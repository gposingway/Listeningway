// ---------------------------------------------
// @file uniform_manager.h
// @brief Uniform management for Listeningway ReShade addon
// ---------------------------------------------
#pragma once
#include <vector>
#include <string>
#include <reshade.hpp>

/**
 * @class UniformManager
 * @brief Manages updating of ReShade effect uniforms for Listeningway audio data.
 * 
 * This class provides functionality to update ReShade effect uniforms with audio data
 * such as volume, frequency bands, and beat values. It is designed to work with the
 * Listeningway ReShade addon.
 */
class UniformManager {
public:
    /**
     * @brief Updates all Listeningway_* uniforms with the latest audio data.
     * 
     * This function takes the current audio data and updates the corresponding
     * uniforms in the ReShade effect runtime.
     * 
     * @param runtime Pointer to the ReShade effect runtime.
     * @param volume The current audio volume, represented as a float.
     * @param freq_bands A vector containing frequency band data.
     * @param beat The current beat value, represented as a float.
     */
    void update_uniforms(reshade::api::effect_runtime* runtime, float volume, const std::vector<float>& freq_bands, float beat);
};
