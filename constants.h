// ---------------------------------------------
// @file constants.h
// @brief Project-wide constants and configuration values
// ---------------------------------------------
#pragma once

// Audio analysis
constexpr size_t LISTENINGWAY_NUM_BANDS = 8; // Number of frequency bands
constexpr size_t LISTENINGWAY_FFT_SIZE = 512; // FFT window size
constexpr float LISTENINGWAY_FLUX_ALPHA = 0.1f; // Smoothing factor for moving average
constexpr float LISTENINGWAY_FLUX_THRESHOLD_MULTIPLIER = 1.5f; // Dynamic threshold multiplier

// Audio analysis tunables
constexpr float LISTENINGWAY_BEAT_FLUX_MIN = 0.01f; // Minimum flux to trigger beat
constexpr float LISTENINGWAY_BEAT_FALLOFF_DEFAULT = 2.0f; // Default beat falloff rate
constexpr float LISTENINGWAY_BEAT_TIME_SCALE = 1e-9f; // Time scaling for beat interval
constexpr float LISTENINGWAY_BEAT_TIME_INITIAL = 0.5f; // Initial time since last beat
constexpr float LISTENINGWAY_BEAT_TIME_MIN = 0.05f; // Minimum time for adaptive falloff
constexpr float LISTENINGWAY_BEAT_TIME_DIVISOR = 0.1f; // Divisor for adaptive falloff
constexpr float LISTENINGWAY_VOLUME_NORM = 2.0f; // Volume normalization multiplier
constexpr float LISTENINGWAY_BAND_NORM = 0.1f; // Frequency band normalization multiplier

// UI/Overlay
constexpr float LISTENINGWAY_FREQ_BAND_ROW_HEIGHT = 24.0f; // Height per frequency band row in overlay

// UI/Overlay tunables
constexpr float LISTENINGWAY_UI_PROGRESS_WIDTH = 0.9f; // Progress bar width multiplier

// Add more constants as needed
