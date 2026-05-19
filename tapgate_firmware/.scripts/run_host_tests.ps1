<#
.SYNOPSIS
    Build and run ESP32 host-side Unity tests.

.DESCRIPTION
    Sets up the ESP-IDF v6 environment (cmake, ninja in PATH via _idf_env.ps1)
    then delegates to run_host_tests.py, which cleans, configures, builds, and
    runs the tests_host/ CMake project via ctest.

.EXAMPLE
    powershell -File .scripts/run_host_tests.ps1
#>

param(
    [string]$ProjectDir = "E:\Projects\ESP32\ESP32-TapGate\tapgate_firmware"
)

. "$PSScriptRoot\_idf_env.ps1"

Set-Location $ProjectDir
& $script:IdfPython "$ProjectDir\run_host_tests.py"
exit $LASTEXITCODE
