// ListeningwayUniforms.fxh
// Global uniforms for Listeningway audio visualization
// Annotation is required for all uniforms (see README for details)

#define LISTENINGWAY_NUM_BANDS 8
#define LISTENINGWAY_INSTALLED 1

// Annotation-based (required)
uniform float Listeningway_Volume < source = "listeningway_volume"; >;
uniform float Listeningway_FreqBands[LISTENINGWAY_NUM_BANDS] < source = "listeningway_freqbands"; >;
uniform float Listeningway_Beat < source = "listeningway_beat"; >;

// Time uniforms (added)
uniform float Listeningway_TimeSeconds < source = "listeningway_timeseconds"; >;
uniform float Listeningway_TimePhase60Hz < source = "listeningway_timephase60hz"; >;
uniform float Listeningway_TimePhase120Hz < source = "listeningway_timephase120hz"; >;
uniform float Listeningway_TotalPhases60Hz < source = "listeningway_totalphases60hz"; >;
uniform float Listeningway_TotalPhases120Hz < source = "listeningway_totalphases120hz"; >;
