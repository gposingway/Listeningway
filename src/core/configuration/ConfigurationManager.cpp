#include "ConfigurationManager.h"
#include "logging.h"
#include <algorithm>
#include <set>

namespace Listeningway {

// Static configuration instance
Configuration ConfigurationManager::m_config = {};

ConfigurationManager& ConfigurationManager::Instance() {
    static ConfigurationManager instance;
    return instance;
}

Configuration& ConfigurationManager::Config() {
    return Instance().GetConfig();
}

const Configuration& ConfigurationManager::ConfigConst() {
    return Instance().GetConfig();
}

Configuration& ConfigurationManager::GetConfig() {
    return m_config;
}

const Configuration& ConfigurationManager::GetConfig() const {
    return m_config;
}

bool ConfigurationManager::Save(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.Save(filename.empty() ? Configuration::GetDefaultConfigPath() : filename);
}

bool ConfigurationManager::Load(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool loaded = m_config.Load(filename.empty() ? Configuration::GetDefaultConfigPath() : filename);
    ValidateProvider();
    m_config.Validate();
    return loaded;
}

void ConfigurationManager::ResetToDefaults() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.ResetToDefaults();
    ValidateProvider();
}

void ConfigurationManager::EnsureValidProvider() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ValidateProvider();
}

std::vector<std::string> ConfigurationManager::EnumerateAvailableProviders() const {
    // TODO: Implement actual provider enumeration logic
    // Example: return {"system", "process", "off"};
    return {"system", "process", "off"};
}

std::string ConfigurationManager::GetDefaultProviderCode() const {
    // TODO: Implement logic to determine the default provider
    // Example: return "system";
    return "system";
}

void ConfigurationManager::ValidateProvider() {
    auto available = EnumerateAvailableProviders();
    auto& code = m_config.audio.captureProviderCode;
    if (std::find(available.begin(), available.end(), code) == available.end()) {
        code = GetDefaultProviderCode();
    }
}

ConfigurationManager::ConfigurationManager() = default;

} // namespace Listeningway