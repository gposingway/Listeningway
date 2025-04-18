# Listeningway - FFXIV Audio Visualizer ReShade Addon

## Description

Listeningway is a modular ReShade addon that captures system audio and provides real-time volume, frequency band, and beat data for use in ReShade effects. It uses WASAPI for audio capture, KissFFT for analysis, and exposes results to shaders via a flexible uniform management system.

## Architecture Overview

- **audio_capture.*:** WASAPI audio capture (threaded)
- **audio_analysis.*:** Real-time audio feature extraction (volume, bands, beat)
- **uniform_manager.*:** Caches and updates all Listeningway_* uniforms in loaded effects
- **overlay.*:** ImGui debug overlay for real-time visualization
- **logging.*:** Thread-safe logging for diagnostics
- **listeningway_addon.cpp:** Main entry point and integration logic
- **settings.cpp/settings.h:** All tunable parameters are loaded from an .ini file (see below)

## Prerequisites

- **Git** (for cloning dependencies)
- **CMake** (added to PATH)
- **Visual Studio** (2022 or newer, with "Desktop development with C++" workload)

## Building

1. **Prepare Dependencies**
   - Open a command prompt in the project root.
   - Run:
     ```
     .\prepare.bat
     ```
   - This will:
     - Clone ReShade SDK and vcpkg if needed.
     - Bootstrap vcpkg.
     - Install all dependencies using the static triplet (`x64-windows-static`).
     - Configure the project with CMake in the `build` directory.

2. **Build the Addon**
   - Run:
     ```
     .\build.bat
     ```
   - This will:
     - Build the addon in Release mode.
     - Rename the output DLL to `Listeningway.addon`.
     - Move it to the `dist` directory.

## Installation

1. Copy `dist\Listeningway.addon` to your FFXIV ReShade directory (where your ReShade DLL is).
2. Place `Listeningway.fx` in your ReShade shaders directory.
3. Launch FFXIV. The addon will load automatically; enable the effect in the ReShade overlay.

## Tuning and Configuration

All tunable parameters (audio analysis, beat detection, UI layout, etc.) are now loaded from `ListeningwaySettings.ini` in the same directory as the DLL. You can edit this file directly to change:
- Number of frequency bands, FFT size, smoothing, thresholds, normalization, etc.
- Beat detection and falloff behavior
- UI/overlay layout and appearance

**How it works:**
- On startup, the addon loads all tunables from `ListeningwaySettings.ini`.
- If a key is missing, the default value from `constants.h` is used.
- When you change a setting in the overlay (such as the audio analysis toggle), it is saved to the .ini file.
- You can also edit the .ini file manually and restart the game/addon to apply changes.

**Example .ini entries:**
```
[Audio]
NumBands=8
FFTSize=512
FluxAlpha=0.1
FluxThresholdMultiplier=1.5
BeatFluxMin=0.01
BeatFalloffDefault=2.0
BeatTimeScale=0.000000001
BeatTimeInitial=0.5
BeatTimeMin=0.05
BeatTimeDivisor=0.1
VolumeNorm=2.0
BandNorm=0.1

[UI]
FreqBandRowHeight=24.0
ProgressWidth=0.9

[General]
AudioAnalysisEnabled=1
```

See comments in `constants.h` for descriptions of each parameter's default.

## Notes

- All dependencies (including KissFFT) are statically linked; no extra DLLs are required in the game directory.
- If you encounter issues, check the ReShade log for errors.
- The number of frequency bands and available uniforms are managed in both the C++ code (see `settings.h` and `uniform_manager.*`) and your shader code. Update both if you want to change the number of bands or add new audio-driven uniforms.
- Doxygen-style documentation is available for developers. Run Doxygen with the provided `Doxyfile` in the `tools/reshade` directory to generate API docs.

---

# Listeningway Addon Shader Integration Guide

To use real-time audio data (volume, frequency bands, and beat detection) from the Listeningway ReShade addon in your shaders, follow these steps:

## 1. Include the Uniform Declarations

Add the following to your shader file (e.g., at the top):

```hlsl
// Listeningway audio uniforms
uniform float Listeningway_Volume;
uniform float Listeningway_FreqBands[8];
uniform float Listeningway_Beat; // (Optional) Beat detection value
```

- `Listeningway_Volume` is a float in the range [0, 1] representing the current audio volume.
- `Listeningway_FreqBands` is a float array (default size 8) where each element represents the normalized magnitude of a frequency band.
- `Listeningway_Beat` is a float in [0, 1] indicating detected beats (useful for pulsing effects).

## 2. Use the Uniforms in Your Shader Code

You can use these uniforms in your pixel/vertex shaders to drive visual effects. Example:

```hlsl
float bar = Listeningway_FreqBands[band_index]; // Use band_index in [0,7]
float vol = Listeningway_Volume; // Use for global volume-based effects
float beat = Listeningway_Beat; // Use for beat-driven effects
```

## 3. (Optional) Customize Number of Bands

If you want a different number of bands, you must:
- Change the array size in both the shader and the .ini file (`NumBands` in `[Audio]` section of `ListeningwaySettings.ini`).
- Restart the game or reload the addon to apply the change.

## 4. Enable the Effect in ReShade

- Place your shader (e.g., `Listeningway.fx`) in the ReShade shaders directory.
- Enable the effect in the ReShade overlay in-game.

## 5. Example Shader Snippet

```hlsl
uniform float Listeningway_Volume;
uniform float Listeningway_FreqBands[8];
uniform float Listeningway_Beat;

float4 main(float2 uv : TEXCOORD) : SV_Target
{
    float intensity = Listeningway_FreqBands[0] * 0.5 + Listeningway_Volume * 0.5 + Listeningway_Beat * 0.5;
    return float4(intensity, 0, 0, 1);
}
```

---

**Note:**  
- The addon will automatically find and update all uniforms named `Listeningway_Volume`, `Listeningway_FreqBands`, and `Listeningway_Beat` in all loaded effects.
- No additional setup is required in the shader beyond declaring the uniforms.