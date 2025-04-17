@echo off
cd /d %~dp0 REM Change directory to the script's location
setlocal

REM --- Configuration ---
set BUILD_DIR=build
set CMAKE_GENERATOR="Visual Studio 17 2022" REM Adjust if you use a different VS version
set CMAKE_PLATFORM=x64
set BUILD_CONFIG=Release
set DLL_NAME=Listeningway.dll
set ADDON_NAME=Listeningway.addon
set OUTPUT_SUBDIR=%BUILD_CONFIG%
set OUTPUT_DLL_PATH=%BUILD_DIR%\%OUTPUT_SUBDIR%\%DLL_NAME%
set OUTPUT_ADDON_PATH=%BUILD_DIR%\%OUTPUT_SUBDIR%\%ADDON_NAME%
set VCPKG_TOOLCHAIN_FILE=%~dp0tools\vcpkg\scripts\buildsystems\vcpkg.cmake
set RESHADE_SDK_INCLUDE_PATH=%~dp0tools\reshade\include
set DIST_DIR=%~dp0dist

REM --- Build Steps ---

REM Check if CMake configuration is needed
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo CMakeCache.txt not found. Running CMake configuration...
    cmake -S . -B "%BUILD_DIR%" -G %CMAKE_GENERATOR% -A %CMAKE_PLATFORM% -DRESHADE_SDK_PATH="%RESHADE_SDK_INCLUDE_PATH%" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_TOOLCHAIN_FILE%"
    if errorlevel 1 (
        echo CMake configuration failed!
        goto failure
    )
) else (
    echo CMakeCache.txt found. Skipping configuration.
)

echo Building project (%BUILD_CONFIG%)...
cmake --build "%BUILD_DIR%" --config %BUILD_CONFIG%
if errorlevel 1 (
    echo Build failed!
    goto failure
)

echo Attempting to delete existing addon file: %OUTPUT_ADDON_PATH%
if exist "%OUTPUT_ADDON_PATH%" (
    del /F /Q "%OUTPUT_ADDON_PATH%"
    if errorlevel 1 (
        echo WARNING: Failed to delete existing %ADDON_NAME%. Rename might fail.
    ) else (
        echo Successfully deleted existing %ADDON_NAME%.
    )
    REM Add a check after delete
    if exist "%OUTPUT_ADDON_PATH%" (
        echo ERROR: %ADDON_NAME% still exists after delete command! Rename will likely fail.
        goto failure
    ) else (
        echo Confirmed %ADDON_NAME% does not exist before rename.
    )
) else (
    echo No existing %ADDON_NAME% found to delete.
)

echo Renaming DLL to ADDON...
if exist "%OUTPUT_DLL_PATH%" (
    echo Renaming %OUTPUT_DLL_PATH% to %ADDON_NAME%
    ren "%OUTPUT_DLL_PATH%" "%ADDON_NAME%"
    if errorlevel 1 (
        echo Failed to rename %DLL_NAME% to %ADDON_NAME%. Check if the file is in use or permissions.
        goto failure
    )
    echo Successfully renamed to %ADDON_NAME% in %BUILD_DIR%\%OUTPUT_SUBDIR%
) else (
    echo %OUTPUT_DLL_PATH% not found! Cannot rename. Build might have failed silently.
    goto failure
)

echo Moving addon to dist folder...
if not exist "%DIST_DIR%" (
    echo Creating dist directory: %DIST_DIR%
    mkdir "%DIST_DIR%"
    if errorlevel 1 (
        echo Failed to create dist directory.
        goto failure
    )
)
if exist "%OUTPUT_ADDON_PATH%" (
    echo Moving %ADDON_NAME% to %DIST_DIR%...
    move /Y "%OUTPUT_ADDON_PATH%" "%DIST_DIR%"
    if errorlevel 1 (
        echo Failed to move addon to %DIST_DIR%.
        goto failure
    )
    echo Successfully moved %ADDON_NAME% to %DIST_DIR%.
) else (
    echo %OUTPUT_ADDON_PATH% not found! Cannot move.
    goto failure
)

echo.
echo --- Build, Rename, and Move Successful ---
goto end

:failure
echo.
echo --- Script Failed ---

:end
endlocal
pause
