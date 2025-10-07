@echo off
REM Script to regenerate protobuf files for ESP32 project
REM Run this script whenever proto files change

echo Regenerating protobuf files...

REM Navigate to nanopb generator directory
cd components\nanopb\generator

REM Check if Python and nanopb are available
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Python not found
    pause
    exit /b 1
)

python nanopb_generator.py --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: nanopb generator not working
    echo Run setup script first: ..\quick-setup.bat
    pause
    exit /b 1
)

REM Generate protobuf files
echo Generating messages.pb.h and messages.pb.c...
python nanopb_generator.py -D "%~dp0messages" -f "%~dp0..\proto\messages.options" -I "%~dp0..\proto" "%~dp0..\proto\messages.proto"

if %errorlevel% == 0 (
    echo [OK] Protobuf files regenerated successfully!
    echo Files updated:
    echo   - messages\messages.pb.h
    echo   - messages\messages.pb.c
) else (
    echo [ERROR] Failed to regenerate protobuf files
    pause
    exit /b 1
)

cd ..\..
echo You can now build the project with: idf.py build
pause