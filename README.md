# Listeningway - Audio Visualizer ReShade Addon

![Listeningway3](https://github.com/user-attachments/assets/0a8338cd-3694-4318-ab69-389f6a0b082e)

## Quick Start for Shader Developers

Listeningway exposes real-time audio data (volume, frequency bands, beat) to your ReShade shaders. You must use annotation-based uniforms:

### Annotation-based (required)
Add a `source` annotation to your uniform variable. This is robust, avoids name collisions, and is required.

```hlsl
// Use your own names with annotation:
uniform float MyVolume < source = "listeningway_volume"; >;
uniform float MyBands[8] < source = "listeningway_freqbands"; >;
uniform float MyBeat < source = "listeningway_beat"; >;
```

**Tip:** You can also `#include "ListeningwayUniforms.fxh"` for ready-to-use uniform declarations.

### Example Shader Snippet
```hlsl
#include "ListeningwayUniforms.fxh" // Optional helper

float4 main(float2 uv : TEXCOORD) : SV_Target
{
    float intensity = Listeningway_FreqBands[0] * 0.5 + Listeningway_Volume * 0.5 + Listeningway_Beat * 0.5;
    return float4(intensity, 0, 0, 1);
}
```

---

## Installation

1. Copy `dist\Listeningway.addon` to your ReShade directory (where your ReShade DLL is).
2. Place your shader (e.g., `Listeningway.fx`) in the ReShade shaders directory.
3. Place `ListeningwayUniforms.fxh` in your ReShade include directory (usually `reshade-shaders\Shaders`).
4. Launch your game. The addon will load automatically; enable your effect in the ReShade overlay.

---

## Tuning and Configuration

All tunable parameters (audio analysis, beat detection, UI layout, etc.) are loaded from `Listeningway.ini` in the same directory as the DLL. These are grouped in the `ListeningwaySettings` struct in the code, and loaded/saved atomically. Edit this file to change:
- Number of frequency bands, FFT size, smoothing, thresholds, normalization, etc.
- Beat detection and falloff behavior
- UI/overlay layout and appearance
- **CaptureStaleTimeout**: Time in seconds to wait before attempting to restart audio capture if no new audio is detected (default: 3.0)

**How it works:**
- On startup, the addon loads all tunables from `Listeningway.ini` into the `ListeningwaySettings` struct.
- If a key is missing, the default value from `constants.h` is used.
- When you change a setting in the overlay (such as the audio analysis toggle), it is saved to the .ini file and the struct is updated atomically.
- You can also edit the .ini file manually and restart the game/addon to apply changes.
- The `AudioAnalysisConfig` struct is used to pass settings to the analysis and capture modules, ensuring all tunables are grouped and consistent.
- Legacy global variables have been removed; all tunables are accessed via the settings struct.

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
CaptureStaleTimeout=3.0

[General]
AudioAnalysisEnabled=1
DebugEnabled=0
```

---

## Advanced: Building the Addon

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

---

## Libraries Used & Credits

| Library / API | Author / Project | Purpose |
|---------------|------------------|---------|
| [ReShade](https://github.com/crosire/reshade) | crosire | Core post-processing framework and SDK |
| [ImGui](https://github.com/ocornut/imgui) | Omar Cornut | Immediate-mode GUI for the overlay |
| [KissFFT](https://github.com/mborgerding/kissfft) | Mark Borgerding | Fast Fourier Transform library for audio analysis |
| [Microsoft WASAPI](https://docs.microsoft.com/en-us/windows/win32/coreaudio/windows-audio-session-api) | Microsoft | Windows audio capture API |

Special thanks to the ReShade community and contributors for documentation, support, and best practices.

---

## Notes

- All dependencies (including KissFFT) are statically linked; no extra DLLs are required in the game directory.
- If you encounter issues, check the ReShade log for errors.
- The number of frequency bands and available uniforms are managed in both the C++ code (see `settings.h` and `uniform_manager.*`) and your shader code. Update both if you want to change the number of bands or add new audio-driven uniforms.
- Doxygen-style documentation is available for developers. Run Doxygen with the provided `Doxyfile` in the `tools/reshade` directory to generate API docs.

---

## Architecture Overview

- **audio_capture.*:** WASAPI audio capture (threaded, uses `AudioAnalysisConfig`)
- **audio_analysis.*:** Real-time audio feature extraction (volume, bands, beat, uses `AudioAnalysisConfig`)
- **uniform_manager.*:** Updates all Listeningway uniforms in loaded effects
- **overlay.*:** ImGui debug overlay for real-time visualization
- **logging.*:** Thread-safe logging for diagnostics
- **listeningway_addon.cpp:** Main entry point and integration logic
- **settings.cpp/settings.h:** All tunable parameters are loaded from an .ini file and grouped in `ListeningwaySettings` struct
