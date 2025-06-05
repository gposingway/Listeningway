#include "constants.h"
#include "settings.h"
#include "audio_analysis.h"
#include "logging.h"
#include "configuration/configuration_manager.h"
#include <windows.h>
#include <mutex>
#include <string>
#include <atomic>

// Legacy global settings - kept for backward compatibility but no longer used
// The actual configuration is now managed by ConfigurationManager
ListeningwaySettings g_settings = {};

std::atomic_bool g_audio_analysis_enabled = DEFAULT_AUDIO_ANALYSIS_ENABLED;
bool g_listeningway_debug_enabled = DEFAULT_DEBUG_ENABLED;

/**
 * @brief Retrieves the path to the settings .ini file.
 * @return The full path to the Listeningway.ini file.
 */
std::string GetSettingsPath() {
    char dllPath[MAX_PATH] = {};
    HMODULE hModule = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetSettingsPath, &hModule);
    GetModuleFileNameA(hModule, dllPath, MAX_PATH);
    std::string path(dllPath);
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) path = path.substr(0, pos + 1);
    return path + "Listeningway.ini";
}

/**
 * @brief Retrieves the path to the log file.
 * @return The full path to the listeningway.log file.
 */
std::string GetLogFilePath() {
    std::string ini = GetSettingsPath();
    size_t pos = ini.find_last_of("\\/");
    std::string dir = (pos != std::string::npos) ? ini.substr(0, pos + 1) : "";
    return dir + "listeningway.log";
}

static std::mutex g_settings_mutex;

/**
 * @brief Loads settings from the .ini file into global variables.
 */
void LoadSettings() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    g_audio_analysis_enabled = GetPrivateProfileIntA("General", "AudioAnalysisEnabled", 1, ini.c_str()) != 0;    g_listeningway_debug_enabled = GetPrivateProfileIntA("General", "DebugEnabled", 0, ini.c_str()) != 0;
    Listeningway::ConfigurationManager::Instance().GetConfig().debug.debugEnabled = g_listeningway_debug_enabled; // Keep both in sync
}

/**
 * @brief Saves current settings to the .ini file.
 */
void SaveSettings() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    WritePrivateProfileStringA("General", "AudioAnalysisEnabled", g_audio_analysis_enabled ? "1" : "0", ini.c_str());
    WritePrivateProfileStringA("General", "DebugEnabled", g_listeningway_debug_enabled ? "1" : "0", ini.c_str());
}

/**
 * @brief Gets the current state of audio analysis.
 * @return True if audio analysis is enabled, false otherwise.
 */
bool GetAudioAnalysisEnabled() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    return g_audio_analysis_enabled;
}

/**
 * @brief Sets the state of audio analysis and saves the setting.
 * @param enabled True to enable audio analysis, false to disable.
 */
void SetAudioAnalysisEnabled(bool enabled) {
    {
        std::lock_guard<std::mutex> lock(g_settings_mutex);
        g_audio_analysis_enabled = enabled;
    }
      // Start or stop the audio analyzer when toggling audio analysis
    if (enabled) {
        // Start the audio analyzer with the current algorithm
        extern AudioAnalyzer g_audio_analyzer;
        const auto config = Listeningway::ConfigurationManager::Snapshot(); // Thread-safe snapshot
        g_audio_analyzer.SetBeatDetectionAlgorithm(config.beat.algorithm);
        g_audio_analyzer.Start();
        LOG_DEBUG("[Settings] Audio analyzer started with algorithm: " + 
                 std::to_string(config.beat.algorithm));
    } else {
        // Stop the audio analyzer
        extern AudioAnalyzer g_audio_analyzer;
        g_audio_analyzer.Stop();
        LOG_DEBUG("[Settings] Audio analyzer stopped");
    }
    
    SaveSettings();
}

/**
 * @brief Gets the current state of debug mode.
 * @return True if debug mode is enabled, false otherwise.
 */
bool GetDebugEnabled() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    return g_listeningway_debug_enabled;
}

/**
 * @brief Sets the state of debug mode and saves the setting.
 * @param enabled True to enable debug mode, false to disable.
 */
