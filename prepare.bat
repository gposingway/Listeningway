@echo off
cd /d %~dp0 REM Change directory to the script's location
setlocal

REM --- Configuration ---
set TOOLS_DIR=tools
set RESHADE_DIR=%TOOLS_DIR%\reshade
set VCPKG_DIR=%TOOLS_DIR%\vcpkg
set RESHADE_REPO_URL=https://github.com/crosire/reshade.git
set VCPKG_REPO_URL=https://github.com/microsoft/vcpkg.git
set BUILD_DIR=build
set CMAKE_GENERATOR="Visual Studio 17 2022" REM Adjust if needed
set CMAKE_PLATFORM=x64
set VCPKG_TOOLCHAIN_FILE=%~dp0%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake
set RESHADE_SDK_INCLUDE_PATH=%~dp0%RESHADE_DIR%\include

REM --- Preparation Steps ---

echo Ensuring tools directory exists...
if not exist "%TOOLS_DIR%" (
    echo Creating %TOOLS_DIR% directory...
    mkdir "%TOOLS_DIR%"
    if errorlevel 1 (
        echo Failed to create %TOOLS_DIR% directory.
        goto failure
    )
)

echo Checking for ReShade repository...
if not exist "%RESHADE_DIR%\.git" (
    echo ReShade repository not found. Cloning from %RESHADE_REPO_URL%...
    git clone --depth 1 %RESHADE_REPO_URL% "%RESHADE_DIR%"
    if errorlevel 1 (
        echo Failed to clone ReShade repository. Please check Git installation and network connection.
        goto failure
    )
    echo ReShade repository cloned successfully.
) else (
    echo ReShade repository found at %RESHADE_DIR%.
)

echo Checking for vcpkg repository...
if not exist "%VCPKG_DIR%\.git" (
    echo vcpkg repository not found. Cloning from %VCPKG_REPO_URL%...
    git clone --depth 1 %VCPKG_REPO_URL% "%VCPKG_DIR%"
    if errorlevel 1 (
        echo Failed to clone vcpkg repository. Please check Git installation and network connection.
        goto failure
    )
    echo vcpkg repository cloned successfully.
) else (
    echo vcpkg repository found at %VCPKG_DIR%.
)

echo Checking for vcpkg executable...
if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo vcpkg.exe not found. Running bootstrap script...
    call "%VCPKG_DIR%\bootstrap-vcpkg.bat" -disableMetrics
    if errorlevel 1 (
        echo Failed to bootstrap vcpkg.
        goto failure
    )
    echo vcpkg bootstrapped successfully.
) else (
    echo vcpkg.exe found.
)

echo Configuring CMake project...
if exist "%BUILD_DIR%" (
    echo Removing existing build directory for clean configuration...
    rmdir /s /q "%BUILD_DIR%"
    if errorlevel 1 (
        echo Failed to remove existing build directory. Please close any programs using files within it.
        goto failure
    )
)
cmake -S . -B "%BUILD_DIR%" -G %CMAKE_GENERATOR% -A %CMAKE_PLATFORM% -DRESHADE_SDK_PATH="%RESHADE_SDK_INCLUDE_PATH%" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%"
if errorlevel 1 (
    echo CMake configuration failed!
    goto failure
)

echo.
echo --- Preparation Successful ---
echo Project is ready to be built (e.g., using simple_build.bat or opening build/Listeningway.sln).
goto end

:failure
echo.
echo --- Preparation Failed ---

:end
endlocal
pause
