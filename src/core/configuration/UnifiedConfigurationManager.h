// UnifiedConfigurationManager.h
// Clean, modern configuration manager for Listeningway
#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <optional>
#include <nlohmann/json.hpp>

struct AudioProviderInfo {
    std::string code; // Unique string code for provider
    std::string name; // Human-readable name
    bool is_default = false;
};

class UnifiedConfigurationManager {
public:
    // Singleton access
    static UnifiedConfigurationManager& Instance();

    // Get current config (thread-safe, read-only)
    const nlohmann::json& GetConfig() const;

    // Save/load config to disk (returns true on success)
    bool Save(const std::string& path = "");
    bool Load(const std::string& path = "");

    // Enumerate available audio providers
    std::vector<AudioProviderInfo> EnumerateProviders() const;

    // Get/set current provider code (thread-safe, validates code)
    std::string GetProviderCode() const;
    bool SetProviderCode(const std::string& code);

    // Resets config to defaults (including provider)
    void ResetToDefaults();

private:
    UnifiedConfigurationManager();
    ~UnifiedConfigurationManager();
    UnifiedConfigurationManager(const UnifiedConfigurationManager&) = delete;
    UnifiedConfigurationManager& operator=(const UnifiedConfigurationManager&) = delete;

    void EnsureConfigPath(const std::string& path);
    void ValidateProviderCode();
    void LoadProviderList();

    mutable std::mutex mutex_;
    nlohmann::json config_;
    std::string config_path_;
    std::vector<AudioProviderInfo> providers_;
}

// Remove macro definition of g_config and replace with inline function
// Returns a copy of the config for thread-safety. For modifications, use Update().
inline nlohmann::json GetGlobalConfigCopy() {
    return UnifiedConfigurationManager::Instance().GetConfig();
}

// All modifications must use Update() for thread-safety.
