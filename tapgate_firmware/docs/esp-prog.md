# ESP-PROG Setup Guide

ESP-PROG is a dedicated programmer/debugger from Espressif that connects to the ESP32 via JTAG. It enables flashing, hardware debugging (breakpoints, register inspection), and powers the board — no direct USB connection to the ESP32 is required.

---

## Required Hardware

- ESP32-WROOM-32 development board
- ESP-PROG programmer
- USB cable (for ESP-PROG → host)
- 6× male-to-female jumper wires
- USB hub (optional, useful when both devices need simultaneous host connection)

---

## JTAG Wiring

ESP-PROG exposes a 10-pin JTAG header. Wire it to the ESP32 as shown below.

<img src="assets/ESP-PROG-JTAG.png" width="360">

| ESP-PROG pin | Signal | ESP32-WROOM-32 pin |
|---|---|---|
| 1 | VDD | 3V3 |
| 5 | TDI | GPIO12 |
| 7 | TCK | GPIO13 |
| 8 | TMS | GPIO14 |
| 9 | TDO | GPIO15 |
| 6 / 10 | GND | GND |

> **Voltage level:** ESP-PROG supports 3.3 V and 5 V. Ensure the JTAG header jumper is set to **3.3 V** — JTAG will not work at 5 V unless VDD is remapped to the ESP32's 5 V pin. See [Pin Headers](https://docs.espressif.com/projects/espressif-esp-iot-solution/en/latest/hw-reference/ESP-Prog_guide.html#pin-headers) for details.

---

## Driver Installation (Windows)

ESP-PROG uses FTDI-based USB. On Windows, replace the default driver with WinUSB using **Zadig**:

→ [Zadig — USB driver installer](https://zadig.akeo.ie/)

Select the ESP-PROG interface and install the WinUSB driver. This is required for OpenOCD to communicate with the device.

---

## OpenOCD

OpenOCD is bundled with ESP-IDF and is available once the environment is activated.

Start the OpenOCD server for ESP32:
```bash
openocd -f interface/ftdi/esp32_devkitj_v1.cfg -f target/esp32.cfg
```

Leave it running in a separate terminal. VS Code and `idf.py` will connect to it automatically.

---

## Debugging in VS Code

1. Start the OpenOCD server (see above).
2. Flash the firmware if needed: `idf.py flash`
3. Open **Run and Debug** (`Ctrl+Shift+D`) and select the **ESP32** configuration.
4. Click **Attach to remote process** — VS Code connects via GDB over OpenOCD.

Debug settings are controlled by `Kconfig.projbuild`:

| Option | Description |
|---|---|
| `APP_DEBUG_MODE` | Enable debug mode in firmware |
| `APP_WAIT_FOR_DEBUGGER` | Halt on boot until debugger attaches |
| `APP_WAIT_FOR_DEBUGGER_TIMEOUT` | Timeout (seconds) for debugger connection |

---

## References

- [Introduction to the ESP-Prog Board](https://docs.espressif.com/projects/esp-iot-solution/en/latest/hw-reference/ESP-Prog_guide.html) — Espressif official guide
- [JTAG Debugging with ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/jtag-debugging/index.html) — Espressif official guide
- [Debug Your Project — VS Code Extension](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/debugproject.html)
- [ESP32-With-ESP-PROG-Demo](https://github.com/PBearson/ESP32-With-ESP-PROG-Demo) — community demo project

---

[← Back to ESP32 MCU Documentation](../README.md)
