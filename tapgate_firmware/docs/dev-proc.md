# Build and Run

Requires ESP-IDF v6+ with an activated environment. See [Development Environment Setup](dev-env.md) if not yet configured.

---

## Contents
- [Device Connection](#device-connection)
- [Build and Flash](#build-and-flash)
- [Testing](#testing)

---

## Device Connection

Two connection methods are supported:

### 1. Direct USB (default)
Connect the ESP32-WROOM-32 dev board directly to your host machine via USB. The board exposes a USB-to-UART bridge; the COM port is auto-assigned by the OS.

Set the port in `.vscode/settings.json`:
```json
"idf.portWin": "COMx"
```

### 2. ESP-PROG via JTAG
Connect via an [ESP-PROG](esp-prog.md) programmer. The ESP32 is powered and programmed through the JTAG interface — no direct USB connection to the board is needed.

This method also enables hardware debugging (breakpoints, register inspection) via OpenOCD.

See [ESP-PROG Setup](esp-prog.md) for wiring and driver installation.

---

## Build and Flash

Open **ESP-IDF PowerShell** or activate the environment manually, then launch your IDE from the same terminal.

```bash
# Navigate to the firmware directory
cd path\to\ESP32-TapGate\tapgate_firmware

# Full rebuild
idf.py fullclean
idf.py reconfigure
idf.py build

# Build, flash, and open monitor in one step
idf.py build flash monitor
```

---

## Testing

The project has two test categories with separate build targets and runners.

### Host Tests

Run directly on your development machine using the host C++ compiler. No hardware required.

- Source: `tests_host/`
- Runner: `run_host_tests.py` — configures CMake, builds, and runs via CTest

```bash
python run_host_tests.py
```

### Device Tests

Compiled for ESP32 and executed on the hardware. The runner builds each test group, flashes it, and collects Unity output over serial.

- Source: `tests/`
- Runner: `run_device_tests.py`
- COM port is read automatically from `.vscode/settings.json`

**Prerequisites:**

1. Install Python dependencies (one time):
   ```cmd
   tests\install_tests_deps.bat
   ```
2. **Close the Device Monitor** — the serial port must be free for the runner to communicate with the board.
3. **If using ESP-PROG:** OpenOCD must be running before launching the script (the runner flashes via `idf.py flash` which uses JTAG).

```bash
# Run all test groups
python run_device_tests.py

# Run a specific group
python run_device_tests.py test_diagnostics

# Run by name mask
python run_device_tests.py "test_comp*"
```

---

[← Back to ESP32 MCU Documentation](../README.md)
