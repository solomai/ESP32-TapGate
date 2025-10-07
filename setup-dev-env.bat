@echo off
REM ESP32 TapGate - Development Environment Setup Script (Windows)
REM This script sets up all dependencies required for building the project

setlocal enabledelayedexpansion

echo === ESP32 TapGate - Development Environment Setup ===
echo.

REM Function to check if command exists
set "PYTHON_CMD="
set "PIP_CMD="

echo Checking system requirements...
echo.

REM Check Python 3
echo [INFO] Checking Python 3...
python --version >nul 2>&1
if %errorlevel% == 0 (
    for /f "tokens=2" %%i in ('python --version 2^>^&1') do set PYTHON_VERSION=%%i
    echo !PYTHON_VERSION! | findstr /r "^3\." >nul
    if !errorlevel! == 0 (
        echo [OK] Python 3 found: !PYTHON_VERSION!
        set "PYTHON_CMD=python"
    ) else (
        echo [ERROR] Python 3 required, found Python !PYTHON_VERSION!
        echo Please install Python 3.7 or later from https://www.python.org/
        pause
        exit /b 1
    )
) else (
    python3 --version >nul 2>&1
    if !errorlevel! == 0 (
        for /f "tokens=2" %%i in ('python3 --version 2^>^&1') do set PYTHON_VERSION=%%i
        echo [OK] Python 3 found: !PYTHON_VERSION!
        set "PYTHON_CMD=python3"
    ) else (
        echo [ERROR] Python 3 not found
        echo Please install Python 3.7 or later from https://www.python.org/
        pause
        exit /b 1
    )
)

REM Check pip
echo [INFO] Checking pip...
pip --version >nul 2>&1
if %errorlevel% == 0 (
    echo [OK] pip found
    set "PIP_CMD=pip"
) else (
    pip3 --version >nul 2>&1
    if !errorlevel! == 0 (
        echo [OK] pip3 found
        set "PIP_CMD=pip3"
    ) else (
        echo [ERROR] pip not found
        echo Please ensure pip is installed with Python
        pause
        exit /b 1
    )
)

REM Check Git
echo [INFO] Checking Git...
git --version >nul 2>&1
if %errorlevel% == 0 (
    for /f "tokens=3" %%i in ('git --version') do set GIT_VERSION=%%i
    echo [OK] Git found: !GIT_VERSION!
) else (
    echo [ERROR] Git not found
    echo Please install Git from https://git-scm.com/
    pause
    exit /b 1
)

REM Check CMake
echo [INFO] Checking CMake...
cmake --version >nul 2>&1
if %errorlevel% == 0 (
    for /f "tokens=3" %%i in ('cmake --version ^| findstr /r "^cmake"') do set CMAKE_VERSION=%%i
    echo [OK] CMake found: !CMAKE_VERSION!
) else (
    echo [WARNING] CMake not found - required for building
    echo Please install CMake 3.20 or later from https://cmake.org/
)

echo.
echo Installing Python dependencies...

REM Install protobuf dependencies
echo [INFO] Installing protobuf and grpcio-tools...
%PIP_CMD% install protobuf grpcio-tools
if %errorlevel% == 0 (
    echo [OK] Python protobuf dependencies installed
) else (
    echo [ERROR] Failed to install Python dependencies
    pause
    exit /b 1
)

REM Test nanopb generator
echo.
echo [INFO] Testing nanopb generator...
cd esp32_mcu\components\nanopb\generator
%PYTHON_CMD% nanopb_generator.py --version >nul 2>&1
if %errorlevel% == 0 (
    for /f %%i in ('%PYTHON_CMD% nanopb_generator.py --version 2^>^&1') do set NANOPB_VERSION=%%i
    echo [OK] nanopb generator working: !NANOPB_VERSION!
) else (
    echo [ERROR] nanopb generator test failed
    cd ..\..\..
    pause
    exit /b 1
)
cd ..\..\..

REM Check ESP-IDF (optional)
echo.
echo [INFO] Checking ESP-IDF environment...
if defined IDF_PATH (
    if exist "%IDF_PATH%\tools\cmake\project.cmake" (
        echo [OK] ESP-IDF found at: %IDF_PATH%
    ) else (
        echo [WARNING] ESP-IDF path set but invalid: %IDF_PATH%
        goto esp_idf_warning
    )
) else (
    :esp_idf_warning
    echo [WARNING] ESP-IDF not found or not configured
    echo For ESP32 development, please install ESP-IDF from:
    echo https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
)

REM Test host build
echo.
echo [INFO] Testing host build...
cd esp32_mcu
set TAPGATE_BUILD_MODE=host
cmake -S . -B build_setup_test >nul 2>&1
if %errorlevel% == 0 (
    cmake --build build_setup_test >nul 2>&1
    if !errorlevel! == 0 (
        echo [OK] Host build test successful
        rmdir /s /q build_setup_test >nul 2>&1
    ) else (
        echo [ERROR] Host build test failed
        rmdir /s /q build_setup_test >nul 2>&1
        cd ..
        pause
        exit /b 1
    )
) else (
    echo [ERROR] Host build configuration failed
    rmdir /s /q build_setup_test >nul 2>&1
    cd ..
    pause
    exit /b 1
)
cd ..

echo.
echo === Setup completed successfully! ===
echo.
echo Next steps:
echo 1. For ESP32 development: Install and configure ESP-IDF
echo 2. Build ESP32 project: cd esp32_mcu ^&^& idf.py build
echo 3. Build host tests: cd esp32_mcu ^&^& set TAPGATE_BUILD_MODE=host ^&^& cmake -S . -B build_host ^&^& cmake --build build_host
echo 4. For MAUI client: Open tapgate_client\TapGateClient.sln in Visual Studio
echo.
pause