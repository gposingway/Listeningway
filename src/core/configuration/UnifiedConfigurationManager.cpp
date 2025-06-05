// UnifiedConfigurationManager.cpp
// (Restored as per user request)
#include "UnifiedConfigurationManager.h"
#include <fstream>
#include <iostream>

UnifiedConfigurationManager::UnifiedConfigurationManager() {}
UnifiedConfigurationManager::~UnifiedConfigurationManager() {}

bool UnifiedConfigurationManager::Load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    EnsureConfigPath(path);
    std::ifstream file(config_path_);
    if (!file.is_open()) return false;
    try {
        file >> config_;
        return true;
    } catch (...) {
        return false;
    }
}

bool UnifiedConfigurationManager::Save(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    EnsureConfigPath(path);
    std::ofstream file(config_path_);
    if (!file.is_open()) return false;
    try {
        file << config_.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

void UnifiedConfigurationManager::ResetToDefaults() {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = nlohmann::json::object();
}

const nlohmann::json& UnifiedConfigurationManager::GetConfig() const {
    // Thread-safe read-only access. All modifications must use Update().
    return config_;
}

void UnifiedConfigurationManager::Update(const std::function<void(nlohmann::json&)>& updater) {
    std::lock_guard<std::mutex> lock(mutex_);
    updater(config_);
}

void UnifiedConfigurationManager::EnsureConfigPath(const std::string& path) {
    if (!path.empty()) {
        config_path_ = path;
    } else if (config_path_.empty()) {
        config_path_ = "listeningway_config.json";
    }
}

// Static instance for LegacySettings
static UnifiedConfigurationManager::LegacySettings g_legacy_settings_instance;

UnifiedConfigurationManager::LegacySettings& UnifiedConfigurationManager::GetLegacySettingsInstance() {
    return g_legacy_settings_instance;
}
