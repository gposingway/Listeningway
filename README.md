# Listeningway - FFXIV Audio Visualizer ReShade Addon

## Description

Listeningway is a ReShade addon for Final Fantasy XIV (FFXIV) that captures game audio and provides real-time volume and frequency band data for use in ReShade effects. It uses WASAPI for audio capture and KissFFT for analysis.

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
