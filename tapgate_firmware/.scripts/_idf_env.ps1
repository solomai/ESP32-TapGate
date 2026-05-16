# Dot-source this script to set up the ESP-IDF v6 environment in any .ps1 helper.
# Usage inside another script: . "$PSScriptRoot\_idf_env.ps1"
# After sourcing, $script:IdfPython and $script:IdfPy are available.

# ── Remove MSYS / Git-Bash env vars that cause idf.py to abort ─────────────────
Remove-Item Env:MSYSTEM    -ErrorAction SilentlyContinue
Remove-Item Env:SHELL      -ErrorAction SilentlyContinue
Remove-Item Env:MSYS       -ErrorAction SilentlyContinue
Remove-Item Env:IDF_TARGET -ErrorAction SilentlyContinue   # governed by sdkconfig
Remove-Item Env:ESPPORT    -ErrorAction SilentlyContinue   # each script sets its own port

# ── ESP-IDF paths ───────────────────────────────────────────────────────────────
$env:IDF_PATH            = "C:\Espressif\.espressif\v6.0.1\esp-idf"
$env:IDF_TOOLS_PATH      = "C:\Espressif\tools"
$env:IDF_PYTHON_ENV_PATH = "C:\Espressif\tools\python\v6.0.1\venv"
$env:ESP_ROM_ELF_DIR     = "C:\Espressif\tools\esp-rom-elfs\20241011"
$env:ESP_IDF_VERSION     = "6.0"
$env:OPENOCD_SCRIPTS     = "C:\Espressif\tools\openocd-esp32\v0.12.0-esp32-20260304\openocd-esp32\share\openocd\scripts"
$env:IDF_COMPONENT_LOCAL_STORAGE_URL = "file://C:\Espressif\tools"

# ── Prepend toolchain directories to PATH ───────────────────────────────────────
$_idfToolPaths = @(
    "C:\Espressif\tools\ccache\4.12.1\ccache-4.12.1-windows-x86_64",
    "C:\Espressif\tools\cmake\4.0.3\bin",
    "C:\Espressif\tools\ninja\1.12.1",
    "C:\Espressif\tools\xtensa-esp-elf\esp-15.2.0_20251204\xtensa-esp-elf\bin",
    "C:\Espressif\tools\xtensa-esp-elf\esp-15.2.0_20251204\xtensa-esp-elf\xtensa-esp-elf\bin",
    "C:\Espressif\tools\riscv32-esp-elf\esp-15.2.0_20251204\riscv32-esp-elf\bin",
    "C:\Espressif\tools\python\v6.0.1\venv\Scripts",
    "C:\Espressif\tools\idf-exe\1.0.3",
    "C:\Espressif\tools\openocd-esp32\v0.12.0-esp32-20260304\openocd-esp32\bin"
)
$env:PATH = ($_idfToolPaths -join ";") + ";" + $env:PATH

# ── Exported helpers ─────────────────────────────────────────────────────────────
$script:IdfPython = "C:\Espressif\tools\python\v6.0.1\venv\Scripts\python.exe"
$script:IdfPy     = "C:\Espressif\.espressif\v6.0.1\esp-idf\tools\idf.py"
