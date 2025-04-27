// ---------------------------------------------
// @file settings.cpp
// @brief Implementation of settings management for Listeningway
// ---------------------------------------------
#include "constants.h"
#include "settings.h"
#include <windows.h>
#include <mutex>
#include <string>
#include <atomic>

// Tunable variables (populated from .ini or defaults)
ListeningwaySettings g_settings;

std::atomic_bool g_audio_analysis_enabled = true;
bool g_listeningway_debug_enabled = false;

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
    g_audio_analysis_enabled = GetPrivateProfileIntA("General", "AudioAnalysisEnabled", 1, ini.c_str()) != 0;
    g_listeningway_debug_enabled = GetPrivateProfileIntA("General", "DebugEnabled", 0, ini.c_str()) != 0;
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
        std::lock_guard<std::mutex> lock(g_settings_mutex);
        g_listeningway_debug_enabled = enabled;
    }
    SaveSettings();
}

#define RW_INI_FLOAT(section, key, var, def) var = (float)GetPrivateProfileIntA(section, key, (int)((def)*10000), ini.c_str()) / 10000.0f;
#define RW_INI_SIZE(section, key, var, def) var = (size_t)GetPrivateProfileIntA(section, key, (int)(def), ini.c_str());
#define WR_INI_FLOAT(section, key, var) { char buf[32]; sprintf_s(buf, "%.6f", var); WritePrivateProfileStringA(section, key, buf, ini.c_str()); }
#define WR_INI_SIZE(section, key, var) { char buf[32]; sprintf_s(buf, "%zu", var); WritePrivateProfileStringA(section, key, buf, ini.c_str()); }
#define RW_INI_BOOL(section, key, var, def) var = (GetPrivateProfileIntA(section, key, (def) ? 1 : 0, ini.c_str()) != 0);

/**
 * @brief Loads all tunable settings from the .ini file into the ListeningwaySettings structure.
 */
void LoadAllTunables() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    RW_INI_SIZE("Audio", "NumBands", g_settings.num_bands, DEFAULT_LISTENINGWAY_NUM_BANDS);
    RW_INI_SIZE("Audio", "FFTSize", g_settings.fft_size, DEFAULT_LISTENINGWAY_FFT_SIZE);
    RW_INI_FLOAT("Audio", "FluxAlpha", g_settings.flux_alpha, DEFAULT_LISTENINGWAY_FLUX_ALPHA);
    RW_INI_FLOAT("Audio", "FluxThresholdMultiplier", g_settings.flux_threshold_multiplier, DEFAULT_LISTENINGWAY_FLUX_THRESHOLD_MULTIPLIER);
    
    // Band-limited beat detection settings
    RW_INI_FLOAT("Audio", "BeatMinFreq", g_settings.beat_min_freq, DEFAULT_LISTENINGWAY_BEAT_MIN_FREQ);
    RW_INI_FLOAT("Audio", "BeatMaxFreq", g_settings.beat_max_freq, DEFAULT_LISTENINGWAY_BEAT_MAX_FREQ);
    RW_INI_FLOAT("Audio", "FluxLowAlpha", g_settings.flux_low_alpha, DEFAULT_LISTENINGWAY_FLUX_LOW_ALPHA);
    RW_INI_FLOAT("Audio", "FluxLowThresholdMultiplier", g_settings.flux_low_threshold_multiplier, DEFAULT_LISTENINGWAY_FLUX_LOW_THRESHOLD_MULTIPLIER);
    
    RW_INI_FLOAT("Audio", "BeatFluxMin", g_settings.beat_flux_min, DEFAULT_LISTENINGWAY_BEAT_FLUX_MIN);
    RW_INI_FLOAT("Audio", "BeatFalloffDefault", g_settings.beat_falloff_default, DEFAULT_LISTENINGWAY_BEAT_FALLOFF_DEFAULT);
    RW_INI_FLOAT("Audio", "BeatTimeScale", g_settings.beat_time_scale, DEFAULT_LISTENINGWAY_BEAT_TIME_SCALE);
    RW_INI_FLOAT("Audio", "BeatTimeInitial", g_settings.beat_time_initial, DEFAULT_LISTENINGWAY_BEAT_TIME_INITIAL);
    RW_INI_FLOAT("Audio", "BeatTimeMin", g_settings.beat_time_min, DEFAULT_LISTENINGWAY_BEAT_TIME_MIN);
    RW_INI_FLOAT("Audio", "BeatTimeDivisor", g_settings.beat_time_divisor, DEFAULT_LISTENINGWAY_BEAT_TIME_DIVISOR);
    RW_INI_FLOAT("Audio", "VolumeNorm", g_settings.volume_norm, DEFAULT_LISTENINGWAY_VOLUME_NORM);
    RW_INI_FLOAT("Audio", "BandNorm", g_settings.band_norm, DEFAULT_LISTENINGWAY_BAND_NORM);
    RW_INI_FLOAT("UI", "FreqBandRowHeight", g_settings.freq_band_row_height, DEFAULT_LISTENINGWAY_FREQ_BAND_ROW_HEIGHT);
    RW_INI_FLOAT("UI", "ProgressWidth", g_settings.ui_progress_width, DEFAULT_LISTENINGWAY_UI_PROGRESS_WIDTH);
    RW_INI_FLOAT("UI", "CaptureStaleTimeout", g_settings.capture_stale_timeout, DEFAULT_LISTENINGWAY_CAPTURE_STALE_TIMEOUT);
    RW_INI_BOOL("Audio", "BandLogScale", g_settings.band_log_scale, DEFAULT_LISTENINGWAY_BAND_LOG_SCALE);
    RW_INI_FLOAT("Audio", "BandMinFreq", g_settings.band_min_freq, DEFAULT_LISTENINGWAY_BAND_MIN_FREQ);
    RW_INI_FLOAT("Audio", "BandMaxFreq", g_settings.band_max_freq, DEFAULT_LISTENINGWAY_BAND_MAX_FREQ);
    RW_INI_FLOAT("Audio", "BandLogStrength", g_settings.band_log_strength, DEFAULT_LISTENINGWAY_BAND_LOG_STRENGTH);
}

