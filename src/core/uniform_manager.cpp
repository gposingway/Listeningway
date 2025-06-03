// ---------------------------------------------
// @file uniform_manager.cpp
// @brief Uniform management implementation for Listeningway ReShade addon
// ---------------------------------------------
#include "uniform_manager.h"
#include <string_view>
#include "settings.h" // Include settings header for g_settings

void UniformManager::update_uniforms(reshade::api::effect_runtime* runtime, float volume, const std::vector<float>& freq_bands, float beat,
    float time_seconds, float phase_60hz, float phase_120hz, float total_phases_60hz, float total_phases_120hz,
    float volume_left, float volume_right, float audio_pan, float audio_format) {
    // Only update uniforms with the correct annotation (source = ...)
    runtime->enumerate_uniform_variables(nullptr, [&](reshade::api::effect_runtime*, reshade::api::effect_uniform_variable var_handle) {
        char source[64] = "";
        if (runtime->get_annotation_string_from_uniform_variable(var_handle, "source", source)) {
            if (strcmp(source, "listeningway_volume") == 0) {
                runtime->set_uniform_value_float(var_handle, &volume, 1);
            } else if (strcmp(source, "listeningway_freqbands") == 0) {
                if (!freq_bands.empty()) {
                    runtime->set_uniform_value_float(var_handle, freq_bands.data(), static_cast<uint32_t>(freq_bands.size()));
                }
            } else if (strcmp(source, "listeningway_beat") == 0) {
                runtime->set_uniform_value_float(var_handle, &beat, 1);
            } else if (strcmp(source, "listeningway_timeseconds") == 0) {
                runtime->set_uniform_value_float(var_handle, &time_seconds, 1);
            } else if (strcmp(source, "listeningway_timephase60hz") == 0) {
                runtime->set_uniform_value_float(var_handle, &phase_60hz, 1);
            } else if (strcmp(source, "listeningway_timephase120hz") == 0) {
                runtime->set_uniform_value_float(var_handle, &phase_120hz, 1);
            } else if (strcmp(source, "listeningway_totalphases60hz") == 0) {
                runtime->set_uniform_value_float(var_handle, &total_phases_60hz, 1);
            } else if (strcmp(source, "listeningway_totalphases120hz") == 0) {
                runtime->set_uniform_value_float(var_handle, &total_phases_120hz, 1);
            } else if (strcmp(source, "listeningway_volumeleft") == 0) {
                runtime->set_uniform_value_float(var_handle, &volume_left, 1); 
            } else if (strcmp(source, "listeningway_volumeright") == 0) {
                runtime->set_uniform_value_float(var_handle, &volume_right, 1);
            } else if (strcmp(source, "listeningway_audiopan") == 0) {
                runtime->set_uniform_value_float(var_handle, &audio_pan, 1);
            } else if (strcmp(source, "listeningway_audioformat") == 0) {
                runtime->set_uniform_value_float(var_handle, &audio_format, 1);
            }
        }
    });
}
