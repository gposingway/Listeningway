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
    
    // Band-limited beat detection settings
    float beat_min_freq = DEFAULT_LISTENINGWAY_BEAT_MIN_FREQ;
    float beat_max_freq = DEFAULT_LISTENINGWAY_BEAT_MAX_FREQ;
    float flux_low_alpha = DEFAULT_LISTENINGWAY_FLUX_LOW_ALPHA;
    float flux_low_threshold_multiplier = DEFAULT_LISTENINGWAY_FLUX_LOW_THRESHOLD_MULTIPLIER;
    
    float beat_flux_min = DEFAULT_LISTENINGWAY_BEAT_FLUX_MIN;
    float beat_falloff_default = DEFAULT_LISTENINGWAY_BEAT_FALLOFF_DEFAULT;
    float beat_time_scale = DEFAULT_LISTENINGWAY_BEAT_TIME_SCALE;
    float beat_time_initial = DEFAULT_LISTENINGWAY_BEAT_TIME_INITIAL;
    float beat_time_min = DEFAULT_LISTENINGWAY_BEAT_TIME_MIN;
    float beat_time_divisor = DEFAULT_LISTENINGWAY_BEAT_TIME_DIVISOR;
    float volume_norm = DEFAULT_LISTENINGWAY_VOLUME_NORM;
    float band_norm = DEFAULT_LISTENINGWAY_BAND_NORM;
    float capture_stale_timeout = DEFAULT_LISTENINGWAY_CAPTURE_STALE_TIMEOUT;
    
    // Beat detection algorithm selection (0 = SimpleEnergy, 1 = SpectralFluxAuto)
    int beat_detection_algorithm = DEFAULT_LISTENINGWAY_BEAT_DETECTION_ALGORITHM;
    
    // Advanced spectral flux autocorrelation settings
    float spectral_flux_threshold = DEFAULT_LISTENINGWAY_SPECTRAL_FLUX_THRESHOLD;    // Threshold for peak picking in ODF
    float tempo_change_threshold = DEFAULT_LISTENINGWAY_TEMPO_CHANGE_THRESHOLD;     // Required confidence for tempo change
    float beat_induction_window = DEFAULT_LISTENINGWAY_BEAT_INDUCTION_WINDOW;       // Window for beat induction in seconds
    float octave_error_weight = DEFAULT_LISTENINGWAY_OCTAVE_ERROR_WEIGHT;         // Weight for resolving octave errors (0.5-1.0)
    float spectral_flux_decay_multiplier = DEFAULT_LISTENINGWAY_SPECTRAL_FLUX_DECAY_MULTIPLIER; // Multiplier for beat decay rate (higher = faster decay)
    
    std::atomic_bool audio_analysis_enabled = DEFAULT_LISTENINGWAY_AUDIO_ANALYSIS_ENABLED;
    bool debug_enabled = DEFAULT_LISTENINGWAY_DEBUG_ENABLED;
    bool band_log_scale = DEFAULT_LISTENINGWAY_BAND_LOG_SCALE;
    float band_min_freq = DEFAULT_LISTENINGWAY_BAND_MIN_FREQ;
    float band_max_freq = DEFAULT_LISTENINGWAY_BAND_MAX_FREQ;
    float band_log_strength = DEFAULT_LISTENINGWAY_BAND_LOG_STRENGTH;
    
    // 5-band equalizer settings
    float equalizer_band1 = DEFAULT_LISTENINGWAY_EQUALIZER_BAND1;
    float equalizer_band2 = DEFAULT_LISTENINGWAY_EQUALIZER_BAND2;
    float equalizer_band3 = DEFAULT_LISTENINGWAY_EQUALIZER_BAND3;
    float equalizer_band4 = DEFAULT_LISTENINGWAY_EQUALIZER_BAND4;
    float equalizer_band5 = DEFAULT_LISTENINGWAY_EQUALIZER_BAND5;
    float equalizer_width = DEFAULT_LISTENINGWAY_EQUALIZER_WIDTH;
    
    // Audio capture provider selection (0 = System Audio, 1 = Process Audio)
    // This is the actual provider used if audio_analysis_enabled is true.
    int audio_capture_provider = DEFAULT_LISTENINGWAY_AUDIO_CAPTURE_PROVIDER;

    // User's selection for audio provider in the overlay.
    // -1 for "None (Audio Analysis Off)", 0 for System, 1 for Process, etc.
    int audio_capture_provider_selection = DEFAULT_LISTENINGWAY_AUDIO_CAPTURE_PROVIDER_SELECTION; // New setting
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
 * @brief Resets all tunables to their default values.
 */
void ResetAllTunablesToDefaults();
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