void SetDebugEnabled(bool enabled) {
    {
        std::lock_guard<std::mutex> lock(g_settings_mutex);        g_listeningway_debug_enabled = enabled;
        Listeningway::ConfigurationManager::Instance().GetConfig().debug.debugEnabled = enabled; // Ensure both variables are in sync
    }
    SaveSettings();
}

#define RW_INI_FLOAT(section, key, var, def) var = ReadFloatFromIni(ini.c_str(), section, key, def);
#define RW_INI_SIZE(section, key, var, def) var = (size_t)GetPrivateProfileIntA(section, key, (int)(def), ini.c_str());
#define WR_INI_FLOAT(section, key, var) { char buf[32]; sprintf_s(buf, "%.6f", var); WritePrivateProfileStringA(section, key, buf, ini.c_str()); }
#define WR_INI_SIZE(section, key, var) { char buf[32]; sprintf_s(buf, "%u", static_cast<unsigned int>(var)); WritePrivateProfileStringA(section, key, buf, ini.c_str()); }
#define RW_INI_BOOL(section, key, var, def) var = (GetPrivateProfileIntA(section, key, (def) ? 1 : 0, ini.c_str()) != 0);
#define RW_INI_STRING(section, key, var, def) { char buf[256]; GetPrivateProfileStringA(section, key, def, buf, sizeof(buf), ini.c_str()); var = std::string(buf); }
#define WR_INI_STRING(section, key, var) WritePrivateProfileStringA(section, key, var.c_str(), ini.c_str());

/**
 * @brief Reads a floating-point value from an INI file.
 * @param iniFile Path to the INI file.
 * @param section Section name in the INI file.
 * @param key Key name in the INI file.
 * @param defaultValue Default value to use if the key is not found.
 * @return The floating-point value read from the INI file or the default value.
 */
float ReadFloatFromIni(const char* iniFile, const char* section, const char* key, float defaultValue) {
    char buffer[64] = {0};
    char defaultBuffer[64] = {0};
    
    // Convert default value to string with high precision
    sprintf_s(defaultBuffer, "%.6f", defaultValue);
    
    // Get the string value from INI file
    GetPrivateProfileStringA(section, key, defaultBuffer, buffer, sizeof(buffer), iniFile);
    
    // Convert the string to float
    float value = defaultValue;
    try {
        value = std::stof(buffer);
    } catch (...) {
        // If conversion fails, use the default value
        value = defaultValue;
    }
    
    return value;
}

/**
 * @brief Loads all tunable settings using the new ConfigurationManager.
 */
void LoadAllTunables() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    
    // Load configuration from JSON file
    Listeningway::ConfigurationManager::Instance().Load();
    
    LOG_DEBUG("[Settings] Loaded all tunables using ConfigurationManager");
}

/**
 * @brief Saves all tunable settings using the new ConfigurationManager.
 * Also updates the beat detector if needed.
 */
void SaveAllTunables() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
      // Update beat detector with current settings if audio analysis is enabled
    if (g_audio_analysis_enabled) {
        extern AudioAnalyzer g_audio_analyzer;
        const auto config = Listeningway::ConfigurationManager::Snapshot(); // Thread-safe snapshot
        g_audio_analyzer.SetBeatDetectionAlgorithm(config.beat.algorithm);
    }
    
    // Save configuration to JSON file
    Listeningway::ConfigurationManager::Instance().Save();
    
    LOG_DEBUG("[Settings] Saved all tunables using ConfigurationManager");
}

/**
 * @brief Resets all tunable settings to their default values using ConfigurationManager.
 * Also updates the beat detector to use the default algorithm.
 */
void ResetAllTunablesToDefaults() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    
    // Reset configuration to defaults
    Listeningway::ConfigurationManager::Instance().ResetToDefaults();
      // Update beat detector with default algorithm if audio analysis is enabled
    if (g_audio_analysis_enabled) {
        extern AudioAnalyzer g_audio_analyzer;
        const auto config = Listeningway::ConfigurationManager::Snapshot(); // Thread-safe snapshot
        g_audio_analyzer.SetBeatDetectionAlgorithm(config.beat.algorithm);
    }
    
    LOG_DEBUG("[Settings] Reset all tunables to defaults using ConfigurationManager");
}
