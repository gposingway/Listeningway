#include "constants.h"
#include "settings.h"
#include "audio_analysis.h"
#include "logging.h"
#include <windows.h>
#include <mutex>
#include <string>
#include <atomic>

// Global settings with default values
ListeningwaySettings g_settings = {
    DEFAULT_NUM_BANDS,
    DEFAULT_FFT_SIZE,
    DEFAULT_FLUX_ALPHA,
    DEFAULT_FLUX_THRESHOLD_MULTIPLIER,
    
    // Beat detection settings
    DEFAULT_BEAT_MIN_FREQ,
    DEFAULT_BEAT_MAX_FREQ,
    DEFAULT_FLUX_LOW_ALPHA,
    DEFAULT_FLUX_LOW_THRESHOLD_MULTIPLIER,    
    DEFAULT_BEAT_FLUX_MIN,
    DEFAULT_BEAT_FALLOFF_DEFAULT,
    DEFAULT_BEAT_TIME_SCALE,
    DEFAULT_BEAT_TIME_INITIAL,
    DEFAULT_BEAT_TIME_MIN,
    DEFAULT_BEAT_TIME_DIVISOR,
    DEFAULT_VOLUME_NORM,
    DEFAULT_BAND_NORM,
    DEFAULT_CAPTURE_STALE_TIMEOUT,
    
    DEFAULT_BEAT_DETECTION_ALGORITHM, // 0 = SimpleEnergy, 1 = SpectralFluxAuto
    
    // Spectral flux autocorrelation settings
    DEFAULT_SPECTRAL_FLUX_THRESHOLD,
    DEFAULT_TEMPO_CHANGE_THRESHOLD,
    DEFAULT_BEAT_INDUCTION_WINDOW,
    DEFAULT_OCTAVE_ERROR_WEIGHT,
    DEFAULT_SPECTRAL_FLUX_DECAY_MULTIPLIER,
    
    DEFAULT_AUDIO_ANALYSIS_ENABLED,   // audio_analysis_enabled
    DEFAULT_DEBUG_ENABLED,            // debug_enabled
    DEFAULT_BAND_LOG_SCALE,           // band_log_scale
    DEFAULT_BAND_MIN_FREQ,            // band_min_freq
    DEFAULT_BAND_MAX_FREQ,            // band_max_freq
    DEFAULT_BAND_LOG_STRENGTH,        // band_log_strength
    
    // 5-Band equalizer settings
    DEFAULT_EQUALIZER_BAND1,          // equalizer_band1
    DEFAULT_EQUALIZER_BAND2,          // equalizer_band2
    DEFAULT_EQUALIZER_BAND3,          // equalizer_band3
    DEFAULT_EQUALIZER_BAND4,          // equalizer_band4
    DEFAULT_EQUALIZER_BAND5,          // equalizer_band5
    DEFAULT_EQUALIZER_WIDTH,          // equalizer_width
    
    // Audio capture provider selection
    DEFAULT_AUDIO_CAPTURE_PROVIDER,   // audio_capture_provider
};

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
    g_audio_analysis_enabled = GetPrivateProfileIntA("General", "AudioAnalysisEnabled", 1, ini.c_str()) != 0;
    g_listeningway_debug_enabled = GetPrivateProfileIntA("General", "DebugEnabled", 0, ini.c_str()) != 0;
    g_settings.debug_enabled = g_listeningway_debug_enabled; // Keep both in sync
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
        g_audio_analyzer.SetBeatDetectionAlgorithm(g_settings.beat_detection_algorithm);
        g_audio_analyzer.Start();
        LOG_DEBUG("[Settings] Audio analyzer started with algorithm: " + 
                 std::to_string(g_settings.beat_detection_algorithm));
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
        std::lock_guard<std::mutex> lock(g_settings_mutex);
        g_listeningway_debug_enabled = enabled;
        g_settings.debug_enabled = enabled; // Ensure both variables are in sync
    }
    SaveSettings();
}

#define RW_INI_FLOAT(section, key, var, def) var = ReadFloatFromIni(ini.c_str(), section, key, def);
#define RW_INI_SIZE(section, key, var, def) var = (size_t)GetPrivateProfileIntA(section, key, (int)(def), ini.c_str());
#define WR_INI_FLOAT(section, key, var) { char buf[32]; sprintf_s(buf, "%.6f", var); WritePrivateProfileStringA(section, key, buf, ini.c_str()); }
#define WR_INI_SIZE(section, key, var) { char buf[32]; sprintf_s(buf, "%u", static_cast<unsigned int>(var)); WritePrivateProfileStringA(section, key, buf, ini.c_str()); }
#define RW_INI_BOOL(section, key, var, def) var = (GetPrivateProfileIntA(section, key, (def) ? 1 : 0, ini.c_str()) != 0);

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
 * @brief Loads all tunable settings from the .ini file into the ListeningwaySettings structure.
 */
