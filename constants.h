// ---------------------------------------------
// @file constants.h
// @brief Project-wide constants and configuration values
// ---------------------------------------------
#pragma once

// Audio analysis
constexpr size_t DEFAULT_LISTENINGWAY_NUM_BANDS = 8; // Default number of frequency bands
constexpr size_t DEFAULT_LISTENINGWAY_FFT_SIZE = 512; // Default FFT window size
constexpr float DEFAULT_LISTENINGWAY_FLUX_ALPHA = 0.1f; // Default smoothing factor for moving average
constexpr float DEFAULT_LISTENINGWAY_FLUX_THRESHOLD_MULTIPLIER = 1.5f; // Default dynamic threshold multiplier

// Audio analysis tunables
constexpr float DEFAULT_LISTENINGWAY_BEAT_FLUX_MIN = 0.01f; // Default minimum flux to trigger beat
constexpr float DEFAULT_LISTENINGWAY_BEAT_FALLOFF_DEFAULT = 2.0f; // Default beat falloff rate
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_SCALE = 1e-9f; // Default time scaling for beat interval
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_INITIAL = 0.5f; // Default initial time since last beat
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_MIN = 0.05f; // Default minimum time for adaptive falloff
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_DIVISOR = 0.1f; // Default divisor for adaptive falloff
constexpr float DEFAULT_LISTENINGWAY_VOLUME_NORM = 2.0f; // Default volume normalization multiplier
constexpr float DEFAULT_LISTENINGWAY_BAND_NORM = 0.1f; // Default frequency band normalization multiplier

// UI/Overlay
constexpr float DEFAULT_LISTENINGWAY_FREQ_BAND_ROW_HEIGHT = 24.0f; // Default height per frequency band row in overlay

// UI/Overlay tunables
constexpr float DEFAULT_LISTENINGWAY_UI_PROGRESS_WIDTH = 0.9f; // Default progress bar width multiplier

// General constants
constexpr float DEFAULT_LISTENINGWAY_CAPTURE_STALE_TIMEOUT = 1.5f;

// Add more constants as needed
