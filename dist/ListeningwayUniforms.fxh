// ListeningwayUniforms.fxh
// Uniforms for Listeningway ReShade audio integration
//
// Usage:
//   #include "ListeningwayUniforms.fxh"
//
// Requirements:
//   - The Listeningway ReShade addon must be installed and active.
//   - At least one effect using these uniforms must be enabled for audio capture to run.
//   - ReShade must be globally enabled (Numpad 7 by default).
//
// Uniforms:
//   Listeningway_Volume:     [0,1] normalized audio volume
//   Listeningway_FreqBands:  [0,1] array of normalized frequency band magnitudes
//   Listeningway_Beat:       [0,1] beat detection (pulses on detected beats)

// NOTE: The number of bands (LISTENINGWAY_NUM_BANDS) must match the value in audio_analysis_constants.h
// Update both this file and the C++ header if you change the band count.

#define LISTENINGWAY_NUM_BANDS 8

uniform float Listeningway_FreqBands[LISTENINGWAY_NUM_BANDS] < ui_visible = false; >;
uniform float Listeningway_Volume < ui_visible = false; >;
uniform float Listeningway_Beat;