void LoadAllTunables() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    RW_INI_SIZE("Audio", "NumBands", g_settings.num_bands, DEFAULT_NUM_BANDS);
    RW_INI_SIZE("Audio", "FFTSize", g_settings.fft_size, DEFAULT_FFT_SIZE);
    RW_INI_FLOAT("Audio", "FluxAlpha", g_settings.flux_alpha, DEFAULT_FLUX_ALPHA);
    RW_INI_FLOAT("Audio", "FluxThresholdMultiplier", g_settings.flux_threshold_multiplier, DEFAULT_FLUX_THRESHOLD_MULTIPLIER);
    
    // Band-limited beat detection settings
    RW_INI_FLOAT("Audio", "BeatMinFreq", g_settings.beat_min_freq, DEFAULT_BEAT_MIN_FREQ);
    RW_INI_FLOAT("Audio", "BeatMaxFreq", g_settings.beat_max_freq, DEFAULT_BEAT_MAX_FREQ);
    RW_INI_FLOAT("Audio", "FluxLowAlpha", g_settings.flux_low_alpha, DEFAULT_FLUX_LOW_ALPHA);
    RW_INI_FLOAT("Audio", "FluxLowThresholdMultiplier", g_settings.flux_low_threshold_multiplier, DEFAULT_FLUX_LOW_THRESHOLD_MULTIPLIER);
    
    RW_INI_FLOAT("Audio", "BeatFluxMin", g_settings.beat_flux_min, DEFAULT_BEAT_FLUX_MIN);
    RW_INI_FLOAT("Audio", "BeatFalloffDefault", g_settings.beat_falloff_default, DEFAULT_BEAT_FALLOFF_DEFAULT);
    RW_INI_FLOAT("Audio", "BeatTimeScale", g_settings.beat_time_scale, DEFAULT_BEAT_TIME_SCALE);
    RW_INI_FLOAT("Audio", "BeatTimeInitial", g_settings.beat_time_initial, DEFAULT_BEAT_TIME_INITIAL);
    RW_INI_FLOAT("Audio", "BeatTimeMin", g_settings.beat_time_min, DEFAULT_BEAT_TIME_MIN);
    RW_INI_FLOAT("Audio", "BeatTimeDivisor", g_settings.beat_time_divisor, DEFAULT_BEAT_TIME_DIVISOR);
    RW_INI_FLOAT("Audio", "VolumeNorm", g_settings.volume_norm, DEFAULT_VOLUME_NORM);
    RW_INI_FLOAT("Audio", "BandNorm", g_settings.band_norm, DEFAULT_BAND_NORM);
    RW_INI_FLOAT("UI", "CaptureStaleTimeout", g_settings.capture_stale_timeout, DEFAULT_CAPTURE_STALE_TIMEOUT);
    RW_INI_BOOL("Audio", "BandLogScale", g_settings.band_log_scale, DEFAULT_BAND_LOG_SCALE);
    RW_INI_FLOAT("Audio", "BandMinFreq", g_settings.band_min_freq, DEFAULT_BAND_MIN_FREQ);
    RW_INI_FLOAT("Audio", "BandMaxFreq", g_settings.band_max_freq, DEFAULT_BAND_MAX_FREQ);
    RW_INI_FLOAT("Audio", "BandLogStrength", g_settings.band_log_strength, DEFAULT_BAND_LOG_STRENGTH);
    
    // 5-Band equalizer
    RW_INI_FLOAT("Audio", "EqualizerBand1", g_settings.equalizer_band1, DEFAULT_EQUALIZER_BAND1);
    RW_INI_FLOAT("Audio", "EqualizerBand2", g_settings.equalizer_band2, DEFAULT_EQUALIZER_BAND2);
    RW_INI_FLOAT("Audio", "EqualizerBand3", g_settings.equalizer_band3, DEFAULT_EQUALIZER_BAND3);
    RW_INI_FLOAT("Audio", "EqualizerBand4", g_settings.equalizer_band4, DEFAULT_EQUALIZER_BAND4);
    RW_INI_FLOAT("Audio", "EqualizerBand5", g_settings.equalizer_band5, DEFAULT_EQUALIZER_BAND5);
    RW_INI_FLOAT("Audio", "EqualizerWidth", g_settings.equalizer_width, DEFAULT_EQUALIZER_WIDTH);
      // Audio capture provider selection (0 = System Audio, 1 = Process Audio)
    g_settings.audio_capture_provider = GetPrivateProfileIntA("Audio", "CaptureProvider", DEFAULT_AUDIO_CAPTURE_PROVIDER, ini.c_str());
    
    // Pan smoothing setting
    RW_INI_FLOAT("Audio", "PanSmoothing", g_settings.pan_smoothing, DEFAULT_PAN_SMOOTHING);
    
    // Beat detection algorithm selection (0 = SimpleEnergy, 1 = SpectralFluxAuto)
    RW_INI_SIZE("Audio", "BeatDetectionAlgorithm", g_settings.beat_detection_algorithm, DEFAULT_BEAT_DETECTION_ALGORITHM);
    
    // Advanced spectral flux autocorrelation settings
    RW_INI_FLOAT("Audio", "SpectralFluxThreshold", g_settings.spectral_flux_threshold, DEFAULT_SPECTRAL_FLUX_THRESHOLD);
    RW_INI_FLOAT("Audio", "TempoChangeThreshold", g_settings.tempo_change_threshold, DEFAULT_TEMPO_CHANGE_THRESHOLD);
    RW_INI_FLOAT("Audio", "BeatInductionWindow", g_settings.beat_induction_window, DEFAULT_BEAT_INDUCTION_WINDOW);
    RW_INI_FLOAT("Audio", "OctaveErrorWeight", g_settings.octave_error_weight, DEFAULT_OCTAVE_ERROR_WEIGHT);
    RW_INI_FLOAT("Audio", "SpectralFluxDecayMultiplier", g_settings.spectral_flux_decay_multiplier, DEFAULT_SPECTRAL_FLUX_DECAY_MULTIPLIER);
}

