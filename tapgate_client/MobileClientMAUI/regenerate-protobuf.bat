@echo off
REM Script to regenerate C# protobuf files for MAUI project using NuGet protoc
REM Uses Grpc.Tools NuGet package for protoc

echo Regenerating C# protobuf files using NuGet protoc...

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"

echo Script directory: %SCRIPT_DIR%

REM Set up absolute paths
set "PROJECT_ROOT=%SCRIPT_DIR%\..\.."
set "PROTO_DIR=%PROJECT_ROOT%\proto"
set "PROTO_FILE=messages.proto"

echo Project root: %PROJECT_ROOT%
echo Proto directory: %PROTO_DIR%
echo Proto file: %PROTO_FILE%

REM Check if files exist
if not exist "%PROTO_DIR%\%PROTO_FILE%" (
    echo Error: Proto file not found: %PROTO_DIR%\%PROTO_FILE%
    if not "%1"=="auto" pause
    exit /b 1
)

REM Check if we have dotnet CLI available
where dotnet >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: dotnet CLI not found
    echo Please install .NET SDK
    if not "%1"=="auto" pause
    exit /b 1
)

echo Using dotnet version:
dotnet --version

REM First restore packages to get protoc from NuGet
echo Restoring NuGet packages to get protoc...
cd /d "%SCRIPT_DIR%"
dotnet restore

if %errorlevel% neq 0 (
    echo Error: Failed to restore NuGet packages
    if not "%1"=="auto" pause
    exit /b 1
)

REM Find protoc in NuGet packages - look for Grpc.Tools
set "PROTOC_CMD="
for /f "delims=" %%i in ('dir /s /b "%USERPROFILE%\.nuget\packages\grpc.tools\*\tools\windows_x64\protoc.exe" 2^>nul') do (
    set "PROTOC_CMD=%%i"
    goto :found_protoc
)

for /f "delims=" %%i in ('dir /s /b "%USERPROFILE%\.nuget\packages\grpc.tools\*\tools\windows_x86\protoc.exe" 2^>nul') do (
    set "PROTOC_CMD=%%i"
    goto :found_protoc
)

:found_protoc
if defined PROTOC_CMD (
    echo Found NuGet protoc at: %PROTOC_CMD%
) else (
    echo Error: protoc not found in NuGet packages
    echo Try running: dotnet restore
    if not "%1"=="auto" pause
    exit /b 1
)

REM Test protoc
"%PROTOC_CMD%" --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: protoc not working: %PROTOC_CMD%
    if not "%1"=="auto" pause
    exit /b 1
)

REM Create output directory (MSBuild will manage this automatically)
set "OUTPUT_DIR=%SCRIPT_DIR%\obj\Debug\net9.0-android\Protos"
if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
)

REM Clean old generated files
echo Cleaning old generated files...
del /q "%OUTPUT_DIR%\*.cs" 2>nul
del /q "%OUTPUT_DIR%\*.g.cs" 2>nul

REM Generate C# files from proto using NuGet protoc
echo Generating C# classes from messages.proto...
echo Command: "%PROTOC_CMD%" --proto_path="%PROTO_DIR%" --csharp_out="%OUTPUT_DIR%" --csharp_opt=file_extension=.g.cs "%PROTO_DIR%\%PROTO_FILE%"

"%PROTOC_CMD%" ^
    --proto_path="%PROTO_DIR%" ^
    --csharp_out="%OUTPUT_DIR%" ^
    --csharp_opt=file_extension=.g.cs ^
    "%PROTO_DIR%\%PROTO_FILE%"

set "GENERATION_RESULT=%errorlevel%"

if %GENERATION_RESULT% == 0 (
    echo [OK] C# protobuf files generated successfully!
    echo Files generated in: %OUTPUT_DIR%
    
    REM List generated files
    echo Generated files:
    dir "%OUTPUT_DIR%\*.g.cs" /b 2>nul
    if %errorlevel% neq 0 (
        echo No .g.cs files found, checking for .cs files:
        dir "%OUTPUT_DIR%\*.cs" /b 2>nul
    )
    
    REM Copy to Core/Messages for easier access
    if exist "%OUTPUT_DIR%\Messages.g.cs" (
        if not exist "%SCRIPT_DIR%\Core\Messages" mkdir "%SCRIPT_DIR%\Core\Messages"
        copy "%OUTPUT_DIR%\Messages.g.cs" "%SCRIPT_DIR%\Core\Messages\" >nul
        echo [OK] Files copied to Core/Messages/
    )
    
    echo.
    echo Note: MSBuild will automatically generate these files during build.
    echo Manual generation is only needed for development/debugging.
) else (
    echo [ERROR] Failed to generate C# protobuf files
    echo Generation command failed with error code: %GENERATION_RESULT%
    if not "%1"=="auto" pause
    exit /b 1
)

echo You can now build the MAUI project with: dotnet build
if not "%1"=="auto" pause
exit /b %GENERATION_RESULT%