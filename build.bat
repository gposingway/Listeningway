@echo off
setlocal

REM === Generate ListeningwayUniforms.fxh from template ===
set NUM_BANDS=32
set TEMPLATE=ListeningwayUniforms.fxh.template
set OUTPUT=ListeningwayUniforms.fxh
if exist %TEMPLATE% (
    powershell -Command "(Get-Content %TEMPLATE%) -replace '\{\{NUM_BANDS\}\}', '%NUM_BANDS%' | Set-Content %OUTPUT%"
) else (
    echo Template %TEMPLATE% not found!
    exit /b 1
)

REM Build the project using MSBuild
if not exist build (
    echo Build directory not found. Run prepare.bat first.
    exit /b 1
)

REM Use Release config by default
set CONFIG=Release
if not "%1"=="" set CONFIG=%1

REM Build with MSBuild
msbuild build\Listeningway.sln /p:Configuration=%CONFIG% /m
if %errorlevel% neq 0 (
    echo Build failed!
    exit /b 1
)

REM Rename and move the output .dll to .addon in dist
set OUTDLL=build\%CONFIG%\Listeningway.dll
set OUTADDON=build\%CONFIG%\Listeningway.addon
set DIST=dist
if exist %OUTADDON% del %OUTADDON%
if exist %OUTDLL% ren %OUTDLL% Listeningway.addon
if not exist %DIST% mkdir %DIST%
move /Y build\%CONFIG%\Listeningway.addon %DIST%\Listeningway.addon

REM Copy Listeningway.fx to dist
copy /Y Listeningway.fx %DIST%\Listeningway.fx
REM Copy ListeningwayUniforms.fxh to dist
copy /Y ListeningwayUniforms.fxh %DIST%\ListeningwayUniforms.fxh

REM Extract FileVersion from listeningway.rc and write to dist/VERSION.txt using PowerShell (robust to spaces)
powershell -Command "Select-String 'FileVersion' listeningway.rc | ForEach-Object { if ($_.Line -match 'FileVersion\"\s*,\s*\"([^\"]+)\"') { $matches[1] } }" > %DIST%\VERSION.txt

REM No DLLs to copy for static build

echo --- Build, Rename, and Move Successful ---
endlocal
pause
