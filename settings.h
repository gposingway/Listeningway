// ---------------------------------------------
// @file settings.h
// @brief Simple .ini settings manager for Listeningway
// ---------------------------------------------
#pragma once
#include <string>
#include <atomic>

// Loads settings from an .ini file in the same directory as the DLL
void LoadSettings();
// Saves settings to the .ini file
void SaveSettings();
// Gets the current value of the audio analysis toggle
bool GetAudioAnalysisEnabled();
// Sets the value of the audio analysis toggle and saves
void SetAudioAnalysisEnabled(bool enabled);
// Gets the current value of the debug toggle
bool GetDebugEnabled();
// Sets the value of the debug toggle and saves
void SetDebugEnabled(bool enabled);

// Gets the path to the settings file
std::string GetSettingsPath();
// Gets the path to the log file
std::string GetLogFilePath();

// Tunable variables (populated from .ini or defaults)
extern size_t g_listeningway_num_bands;
extern size_t g_listeningway_fft_size;
extern float g_listeningway_flux_alpha;
extern float g_listeningway_flux_threshold_multiplier;
extern float g_listeningway_beat_flux_min;
extern float g_listeningway_beat_falloff_default;
extern float g_listeningway_beat_time_scale;
extern float g_listeningway_beat_time_initial;
extern float g_listeningway_beat_time_min;
extern float g_listeningway_beat_time_divisor;
extern float g_listeningway_volume_norm;
extern float g_listeningway_band_norm;
extern float g_listeningway_freq_band_row_height;
extern float g_listeningway_ui_progress_width;
extern std::atomic_bool g_audio_analysis_enabled;
extern bool g_listeningway_debug_enabled;
extern float g_listeningway_capture_stale_timeout;

// Loads all tunables from .ini (or uses defaults)
void LoadAllTunables();
// Saves all tunables to .ini
void SaveAllTunables();
