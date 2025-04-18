// ListeningwayUniforms.fxh
// Global uniforms for Listeningway audio visualization
// Annotation is required for all uniforms (see README for details)

#define LISTENINGWAY_NUM_BANDS 8

// Annotation-based (required)
uniform float Listeningway_Volume < source = "listeningway_volume"; >;
uniform float Listeningway_FreqBands[LISTENINGWAY_NUM_BANDS] < source = "listeningway_freqbands"; >;
uniform float Listeningway_Beat < source = "listeningway_beat"; >;
