@echo off
REM Demon Synth Windows Installer Build Script
REM Requires: Inno Setup 6.x installed

echo =========================================
echo   Demon Synth Windows Installer Builder
echo =========================================

set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..\..
set BUILD_DIR=%PROJECT_ROOT%\build
set OUTPUT_DIR=%SCRIPT_DIR%..\Output

REM Check if Inno Setup is installed
set ISCC_PATH="C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not exist %ISCC_PATH% (
    echo Error: Inno Setup not found at %ISCC_PATH%
    echo Please install Inno Setup 6 from https://jrsoftware.org/isinfo.php
    pause
    exit /b 1
)

REM Check if build exists
if not exist "%BUILD_DIR%\NulyBeatsPlugin_artefacts\Release\VST3\Demon Synth.vst3" (
    echo Error: VST3 plugin not found. Please build first:
    echo   cd build
    echo   cmake --build . --config Release
    pause
    exit /b 1
)

REM Create output directory
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo Building installer...
%ISCC_PATH% "%SCRIPT_DIR%DemonSynth.iss"

if %ERRORLEVEL% neq 0 (
    echo Error: Installer build failed!
    pause
    exit /b 1
)

echo.
echo =========================================
echo   Installer created successfully!
echo   Output: %OUTPUT_DIR%\DemonSynth_v1.0.0_Windows.exe
echo =========================================
pause