/**
 * @brief Saves all tunable settings from the ListeningwaySettings structure to the .ini file.
 * Also updates the beat detector if needed.
 */
void SaveAllTunables() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    std::string ini = GetSettingsPath();
    
    // Update beat detector with current settings if audio analysis is enabled
    if (g_audio_analysis_enabled) {
        extern AudioAnalyzer g_audio_analyzer;
        g_audio_analyzer.SetBeatDetectionAlgorithm(g_settings.beat_detection_algorithm);
    }
    
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
    WR_INI_FLOAT("UI", "CaptureStaleTimeout", g_settings.capture_stale_timeout);
    
    // Frequency band settings
    WritePrivateProfileStringA("Audio", "BandLogScale", g_settings.band_log_scale ? "1" : "0", ini.c_str());
    WR_INI_FLOAT("Audio", "BandMinFreq", g_settings.band_min_freq);
    WR_INI_FLOAT("Audio", "BandMaxFreq", g_settings.band_max_freq);
    WR_INI_FLOAT("Audio", "BandLogStrength", g_settings.band_log_strength);
    
    // 5-Band equalizer
    WR_INI_FLOAT("Audio", "EqualizerBand1", g_settings.equalizer_band1);
    WR_INI_FLOAT("Audio", "EqualizerBand2", g_settings.equalizer_band2);
    WR_INI_FLOAT("Audio", "EqualizerBand3", g_settings.equalizer_band3);
    WR_INI_FLOAT("Audio", "EqualizerBand4", g_settings.equalizer_band4);
    WR_INI_FLOAT("Audio", "EqualizerBand5", g_settings.equalizer_band5);
    WR_INI_FLOAT("Audio", "EqualizerWidth", g_settings.equalizer_width);
      // Audio capture provider selection
    WR_INI_SIZE("Audio", "CaptureProvider", g_settings.audio_capture_provider);
    
    // Pan smoothing setting
    WR_INI_FLOAT("Audio", "PanSmoothing", g_settings.pan_smoothing);
    
    // Beat detection algorithm selection
    WR_INI_SIZE("Audio", "BeatDetectionAlgorithm", g_settings.beat_detection_algorithm);
    
    // Advanced spectral flux autocorrelation settings
    WR_INI_FLOAT("Audio", "SpectralFluxThreshold", g_settings.spectral_flux_threshold);
    WR_INI_FLOAT("Audio", "TempoChangeThreshold", g_settings.tempo_change_threshold);
    WR_INI_FLOAT("Audio", "BeatInductionWindow", g_settings.beat_induction_window);
    WR_INI_FLOAT("Audio", "OctaveErrorWeight", g_settings.octave_error_weight);
    WR_INI_FLOAT("Audio", "SpectralFluxDecayMultiplier", g_settings.spectral_flux_decay_multiplier);
}

