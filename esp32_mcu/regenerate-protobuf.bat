@echo off
REM Script to regenerate protobuf files for ESP32 project
REM Based on nanopb documentation: user@host:~$ nanopb/generator/nanopb_generator.py message.proto

echo Regenerating protobuf files...

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

echo Script directory: %SCRIPT_DIR%

REM Set up absolute paths
set "PROTO_DIR=%SCRIPT_DIR%\..\proto"
set "PROTO_FILE=messages.proto"
set "OUTPUT_DIR=%SCRIPT_DIR%\messages"
set "NANOPB_GENERATOR=%SCRIPT_DIR%\components\nanopb\generator\nanopb_generator.py"

echo Proto directory: %PROTO_DIR%
echo Proto file: %PROTO_FILE%
echo Output directory: %OUTPUT_DIR%
echo Nanopb generator: %NANOPB_GENERATOR%

REM Check if files exist
if not exist "%PROTO_DIR%\%PROTO_FILE%" (
    echo Error: Proto file not found: %PROTO_DIR%\%PROTO_FILE%
    if not "%1"=="auto" pause
    exit /b 1
)

if not exist "%NANOPB_GENERATOR%" (
    echo Error: nanopb_generator.py not found: %NANOPB_GENERATOR%
    if not "%1"=="auto" pause
    exit /b 1
)

if not exist "%OUTPUT_DIR%" (
    echo Creating output directory: %OUTPUT_DIR%
    mkdir "%OUTPUT_DIR%"
)

REM Find Python from ESP-IDF
set "PYTHON_CMD="
if defined IDF_PYTHON_ENV_PATH (
    set "PYTHON_CMD=%IDF_PYTHON_ENV_PATH%\Scripts\python.exe"
    echo Found ESP-IDF Python: %PYTHON_CMD%
) else if defined PYTHON (
    set "PYTHON_CMD=%PYTHON%"
    echo Found PYTHON env var: %PYTHON_CMD%
) else (
    where python >nul 2>&1
    if %errorlevel% == 0 (
        set "PYTHON_CMD=python"
        echo Found system python
    ) else (
        echo Error: Python not found
        echo Please ensure ESP-IDF environment is activated
        if not "%1"=="auto" pause
        exit /b 1
    )
)

REM Test python
%PYTHON_CMD% --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Python not working: %PYTHON_CMD%
    if not "%1"=="auto" pause
    exit /b 1
)

REM Generate protobuf files using nanopb_generator.py directly
echo Generating messages.pb.h and messages.pb.c...
echo Command: %PYTHON_CMD% "%NANOPB_GENERATOR%" -I "%PROTO_DIR%" "%PROTO_FILE%"
echo Working directory: %OUTPUT_DIR%

cd /d "%OUTPUT_DIR%"
%PYTHON_CMD% "%NANOPB_GENERATOR%" -I "%PROTO_DIR%" "%PROTO_FILE%"

set "GENERATION_RESULT=%errorlevel%"

if %GENERATION_RESULT% == 0 (
    echo [OK] Protobuf files regenerated successfully!
    
    REM Check if files actually exist
    if exist "messages.pb.h" (
        echo ✓ messages.pb.h created
    ) else (
        echo ✗ messages.pb.h NOT found!
        set "GENERATION_RESULT=1"
    )
    
    if exist "messages.pb.c" (
        echo ✓ messages.pb.c created
    ) else (
        echo ✗ messages.pb.c NOT found!
        set "GENERATION_RESULT=1"
    )
    
    if %GENERATION_RESULT% == 0 (
        echo Files updated:
        dir "messages.pb.*" /b
    )
) else (
    echo [ERROR] Failed to regenerate protobuf files
    echo Generation command failed with error code: %GENERATION_RESULT%
    if not "%1"=="auto" pause
    exit /b 1
)

cd /d "%SCRIPT_DIR%"
echo You can now build the project with: idf.py build
if not "%1"=="auto" pause
exit /b %GENERATION_RESULT%