#pragma once

#include "Configuration.h"
#include <mutex>
#include <string>
#include <vector>

namespace Listeningway {

/**
 * @brief Centralized configuration manager (singleton)
 *
 * Owns a single static Configuration instance, provides thread-safe access,
 * and handles all provider logic and persistence.
 */
class ConfigurationManager {
public:
    // Singleton access
    static ConfigurationManager& Instance();

    // Static access to the configuration (always available)
    static Configuration& Config();
    static const Configuration& ConfigConst();

    // Thread-safe access (for advanced use)
    Configuration& GetConfig();
    const Configuration& GetConfig() const;

    // Save/load/reset
    bool Save(const std::string& filename = "");
    bool Load(const std::string& filename = "");
    void ResetToDefaults();

    // Provider logic: enumerate, validate, set default if needed
    void EnsureValidProvider();
    std::vector<std::string> EnumerateAvailableProviders() const;
    std::string GetDefaultProviderCode() const;

private:
    ConfigurationManager();
    ~ConfigurationManager() = default;
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;

    static Configuration m_config;
    mutable std::mutex m_mutex;

    // Helper for provider logic
    void ValidateProvider();
};

} // namespace Listeningway