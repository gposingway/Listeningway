// ---------------------------------------------
// @file constants.h
// @brief Project-wide constants and configuration values
// ---------------------------------------------
#pragma once

// Audio analysis
constexpr size_t DEFAULT_LISTENINGWAY_NUM_BANDS = 32; // Default number of frequency bands
constexpr size_t DEFAULT_LISTENINGWAY_FFT_SIZE = 512; // Default FFT window size
constexpr float DEFAULT_LISTENINGWAY_FLUX_ALPHA = 0.1f; // Default smoothing factor for moving average
constexpr float DEFAULT_LISTENINGWAY_FLUX_THRESHOLD_MULTIPLIER = 1.5f; // Default dynamic threshold multiplier

// Band-limited beat detection
constexpr float DEFAULT_LISTENINGWAY_BEAT_MIN_FREQ = 0.0f; // Default minimum frequency for beat detection (Hz)
constexpr float DEFAULT_LISTENINGWAY_BEAT_MAX_FREQ = 400.0f; // Default maximum frequency for beat detection (Hz)
constexpr float DEFAULT_LISTENINGWAY_FLUX_LOW_ALPHA = 0.35f; // Default smoothing factor for low-frequency flux
constexpr float DEFAULT_LISTENINGWAY_FLUX_LOW_THRESHOLD_MULTIPLIER = 2.0f; // Default threshold multiplier for low-frequency flux

// Spectral Flux with Autocorrelation algorithm defaults
constexpr int DEFAULT_LISTENINGWAY_BEAT_DETECTION_ALGORITHM = 1;
constexpr float DEFAULT_LISTENINGWAY_SPECTRAL_FLUX_THRESHOLD = 0.05f; // Default threshold for peak picking in spectral flux
constexpr float DEFAULT_LISTENINGWAY_TEMPO_CHANGE_THRESHOLD = 0.25f; // Default required confidence for tempo change
constexpr float DEFAULT_LISTENINGWAY_BEAT_INDUCTION_WINDOW = 0.1f; // Default window for beat induction in seconds
constexpr float DEFAULT_LISTENINGWAY_OCTAVE_ERROR_WEIGHT = 0.6f; // Default weight for resolving octave errors (0.5-1.0)
constexpr float DEFAULT_LISTENINGWAY_SPECTRAL_FLUX_DECAY_MULTIPLIER = 2.0f; // Default decay multiplier for beat value

// Audio analysis tunables
constexpr float DEFAULT_LISTENINGWAY_BEAT_FLUX_MIN = 0.01f; // Default minimum flux to trigger beat
constexpr float DEFAULT_LISTENINGWAY_BEAT_FALLOFF_DEFAULT = 1.5f; // Default beat falloff rate (reduced from 2.0f)
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_SCALE = 1e-9f; // Default time scaling for beat interval
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_INITIAL = 0.5f; // Default initial time since last beat
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_MIN = 0.05f; // Default minimum time for adaptive falloff
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_DIVISOR = 0.05f; // Default divisor for adaptive falloff (reduced from 0.1f)
constexpr float DEFAULT_LISTENINGWAY_VOLUME_NORM = 2.0f; // Default volume normalization multiplier
constexpr float DEFAULT_LISTENINGWAY_BAND_NORM = 0.1f; // Default frequency band normalization multiplier
constexpr float DEFAULT_LISTENINGWAY_BAND_MIN_FREQ = 80.0f; // Default min frequency for log bands (Hz)
constexpr float DEFAULT_LISTENINGWAY_BAND_MAX_FREQ = 13000.0f; // Default max frequency for log bands (Hz)
constexpr bool  DEFAULT_LISTENINGWAY_BAND_LOG_SCALE = true; // Use logarithmic band mapping by default
constexpr float DEFAULT_LISTENINGWAY_BAND_LOG_STRENGTH = 0.5f; // Default log scale strength (1.0 = standard log, >1.0 = more bass detail)

// UI/Overlay
constexpr float DEFAULT_LISTENINGWAY_FREQ_BAND_ROW_HEIGHT = 24.0f; // Default height per frequency band row in overlay

// UI/Overlay tunables
constexpr float DEFAULT_LISTENINGWAY_UI_PROGRESS_WIDTH = 0.9f; // Default progress bar width multiplier

// General constants
constexpr float DEFAULT_LISTENINGWAY_CAPTURE_STALE_TIMEOUT = 1.5f;
