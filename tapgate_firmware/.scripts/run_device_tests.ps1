<#
.SYNOPSIS
    Run ESP32 device (on-target Unity) tests with the correct ESP-IDF Python venv.

.DESCRIPTION
    Wrapper around run_device_tests.py that:
      - Sets up the full ESP-IDF v6 environment (venv Python, toolchain paths)
      - Detects the serial port from .vscode/settings.json
      - Checks whether the board is reachable before starting
      - Passes remaining arguments through to run_device_tests.py

.EXAMPLE
    powershell -File .scripts/run_device_tests.ps1
    powershell -File .scripts/run_device_tests.ps1 -Filter "test_diag*"
#>

param(
    [string]$ProjectDir = "E:\Projects\ESP32\ESP32-TapGate\tapgate_firmware",
    [string]$Filter     = ""
)

# ── Set up ESP-IDF environment ───────────────────────────────────────────────────
. "$PSScriptRoot\_idf_env.ps1"

# ── Resolve serial port from .vscode/settings.json ──────────────────────────────
$vsSettings = Join-Path $ProjectDir ".vscode\settings.json"
$port       = "COM8"  # fallback

if (Test-Path $vsSettings) {
    try {
        $raw = Get-Content $vsSettings -Raw
        $raw = $raw -replace '//[^\n]*', ''           # strip // comments
        $raw = $raw -replace ',\s*([}\]])', '$1'       # strip trailing commas
        $cfg = $raw | ConvertFrom-Json
        $p = $cfg."idf.portWin"
        if (-not $p) { $p = $cfg."idf.port" }
        if ($p) { $port = $p }
    } catch {
        Write-Host "[WARN] Could not parse .vscode/settings.json, using default port $port" -ForegroundColor Yellow
    }
}

# ── Check that the board is actually connected ───────────────────────────────────
$availPorts = [System.IO.Ports.SerialPort]::GetPortNames()
if ($availPorts -notcontains $port) {
    Write-Host ""
    Write-Host "================================================================" -ForegroundColor Yellow
    Write-Host "  [WARNING]  Serial port $port is not available." -ForegroundColor Yellow
    Write-Host "================================================================" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Device tests require an ESP32 board connected via USB." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Please check:" -ForegroundColor Cyan
    Write-Host "    1. Is the ESP32 board plugged in?" -ForegroundColor Cyan
    Write-Host "    2. Is the COM port correct in .vscode/settings.json (idf.portWin)?" -ForegroundColor Cyan
    Write-Host "    3. Is the USB-Serial driver installed (CH340, CP210x, FTDI)?" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  Available ports: $($availPorts -join ', ')" -ForegroundColor Cyan
    Write-Host "  To list ports:   Get-WMIObject Win32_SerialPort | Select Name,DeviceID" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  If the board is connected and port detection fails, try OpenOCD:" -ForegroundColor Cyan
    Write-Host "  openocd -f board/esp32-wrover-kit-3.3v.cfg" -ForegroundColor Cyan
    Write-Host ""
    exit 1
}

Write-Host "[INFO] Board detected on $port" -ForegroundColor Green

# ── Run the tests ────────────────────────────────────────────────────────────────
Set-Location $ProjectDir

$runArgs = @()
if ($Filter) { $runArgs += $Filter }

& $script:IdfPython "$ProjectDir\run_device_tests.py" @runArgs
exit $LASTEXITCODE
