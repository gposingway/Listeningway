// ---------------------------------------------
// @file settings.h
// @brief Simple .ini settings manager for Listeningway
// ---------------------------------------------
#pragma once
#include <string>
#include <atomic>
#include "constants.h"

/**
 * @brief Holds all tunable settings for Listeningway, loaded from the ini file.
 */
struct ListeningwaySettings {
    size_t num_bands = DEFAULT_LISTENINGWAY_NUM_BANDS;
    size_t fft_size = DEFAULT_LISTENINGWAY_FFT_SIZE;
    float flux_alpha = DEFAULT_LISTENINGWAY_FLUX_ALPHA;
    float flux_threshold_multiplier = DEFAULT_LISTENINGWAY_FLUX_THRESHOLD_MULTIPLIER;
    float beat_flux_min = DEFAULT_LISTENINGWAY_BEAT_FLUX_MIN;
    float beat_falloff_default = DEFAULT_LISTENINGWAY_BEAT_FALLOFF_DEFAULT;
    float beat_time_scale = DEFAULT_LISTENINGWAY_BEAT_TIME_SCALE;
    float beat_time_initial = DEFAULT_LISTENINGWAY_BEAT_TIME_INITIAL;
    float beat_time_min = DEFAULT_LISTENINGWAY_BEAT_TIME_MIN;
    float beat_time_divisor = DEFAULT_LISTENINGWAY_BEAT_TIME_DIVISOR;
    float volume_norm = DEFAULT_LISTENINGWAY_VOLUME_NORM;
    float band_norm = DEFAULT_LISTENINGWAY_BAND_NORM;
    float freq_band_row_height = DEFAULT_LISTENINGWAY_FREQ_BAND_ROW_HEIGHT;
    float ui_progress_width = DEFAULT_LISTENINGWAY_UI_PROGRESS_WIDTH;
    float capture_stale_timeout = DEFAULT_LISTENINGWAY_CAPTURE_STALE_TIMEOUT;
    std::atomic_bool audio_analysis_enabled = true;
    bool debug_enabled = false;
};

extern ListeningwaySettings g_settings;

/**
 * @brief Loads all tunables from .ini (or uses defaults).
 */
void LoadAllTunables();
/**
 * @brief Saves all tunables to .ini.
 */
void SaveAllTunables();
/**
 * @brief Loads settings from an .ini file in the same directory as the DLL.
 */
void LoadSettings();
/**
 * @brief Saves settings to the .ini file.
 */
void SaveSettings();
/**
 * @brief Gets the current value of the audio analysis toggle.
 */
bool GetAudioAnalysisEnabled();
/**
 * @brief Sets the value of the audio analysis toggle and saves.
 */
void SetAudioAnalysisEnabled(bool enabled);
/**
 * @brief Gets the current value of the debug toggle.
 */
bool GetDebugEnabled();
/**
 * @brief Sets the value of the debug toggle and saves.
 */
void SetDebugEnabled(bool enabled);
/**
 * @brief Gets the path to the settings file.
 */
std::string GetSettingsPath();
/**
 * @brief Gets the path to the log file.
 */
std::string GetLogFilePath();
