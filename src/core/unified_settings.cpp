#include "unified_settings.h"

UnifiedConfigurationManager::LegacySettings& g_settings = UnifiedConfigurationManager::GetLegacySettingsInstance();

void InitializeUnifiedSettings() {
    // Optionally initialize fields from config or defaults
}

void LoadAllTunables() {}
void SaveAllTunables() {}
void ResetAllTunablesToDefaults() {}
void LoadSettings() {}
void SaveSettings() {}
bool GetAudioAnalysisEnabled() { return g_settings.audio_analysis_enabled; }
void SetAudioAnalysisEnabled(bool enabled) { g_settings.audio_analysis_enabled = enabled; }
bool GetDebugEnabled() { return g_settings.debug_enabled; }
void SetDebugEnabled(bool enabled) { g_settings.debug_enabled = enabled; }
std::string GetSettingsPath() { return g_settings.settings_path; }
std::string GetLogFilePath() { return g_settings.log_file_path; }
