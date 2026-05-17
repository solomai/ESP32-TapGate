# ESP32 MCU Documentation

## Overview

---

## Contents
- [Hardware](#hardware)
- [Architecture](#architecture)
- [Setup development environment](#setup-development-environment)

---

## Hardware

Development and testing are performed on the **ESP32-WROOM-32** development board — a module built around the Xtensa® LX6 dual-core 32-bit MCU with integrated 802.11 b/g/n Wi-Fi and Bluetooth 4.2 (Classic + BLE). For full electrical and RF specifications, refer to the [ESP32-WROOM-32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf) and [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf).

The firmware targets the ESP32 SoC family and is compatible with any board that exposes the standard ESP-IDF hardware abstraction layer.

---

## Architecture  
High-level view of the software design, components, and how modules interact.  
- [Software Architecture](docs/architecture.md) — details on tasks, memory usage, and system flow.
- [Partitioning and Partition Sizing Rationale](docs/partitions.md) — describes the flash partition layout, purpose, and sizing choices for persistent nonce counters.


## Setup development environment
- [Development Environment](docs/dev-env.md) — toolchain installation, IDE setup
- [Build, Flash and Tests](docs/dev-proc.md) — build commands, host/device tests, connection methods
- [ESP-PROG Setup](docs/esp-prog.md) — JTAG wiring, OpenOCD, hardware debugging
- [Embedded Guidelines](docs/embedded-guidelines.md) — C++ constraints and coding rules

---