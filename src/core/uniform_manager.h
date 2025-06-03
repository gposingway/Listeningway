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
 * such as volume, frequency bands, beat values, and time/phase parameters. It is designed to work with the
 * Listeningway ReShade addon.
 */
class UniformManager {
public:
    /**
     * @brief Updates all Listeningway_* uniforms with the latest audio and time data.
     * 
     * This function takes the current audio data and time values, updating the corresponding
     * uniforms in the ReShade effect runtime.
     * 
     * @param runtime Pointer to the ReShade effect runtime.
     * @param volume The current audio volume, represented as a float.
     * @param freq_bands A vector containing frequency band data.
     * @param beat The current beat value, represented as a float.
     * @param time_seconds Elapsed time in seconds since addon start.
     * @param phase_60hz Phase in [0,1) at 60Hz.
     * @param phase_120hz Phase in [0,1) at 120Hz.
     * @param total_phases_60hz Total 60Hz cycles since start (float).
     * @param total_phases_120hz Total 120Hz cycles since start (float).     * @param volume_left The current audio volume for the left channel, represented as a float (default 0.0f).
     * @param volume_right The current audio volume for the right channel, represented as a float (default 0.0f).
     * @param audio_pan The audio pan value, represented as a float in the range [-1.0, 1.0] (default 0.0f).
     * @param audio_format The detected audio format, represented as a float (0=none, 1=mono, 2=stereo, 6=5.1, 8=7.1) (default 0.0f).
     */
    void update_uniforms(reshade::api::effect_runtime* runtime, float volume, const std::vector<float>& freq_bands, float beat,
                        float time_seconds, float phase_60hz, float phase_120hz, float total_phases_60hz, float total_phases_120hz,
                        float volume_left = 0.0f, float volume_right = 0.0f, float audio_pan = 0.0f, float audio_format = 0.0f);
};
