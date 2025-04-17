# Listeningway - FFXIV Audio Visualizer ReShade Addon

## Description

Listeningway is a ReShade addon that captures game audio and provides real-time volume and frequency band data for use in ReShade effects. It uses WASAPI for audio capture and KissFFT for analysis.

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

- All dependencies (including KissFFT) are now statically linked. No extra DLLs are required in the game directory.
- If you encounter issues, check the ReShade log for errors.

---

# Listeningway Addon Shader Integration Guide

To use real-time audio data (volume and frequency bands) from the Listeningway ReShade addon in your shaders, follow these steps:

## 1. Include the Uniform Declarations

Add the following to your shader file (e.g., at the top):

```hlsl
// Listeningway audio uniforms
uniform float Listeningway_Volume;
uniform float Listeningway_FreqBands[8];
```

- `Listeningway_Volume` is a float in the range [0, 1] representing the current audio volume.
- `Listeningway_FreqBands` is a float array (default size 8) where each element represents the normalized magnitude of a frequency band.

## 2. Use the Uniforms in Your Shader Code

You can use these uniforms in your pixel/vertex shaders to drive visual effects. Example:

```hlsl
float bar = Listeningway_FreqBands[band_index]; // Use band_index in [0,7]
float vol = Listeningway_Volume; // Use for global volume-based effects
```

## 3. (Optional) Customize Number of Bands

If you want a different number of bands, you must:
- Change the array size in both the shader and the C++ addon (LISTENINGWAY_NUM_BANDS).
- Rebuild the addon and update the shader accordingly.

## 4. Enable the Effect in ReShade

- Place your shader (e.g., `Listeningway.fx`) in the ReShade shaders directory.
- Enable the effect in the ReShade overlay in-game.

## 5. Example Shader Snippet

```hlsl
uniform float Listeningway_Volume;
uniform float Listeningway_FreqBands[8];

float4 main(float2 uv : TEXCOORD) : SV_Target
{
    float intensity = Listeningway_FreqBands[0] * 0.5 + Listeningway_Volume * 0.5;
    return float4(intensity, 0, 0, 1);
}
```

---

**Note:**  
- The addon will automatically find and update all uniforms named `Listeningway_Volume` and `Listeningway_FreqBands` in all loaded effects.
- No additional setup is required in the shader beyond declaring the uniforms.

For advanced usage or troubleshooting, refer to the Listeningway README or contact the addon maintainer.
