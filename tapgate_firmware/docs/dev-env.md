# Development Environment Setup

This document covers toolchain installation and IDE configuration required before building the firmware. For build, flash, and test commands see [Build and Run](dev-proc.md).

---

## Toolchain Components

| Component | Required version | Notes |
|---|---|---|
| ESP-IDF | **v6.x** | primary installer — manages all components below |
| Xtensa GCC | 14.2+ | bundled |
| CMake | 3.24+ | bundled |
| Ninja | 1.10+ | bundled |
| Python | 3.9+ | bundled |
| Git | any recent | must be installed separately |

All compiler and build tools are managed by ESP-IDF — do not install them separately.

---

## Installing ESP-IDF

### Windows
Use the **ESP-IDF Tools Installer** — it installs ESP-IDF, the Xtensa toolchain, Python, CMake, Ninja, and configures all environment variables automatically.

→ [ESP-IDF Tools Installer (Windows)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/windows-setup.html)

### Linux / macOS
Use the official script-based installation:

→ [ESP-IDF Get Started — Linux / macOS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/linux-macos-setup.html)

Full reference: [ESP-IDF GitHub](https://github.com/espressif/esp-idf)

---

## Activating the Environment

Every new terminal session requires the ESP-IDF environment to be activated before running any `idf.py` command.

**Windows** — open **ESP-IDF PowerShell** or **ESP-IDF CMD** (installed shortcuts), or run:
```powershell
<esp-idf-path>\export.ps1
```

**Linux / macOS:**
```bash
. <esp-idf-path>/export.sh
```

Verify the active version:
```bash
idf.py --version
# Expected: ESP-IDF v6.x.x
```

---

## IDE Integration

### VS Code
Install the [ESP-IDF Extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) (`espressif.esp-idf-extension`), then run **ESP-IDF: Configure ESP-IDF Extension** and point it to your existing ESP-IDF installation.

Recommended additional extensions: `ms-vscode.cpptools`, `ms-vscode.cmake-tools`.

> **Corporate / offline networks:** download `.vsix` files from [marketplace.visualstudio.com](https://marketplace.visualstudio.com) and install via *Extensions → Install from VSIX*.

### CLion
Open the project as a CMake project. Set the CMake toolchain to the ESP-IDF CMake toolchain file:
```
<esp-idf-path>/tools/cmake/toolchain-esp32.cmake
```
Start CLion from the activated ESP-IDF terminal so it inherits the correct environment.

---

## Nanopb (Protobuf Code Generator)

The project uses [nanopb](https://github.com/nanopb/nanopb) for Protobuf serialization. The generator must be installed separately:

```bash
pip install protobuf grpcio-tools
```

Verify:
```bash
python esp32_mcu/components/nanopb/generator/nanopb_generator.py --version
```

Run the project setup script to automate dependency checks:

**Windows:**
```cmd
setup-dev-env.bat
```
**Linux / macOS:**
```bash
./setup-dev-env.sh
```

---

[← Back to ESP32 MCU Documentation](../README.md)
