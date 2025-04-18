cmake_minimum_required(VERSION 3.15) # vcpkg recommends at least 3.15

project(Listeningway LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer standard features

# --- ReShade SDK ---
# Option 1: Set this variable via CMake command line or GUI
# cmake .. -DRESHADE_SDK_PATH=/path/to/reshade/sdk/include
option(RESHADE_SDK_PATH "Path to the ReShade SDK include directory" "")
if(NOT RESHADE_SDK_PATH OR NOT EXISTS "${RESHADE_SDK_PATH}/reshade.hpp")
    message(FATAL_ERROR "ReShade SDK path not found or invalid. Please set RESHADE_SDK_PATH correctly (e.g., path to the 'include' directory containing reshade.hpp). Current value: '${RESHADE_SDK_PATH}'")
endif()
message(STATUS "Using ReShade SDK from: ${RESHADE_SDK_PATH}")
include_directories(${RESHADE_SDK_PATH})

# --- Project Configuration ---
# Add source file(s) including the resource file
add_library(${PROJECT_NAME} SHARED
    listeningway_addon.cpp
    audio_capture.cpp
    audio_analysis.cpp
    overlay.cpp
    logging.cpp
    uniform_manager.cpp
    settings.cpp
    listeningway.rc
)

# Add ImGui include directory specifically for the Listeningway target
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/tools/reshade/deps/imgui
)

# --- Preprocessor Definitions ---
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN # Exclude rarely-used stuff from Windows headers
    NOMINMAX          # Prevent Windows headers from defining min/max macros
    # Add other necessary definitions here
)

# --- Dependencies ---
# Find vcpkg packages (requires CMAKE_TOOLCHAIN_FILE to be set)
find_package(KissFFT CONFIG REQUIRED) # Find package without components

# Link Windows Libraries needed for WASAPI/COM
target_link_libraries(${PROJECT_NAME} PRIVATE
    Ole32           # For COM
    Propsys         # For IMMDeviceEnumerator property store access (safer)
    Avrt            # For audio thread priority management (optional but good)
    # Add other Windows libs if needed (e.g., Winmm)
)

# Link vcpkg libraries
# Use kissfft-float linkage
target_link_libraries(${PROJECT_NAME} PRIVATE
    kissfft::kissfft-float
)

# --- Output Configuration ---
# Ensure the output DLL is named correctly (e.g., Listeningway.dll)
# CMake usually handles this based on the project name, but you can force it:
# set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "Listeningway")

# Optional: Set output directory for the DLL if desired
# set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message(STATUS "Configuring Listeningway ReShade Addon")
