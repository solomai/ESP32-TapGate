# ESP32 MCU Documentation

## Overview  
General description of the ESP32 firmware, its purpose, and supported features.  
- [Target Device](../docs/device/target.md) — explains the hardware platform, main specifications, and supported peripherals.  

## Architecture  
High-level view of the software design, components, and how modules interact.  
- [Software Architecture for Device](../docs/device/architecture.md) — details on tasks, memory usage, and system flow.
- [Partitioning and Partition Sizing Rationale](../docs/device/partitions.md) — describes the flash partition layout, purpose, and sizing choices for persistent nonce counters.

Modules:
- [Clients](../docs/device/module-clients.md) — Clients module details.
- [X25519](../docs/device/module-ecies.md) —  module for elliptic-curve key agreement and secure message encryption (ECIES).

## Setup & Configuration
Instructions for preparing the environment, building, flashing, and monitoring.
- [How-To Build Guide](../docs/device/howto.md) — step-by-step guide on how to set up and compile the project.

### Host unit tests
- Configure the native test project with `cmake --preset host-tests` so IDEs can pick up the generated `CTest` metadata. The preset enables `BUILD_TESTING` and never requires the ESP-IDF toolchain.
- Build and execute the suite locally via `cmake --build --preset host-tests` followed by `ctest --preset host-tests` (the preset already passes the correct `-C Debug` flag for multi-config generators).
- Each Unity binary understands `--list` to enumerate available test names and `--filter <substring>` to run a subset, which helps when dispatching single cases from IDE menus.
- When CMake is invoked without ESP-IDF's `ESP_PLATFORM` flag (for example when selecting a Visual Studio kit), the top-level project automatically enters the host-test configuration so IDE test menus can discover the executables.

## Protocol
Description of the communication protocol between ESP32 and external clients.
- [Communication Protocol](../docs/general/protocol.md) — message format, sequences, and error handling.

---

[← Back to main README](../README.md)