/**
 * @brief Saves all tunable settings from the ListeningwaySettings structure to the .ini file.
 */
void SaveAllTunables() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    WR_INI_SIZE("Audio", "NumBands", g_settings.num_bands);
    WR_INI_SIZE("Audio", "FFTSize", g_settings.fft_size);
    WR_INI_FLOAT("Audio", "FluxAlpha", g_settings.flux_alpha);
    WR_INI_FLOAT("Audio", "FluxThresholdMultiplier", g_settings.flux_threshold_multiplier);
    
    // Band-limited beat detection settings
    WR_INI_FLOAT("Audio", "BeatMinFreq", g_settings.beat_min_freq);
    WR_INI_FLOAT("Audio", "BeatMaxFreq", g_settings.beat_max_freq);
    WR_INI_FLOAT("Audio", "FluxLowAlpha", g_settings.flux_low_alpha);
    WR_INI_FLOAT("Audio", "FluxLowThresholdMultiplier", g_settings.flux_low_threshold_multiplier);
    
    WR_INI_FLOAT("Audio", "BeatFluxMin", g_settings.beat_flux_min);
    WR_INI_FLOAT("Audio", "BeatFalloffDefault", g_settings.beat_falloff_default);
    WR_INI_FLOAT("Audio", "BeatTimeScale", g_settings.beat_time_scale);
    WR_INI_FLOAT("Audio", "BeatTimeInitial", g_settings.beat_time_initial);
    WR_INI_FLOAT("Audio", "BeatTimeMin", g_settings.beat_time_min);
    WR_INI_FLOAT("Audio", "BeatTimeDivisor", g_settings.beat_time_divisor);
    WR_INI_FLOAT("Audio", "VolumeNorm", g_settings.volume_norm);
    WR_INI_FLOAT("Audio", "BandNorm", g_settings.band_norm);
    WR_INI_FLOAT("UI", "FreqBandRowHeight", g_settings.freq_band_row_height);
    WR_INI_FLOAT("UI", "ProgressWidth", g_settings.ui_progress_width);
    WR_INI_FLOAT("UI", "CaptureStaleTimeout", g_settings.capture_stale_timeout);
}
