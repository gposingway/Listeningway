// ---------------------------------------------
// @file settings.cpp
// @brief Simple .ini settings manager for Listeningway
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

std::string GetLogFilePath() {
    std::string ini = GetSettingsPath();
    size_t pos = ini.find_last_of("\\/");
    std::string dir = (pos != std::string::npos) ? ini.substr(0, pos + 1) : "";
    return dir + "listeningway.log";
}

static std::mutex g_settings_mutex;

void LoadSettings() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    g_audio_analysis_enabled = GetPrivateProfileIntA("General", "AudioAnalysisEnabled", 1, ini.c_str()) != 0;
    g_listeningway_debug_enabled = GetPrivateProfileIntA("General", "DebugEnabled", 0, ini.c_str()) != 0;
}

void SaveSettings() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    WritePrivateProfileStringA("General", "AudioAnalysisEnabled", g_audio_analysis_enabled ? "1" : "0", ini.c_str());
    WritePrivateProfileStringA("General", "DebugEnabled", g_listeningway_debug_enabled ? "1" : "0", ini.c_str());
}

bool GetAudioAnalysisEnabled() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    return g_audio_analysis_enabled;
}

void SetAudioAnalysisEnabled(bool enabled) {
    {
        std::lock_guard<std::mutex> lock(g_settings_mutex);
        g_audio_analysis_enabled = enabled;
    }
    SaveSettings();
}

bool GetDebugEnabled() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    return g_listeningway_debug_enabled;
}

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

void LoadAllTunables() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    RW_INI_SIZE("Audio", "NumBands", g_settings.num_bands, DEFAULT_LISTENINGWAY_NUM_BANDS);
    RW_INI_SIZE("Audio", "FFTSize", g_settings.fft_size, DEFAULT_LISTENINGWAY_FFT_SIZE);
    RW_INI_FLOAT("Audio", "FluxAlpha", g_settings.flux_alpha, DEFAULT_LISTENINGWAY_FLUX_ALPHA);
    RW_INI_FLOAT("Audio", "FluxThresholdMultiplier", g_settings.flux_threshold_multiplier, DEFAULT_LISTENINGWAY_FLUX_THRESHOLD_MULTIPLIER);
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
}

void SaveAllTunables() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    WR_INI_SIZE("Audio", "NumBands", g_settings.num_bands);
    WR_INI_SIZE("Audio", "FFTSize", g_settings.fft_size);
    WR_INI_FLOAT("Audio", "FluxAlpha", g_settings.flux_alpha);
    WR_INI_FLOAT("Audio", "FluxThresholdMultiplier", g_settings.flux_threshold_multiplier);
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
