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

## Audio Analysis Constants

All audio analysis constants (such as normalization factors, smoothing, and band count) are defined in `audio_analysis_constants.h`. Update this file to change analysis parameters globally.

- The number of frequency bands (`LISTENINGWAY_NUM_BANDS`) must match the value in the shader include (`ListeningwayUniforms.fxh`).
- If you change the number of bands, update both C++ and shader code accordingly.

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
     - Install all dependencies using the static triplet (`x64-windows-static`), so no extra DLLs are needed at runtime.
     - Configure the project with CMake in the `build` directory.

2. **Build the Addon**
   - Run:
     ```
     .\simple_build.bat
     ```
   - This will:
     - Build the addon in Release mode.
     - Rename the output DLL to `Listeningway.addon`.
     - Move it to the `dist` directory.

## Installation

1. Copy `dist\Listeningway.addon` to your FFXIV ReShade directory (where your ReShade DLL is).
2. Place `Listeningway.fx` in your ReShade shaders directory.
3. Launch FFXIV. The addon will load automatically; enable the effect in the ReShade overlay.

## Notes

- All dependencies (including KissFFT) are statically linked; no extra DLLs are required in the game directory.
- If you encounter issues, check the ReShade log for errors.
- The number of frequency bands and available uniforms are managed in both the C++ code (see `LISTENINGWAY_NUM_BANDS` and `uniform_manager.*`) and your shader code. Update both if you want to change the number of bands or add new audio-driven uniforms.
- Doxygen-style documentation is available for developers. Run Doxygen with the provided `Doxyfile` in the `tools/reshade` directory to generate API docs.

---

# Listeningway Addon Shader Integration Guide

To use real-time audio data (volume, frequency bands, and beat detection) from the Listeningway ReShade addon in your shaders, follow these steps:

## 1. Include the Uniform Declarations

**Recommended:**

Include the provided uniform header at the top of your shader:

```hlsl
#include "ListeningwayUniforms.fxh"
```

- This file declares all required uniforms and is kept up to date with the addon.
- You can find it in the Listeningway addon package or repository.

**Manual alternative:**

If you prefer, you may declare the uniforms manually (not recommended):

```hlsl
uniform float Listeningway_Volume;
uniform float Listeningway_FreqBands[8];
uniform float Listeningway_Beat;
```

## 2. Requirements

- The Listeningway ReShade addon must be installed and active.
- At least one effect using these uniforms must be enabled for audio capture to run.
- ReShade must be globally enabled (Numpad 7 by default).

## 3. Use the Uniforms in Your Shader Code

You can use these uniforms in your pixel/vertex shaders to drive visual effects. Example:

```hlsl
float bar = Listeningway_FreqBands[band_index]; // Use band_index in [0,7]
float vol = Listeningway_Volume; // Use for global volume-based effects
float beat = Listeningway_Beat; // Use for beat-driven effects
```

## 4. (Optional) Customize Number of Bands

If you want a different number of bands, you must:
- Change the array size in both the shader and the C++ addon (LISTENINGWAY_NUM_BANDS).
- Rebuild the addon and update the shader accordingly.

## 5. Enable the Effect in ReShade

- Place your shader (e.g., `Listeningway.fx`) in the ReShade shaders directory.
- Enable the effect in the ReShade overlay in-game.

## 6. Example Shader Snippet

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