/**
 * @brief Resets all tunable settings to their default values from constants.h.
 * Also updates the beat detector to use the default algorithm.
 */
void ResetAllTunablesToDefaults() {
    std::lock_guard<std::mutex> lock(g_settings_mutex);
    
    // Reset all settings to defaults defined in constants.h
    g_settings.num_bands = DEFAULT_NUM_BANDS;
    g_settings.fft_size = DEFAULT_FFT_SIZE;
    g_settings.flux_alpha = DEFAULT_FLUX_ALPHA;
    g_settings.flux_threshold_multiplier = DEFAULT_FLUX_THRESHOLD_MULTIPLIER;
    
    // Band-limited beat detection settings
    g_settings.beat_min_freq = DEFAULT_BEAT_MIN_FREQ;
    g_settings.beat_max_freq = DEFAULT_BEAT_MAX_FREQ;
    g_settings.flux_low_alpha = DEFAULT_FLUX_LOW_ALPHA;
    g_settings.flux_low_threshold_multiplier = DEFAULT_FLUX_LOW_THRESHOLD_MULTIPLIER;
    
    g_settings.beat_flux_min = DEFAULT_BEAT_FLUX_MIN;
    g_settings.beat_falloff_default = DEFAULT_BEAT_FALLOFF_DEFAULT;
    g_settings.beat_time_scale = DEFAULT_BEAT_TIME_SCALE;
    g_settings.beat_time_initial = DEFAULT_BEAT_TIME_INITIAL;
    g_settings.beat_time_min = DEFAULT_BEAT_TIME_MIN;
    g_settings.beat_time_divisor = DEFAULT_BEAT_TIME_DIVISOR;
    g_settings.volume_norm = DEFAULT_VOLUME_NORM;
    g_settings.band_norm = DEFAULT_BAND_NORM;
    g_settings.capture_stale_timeout = DEFAULT_CAPTURE_STALE_TIMEOUT;
    
    // Keep audio_analysis_enabled and debug_enabled at their current values
    // since these are UI state rather than tuning parameters
    
    g_settings.band_log_scale = DEFAULT_BAND_LOG_SCALE;
    g_settings.band_min_freq = DEFAULT_BAND_MIN_FREQ;
    g_settings.band_max_freq = DEFAULT_BAND_MAX_FREQ;
    g_settings.band_log_strength = DEFAULT_BAND_LOG_STRENGTH;
    
    // Reset 5-Band equalizer
    g_settings.equalizer_band1 = DEFAULT_EQUALIZER_BAND1;
    g_settings.equalizer_band2 = DEFAULT_EQUALIZER_BAND2;
    g_settings.equalizer_band3 = DEFAULT_EQUALIZER_BAND3;
    g_settings.equalizer_band4 = DEFAULT_EQUALIZER_BAND4;
    g_settings.equalizer_band5 = DEFAULT_EQUALIZER_BAND5;    g_settings.equalizer_width = DEFAULT_EQUALIZER_WIDTH;
    
    // Reset pan smoothing
    g_settings.pan_smoothing = DEFAULT_PAN_SMOOTHING;
    
    // Reset beat detection algorithm settings to defaults
    g_settings.beat_detection_algorithm = DEFAULT_BEAT_DETECTION_ALGORITHM;
    g_settings.spectral_flux_threshold = DEFAULT_SPECTRAL_FLUX_THRESHOLD;
    g_settings.tempo_change_threshold = DEFAULT_TEMPO_CHANGE_THRESHOLD;
    g_settings.beat_induction_window = DEFAULT_BEAT_INDUCTION_WINDOW;
    g_settings.octave_error_weight = DEFAULT_OCTAVE_ERROR_WEIGHT;
    g_settings.spectral_flux_decay_multiplier = DEFAULT_SPECTRAL_FLUX_DECAY_MULTIPLIER;
    
    // Update beat detector with default algorithm if audio analysis is enabled
    if (g_audio_analysis_enabled) {
        extern AudioAnalyzer g_audio_analyzer;
        g_audio_analyzer.SetBeatDetectionAlgorithm(DEFAULT_BEAT_DETECTION_ALGORITHM);
    }
}
