# ESP32 MCU Documentation

## Overview  
General description of the ESP32 firmware, its purpose, and supported features.  
- [Target Device](../docs/device/target.md) — explains the hardware platform, main specifications, and supported peripherals.  

## Architecture  
High-level view of the software design, components, and how modules interact.  
- [Software Architecture for Device](../docs/device/architecture.md) — details on tasks, memory usage, and system flow.

Modules:
- [Clients](../docs/device/module-clients.md) — Clients module details.

## Setup & Configuration
Instructions for preparing the environment, building, flashing, and monitoring.
- [How-To Build Guide](../docs/device/howto.md) — step-by-step guide on how to set up and compile the project.

### Host unit tests
- Configure the native test project with `cmake --preset host-tests` to work with IDE test runners. The preset builds the sources under `tests_host` without requiring the ESP-IDF toolchain.
- Build and execute the suite locally via `cmake --build --preset host-tests` followed by `ctest --preset host-tests`.

## Protocol
Description of the communication protocol between ESP32 and external clients.
- [Communication Protocol](../docs/general/protocol.md) — message format, sequences, and error handling.

---

[← Back to main README](../README.md)
