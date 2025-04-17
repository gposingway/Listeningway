// ---------------------------------------------
// @file uniform_manager.cpp
// @brief Uniform management implementation for Listeningway ReShade addon
// ---------------------------------------------
#include "uniform_manager.h"
#include <string_view>

void UniformManager::cache_uniforms(reshade::api::effect_runtime* runtime) {
    m_volume_uniforms.clear();
    m_freq_bands_uniforms.clear();
    m_beat_uniforms.clear();
    runtime->enumerate_uniform_variables(nullptr, [&](reshade::api::effect_runtime*, reshade::api::effect_uniform_variable var_handle) {
        char name[256] = {};
        runtime->get_uniform_variable_name(var_handle, name);
        if (std::string_view(name) == "Listeningway_Volume") {
            m_volume_uniforms.push_back(var_handle);
        } else if (std::string_view(name) == "Listeningway_FreqBands") {
            m_freq_bands_uniforms.push_back(var_handle);
        } else if (std::string_view(name) == "Listeningway_Beat") {
            m_beat_uniforms.push_back(var_handle);
        }
    });
}

void UniformManager::update_uniforms(reshade::api::effect_runtime* runtime, float volume, const std::vector<float>& freq_bands, float beat) {
    for (auto var_handle : m_volume_uniforms) {
        runtime->set_uniform_value_float(var_handle, &volume, 1);
    }
    for (auto var_handle : m_freq_bands_uniforms) {
        if (!freq_bands.empty())
            runtime->set_uniform_value_float(var_handle, freq_bands.data(), static_cast<uint32_t>(freq_bands.size()));
    }
    for (auto var_handle : m_beat_uniforms) {
        runtime->set_uniform_value_float(var_handle, &beat, 1);
    }
}

void UniformManager::clear() {
    m_volume_uniforms.clear();
    m_freq_bands_uniforms.clear();
    m_beat_uniforms.clear();
}
