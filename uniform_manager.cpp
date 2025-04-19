// ---------------------------------------------
// @file uniform_manager.cpp
// @brief Uniform management implementation for Listeningway ReShade addon
// ---------------------------------------------
#include "uniform_manager.h"
#include <string_view>
#include "settings.h" // Include settings header for g_settings

void UniformManager::update_uniforms(reshade::api::effect_runtime* runtime, float volume, const std::vector<float>& freq_bands, float beat) {
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
            }
        }
    });
}
