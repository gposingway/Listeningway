// ---------------------------------------------
// @file audio_analysis_constants.h
// @brief Constants for audio analysis in Listeningway
// ---------------------------------------------
#pragma once

// Multiplier to normalize RMS volume to [0,1] range
constexpr float VOLUME_NORMALIZATION_FACTOR = 2.0f;
// Multiplier to normalize frequency band magnitudes to [0,1] range
constexpr float FREQ_BAND_NORMALIZATION_FACTOR = 0.1f;
// Smoothing factor for moving average of spectral flux
constexpr float FLUX_SMOOTHING_ALPHA = 0.1f;
// Multiplier for dynamic beat detection threshold
constexpr float FLUX_THRESHOLD_MULTIPLIER = 1.5f;
// Minimum flux value to consider for beat detection
constexpr float FLUX_MIN_BEAT = 0.01f;
// Minimum time between beats (seconds) for adaptive falloff
constexpr float MIN_TIME_BETWEEN_BEATS = 0.05f;
// Minimum denominator for adaptive falloff rate
constexpr float MIN_FALLOFF_DENOMINATOR = 0.1f;
// Default time since last beat if unknown (seconds)
constexpr float DEFAULT_TIME_SINCE_LAST_BEAT = 0.5f;
// Default falloff rate if time since last beat is very short
constexpr float DEFAULT_FALLOFF_RATE = 2.0f;

// Number of frequency bands (should match shader definition)
#define LISTENINGWAY_NUM_BANDS 8
