@echo off
REM Quick setup script for ESP32 MCU protobuf dependencies only
REM Use this if you only need to work with ESP32 part

echo ESP32 TapGate - Quick ESP32 Setup
echo Installing Python protobuf dependencies...

REM Install dependencies
pip install protobuf grpcio-tools
if %errorlevel% neq 0 (
    echo Error installing dependencies
    pause
    exit /b 1
)

REM Test nanopb
echo Testing nanopb generator...
cd components\nanopb\generator
python nanopb_generator.py --version
if %errorlevel% neq 0 (
    echo Error: nanopb generator test failed
    cd ..\..\..
    pause
    exit /b 1
)
cd ..\..\..

echo [OK] ESP32 protobuf setup complete!
echo You can now build the ESP32 project.
pause