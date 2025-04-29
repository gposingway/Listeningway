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

REM Check if ReShade addon directory exists, create if not

REM Copy the addon file
echo Copying %ADDON_FILE% to %FFXIV_DIR%...
copy "%SOURCE_DIR%\%ADDON_FILE%" "%FFXIV_DIR%\" /Y
if !errorlevel! neq 0 (
    echo ERROR: Failed to copy file.
    exit /b 1
)

echo.
echo Listeningway successfully deployed to:
echo %FFXIV_DIR%\%ADDON_FILE%
echo.
echo --- Deployment Complete ---

endlocal