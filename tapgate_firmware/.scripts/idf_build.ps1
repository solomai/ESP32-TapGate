<#
.SYNOPSIS
    Build / flash / monitor ESP32 firmware via idf.py with the correct ESP-IDF environment.

.DESCRIPTION
    Wrapper around idf.py that sets up the full ESP-IDF v6 environment
    (venv Python, toolchain paths, MSYS workaround) and then forwards
    the command to idf.py.

.EXAMPLE
    powershell -File .scripts/idf_build.ps1                      # build
    powershell -File .scripts/idf_build.ps1 -Cmd "fullclean"     # clean
    powershell -File .scripts/idf_build.ps1 -Cmd "-p COM8 flash" # flash
    powershell -File .scripts/idf_build.ps1 -Cmd "size"          # firmware size
#>

param(
    [string]$ProjectDir = "E:\Projects\ESP32\ESP32-TapGate\tapgate_firmware",
    [string]$Cmd        = "build"
)

. "$PSScriptRoot\_idf_env.ps1"

Set-Location $ProjectDir
$argList = $Cmd -split '\s+'
& $script:IdfPython $script:IdfPy @argList
exit $LASTEXITCODE
