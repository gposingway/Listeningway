#pragma once

// Audio Analysis
constexpr size_t DEFAULT_LISTENINGWAY_NUM_BANDS = 32;
constexpr size_t DEFAULT_LISTENINGWAY_FFT_SIZE = 512;
constexpr float DEFAULT_LISTENINGWAY_FLUX_ALPHA = 0.1f;
constexpr float DEFAULT_LISTENINGWAY_FLUX_THRESHOLD_MULTIPLIER = 1.5f;

// Beat Detection
constexpr float DEFAULT_LISTENINGWAY_BEAT_MIN_FREQ = 0.0f;
constexpr float DEFAULT_LISTENINGWAY_BEAT_MAX_FREQ = 400.0f;
constexpr float DEFAULT_LISTENINGWAY_FLUX_LOW_ALPHA = 0.35f;
constexpr float DEFAULT_LISTENINGWAY_FLUX_LOW_THRESHOLD_MULTIPLIER = 2.0f;

// Spectral Flux with Autocorrelation
constexpr int DEFAULT_LISTENINGWAY_BEAT_DETECTION_ALGORITHM = 1;
constexpr float DEFAULT_LISTENINGWAY_SPECTRAL_FLUX_THRESHOLD = 0.05f;
constexpr float DEFAULT_LISTENINGWAY_TEMPO_CHANGE_THRESHOLD = 0.25f;
constexpr float DEFAULT_LISTENINGWAY_BEAT_INDUCTION_WINDOW = 0.10f;
constexpr float DEFAULT_LISTENINGWAY_OCTAVE_ERROR_WEIGHT = 0.60f;
constexpr float DEFAULT_LISTENINGWAY_SPECTRAL_FLUX_DECAY_MULTIPLIER = 2.0f;

// Audio Analysis Tunables
constexpr float DEFAULT_LISTENINGWAY_BEAT_FLUX_MIN = 0.01f;
constexpr float DEFAULT_LISTENINGWAY_BEAT_FALLOFF_DEFAULT = 1.5f;
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_SCALE = 1e-9f;
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_INITIAL = 0.5f;
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_MIN = 0.05f;
constexpr float DEFAULT_LISTENINGWAY_BEAT_TIME_DIVISOR = 0.05f;
constexpr float DEFAULT_LISTENINGWAY_VOLUME_NORM = 2.0f;
constexpr float DEFAULT_LISTENINGWAY_BAND_NORM = 0.1f;
constexpr float DEFAULT_LISTENINGWAY_BAND_MIN_FREQ = 34.0f;
constexpr float DEFAULT_LISTENINGWAY_BAND_MAX_FREQ = 13000.0f;
constexpr bool  DEFAULT_LISTENINGWAY_BAND_LOG_SCALE = true;
constexpr float DEFAULT_LISTENINGWAY_BAND_LOG_STRENGTH = 0.47f;

// 5-Band Equalizer
constexpr float DEFAULT_LISTENINGWAY_EQUALIZER_BAND1 = 1.00f;
constexpr float DEFAULT_LISTENINGWAY_EQUALIZER_BAND2 = 1.44f;
constexpr float DEFAULT_LISTENINGWAY_EQUALIZER_BAND3 = 2.00f;
constexpr float DEFAULT_LISTENINGWAY_EQUALIZER_BAND4 = 1.80f;
constexpr float DEFAULT_LISTENINGWAY_EQUALIZER_BAND5 = 1.63f;
constexpr float DEFAULT_LISTENINGWAY_EQUALIZER_WIDTH = 0.15f;

// Audio Capture
constexpr int DEFAULT_LISTENINGWAY_AUDIO_CAPTURE_PROVIDER = 0; // 0 = System, 1 = Process
constexpr int DEFAULT_LISTENINGWAY_AUDIO_CAPTURE_PROVIDER_SELECTION = 0; // 0 = None (Audio Analysis Off)

// Pan smoothing (0.0 = no smoothing, higher values = more smoothing)
constexpr float DEFAULT_LISTENINGWAY_PAN_SMOOTHING = 0.1f; // Default: no smoothing to preserve current behavior

// UI/Overlay tunables
constexpr float DEFAULT_LISTENINGWAY_CAPTURE_STALE_TIMEOUT = 1.5f;

// Global feature flags
constexpr bool DEFAULT_LISTENINGWAY_AUDIO_ANALYSIS_ENABLED = true;  // Audio analysis enabled by default
constexpr bool DEFAULT_LISTENINGWAY_DEBUG_ENABLED = false;          // Debug logging disabled by default
