@echo off
setlocal enabledelayedexpansion

echo --- Listeningway Deployment Script ---

REM Define source and destination paths
set "SOURCE_DIR=F:\Files\repo\github\Listeningway\dist"
set "FFXIV_DIR=E:\Games\SquareEnix\FINAL FANTASY XIV - A Realm Reborn\game"
set "ADDON_FILE=listeningway.addon"

REM Check if source file exists
if not exist "%SOURCE_DIR%\%ADDON_FILE%" (
    echo ERROR: Source file %SOURCE_DIR%\%ADDON_FILE% not found.
    echo Make sure you've built the project successfully before running this script.
    exit /b 1
)

REM Check if FFXIV directory exists
if not exist "%FFXIV_DIR%" (
    echo WARNING: FFXIV directory not found at %FFXIV_DIR%
    
    REM Ask user if they want to create the directory
    set /p CREATE_DIR="Would you like to create the directory? (Y/N): "
    if /i "!CREATE_DIR!" == "Y" (
        mkdir "%FFXIV_DIR%"
        if !errorlevel! neq 0 (
            echo ERROR: Failed to create directory.
            exit /b 1
        )
        echo Created directory: %FFXIV_DIR%
    ) else (
        echo Deployment cancelled.
        exit /b 1
    )
)

REM Check if ReShade shader directory exists, create if not
set "SHADER_DIR=%FFXIV_DIR%\reshade-shaders\Shaders"
if not exist "%SHADER_DIR%" (
    echo Creating ReShade shader directory: %SHADER_DIR%
    mkdir "%SHADER_DIR%"
    if !errorlevel! neq 0 (
        echo ERROR: Failed to create shader directory.
        exit /b 1
    )
)

REM Copy the addon file
echo Copying %ADDON_FILE% to %FFXIV_DIR%...
copy "%SOURCE_DIR%\%ADDON_FILE%" "%FFXIV_DIR%\" /Y
if !errorlevel! neq 0 (
    echo WARNING: Failed to copy %ADDON_FILE%.
)

REM Copy Listeningway.fx to game directory
copy /Y "%SOURCE_DIR%\Listeningway.fx" "%FFXIV_DIR%\reshade-shaders\shaders\Listeningway.fx" || echo WARNING: Failed to copy Listeningway.fx.
REM Copy ListeningwayUniforms.fxh to game directory
copy /Y "%SOURCE_DIR%\ListeningwayUniforms.fxh" "%FFXIV_DIR%\reshade-shaders\shaders\ListeningwayUniforms.fxh" || echo WARNING: Failed to copy ListeningwayUniforms.fxh.

echo.
echo Listeningway successfully deployed to:
echo %FFXIV_DIR%\%ADDON_FILE%
echo.
echo --- Deployment Complete ---

endlocal