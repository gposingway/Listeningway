# Listeningway - FFXIV Audio Visualizer ReShade Addon

## Description

Listeningway is a ReShade addon designed for Final Fantasy XIV (FFXIV). It captures the game's audio output and provides data (like overall volume and frequency bands) that can be used by ReShade effects (`.fx` files) to create audio-reactive visuals.

This addon uses WASAPI for audio loopback capture and KissFFT for Fast Fourier Transform analysis.

## Prerequisites

Before building, ensure you have the following installed:

1.  **Git:** Required for cloning dependencies. ([https://git-scm.com/](https://git-scm.com/))
2.  **CMake:** Required for generating build files. ([https://cmake.org/](https://cmake.org/)) Make sure CMake is added to your system's PATH.
3.  **Visual Studio:** Required for compiling the C++ code. Install Visual Studio (e.g., 2022 Community Edition) with the "Desktop development with C++" workload. ([https://visualstudio.microsoft.com/](https://visualstudio.microsoft.com/))

## Building

This project uses batch scripts to simplify the setup and build process.

1.  **Prepare Dependencies (`prepare.bat`):**
    *   Open a command prompt or terminal in the project's root directory.
    *   Run the preparation script:
        ```bash
        .\prepare.bat
        ```
    *   This script will:
        *   Create a `tools` directory.
        *   Clone the necessary versions of the ReShade SDK and vcpkg into the `tools` directory if they don't exist.
        *   Bootstrap vcpkg (compile `vcpkg.exe`) if needed.
        *   Run CMake to configure the project and generate build files (e.g., a Visual Studio solution) in the `build` directory.

2.  **Build the Addon (`simple_build.bat`):**
    *   After running `prepare.bat` successfully, run the build script:
        ```bash
        .\simple_build.bat
        ```
    *   This script will:
        *   Compile the addon code in `Release` configuration using the previously generated build files.
        *   Rename the output `Listeningway.dll` to `Listeningway.addon`.
        *   Move the `Listeningway.addon` file into a `dist` directory in the project root.

## Installation

1.  Locate the `Listeningway.addon` file inside the `dist` directory after a successful build.
2.  Copy `Listeningway.addon` to your FFXIV ReShade addons directory. This is typically the same directory where your ReShade preset (`.ini`) and shaders (`reshade-shaders` folder) are located within the FFXIV `game` folder.
    *   *Example Path:* `C:\Program Files (x86)\SquareEnix\FINAL FANTASY XIV - A Realm Reborn\game\`
    *   *Note:* ReShade looks for `.addon` files in the same directory as the ReShade DLL (`dxgi.dll`, `d3d11.dll`, etc.) by default.
3.  Ensure the accompanying `Listeningway.fx` shader file is placed within your ReShade shaders directory (e.g., `reshade-shaders\Shaders`).
4.  Launch FFXIV. The addon should be loaded automatically by ReShade. You can enable the `Listeningway.fx` effect in the ReShade overlay.
