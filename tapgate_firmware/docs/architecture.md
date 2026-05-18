# Software architecture

## Modules

- [Event Journal](#event-journal) — persistent audit log of operationally significant events.

- [DateTime](#datetime) — system date, time, and timezone management.

- [Device Context](#device-context) — authoritative device state: persisted configuration and ephemeral runtime data.

---

## Event Journal

Event Journal is a persistent auditing component designed to record operationally significant events — system startup, command execution, operational faults, anomalous client activity, and similar runtime occurrences. It is not a debug logger; it is a structured event record intended for post-mortem analysis and behavioral auditing of the device over its operating lifetime.

Every event is classified by severity (`INFO`, `WARNING`, `ERROR`, `ALERT`) and written simultaneously to persistent storage and to the standard ESP-IDF log output, so entries remain visible in the serial monitor during normal operation.

---

## DateTime

`DateTimeWrapper` is a singleton that centralizes all date, time, and timezone operations for the device. It wraps the ESP-IDF POSIX time API (`gettimeofday` / `settimeofday` / `strftime`) and exposes a uniform interface for setting, querying, and formatting time in both UTC and local representations.

---

## Device Context

`DeviceCtx` is a singleton that holds the authoritative state of the device — persisted configuration (loaded from NVS on boot, written back on every change) and ephemeral runtime state (initialized to defaults on boot, never persisted). All components read and mutate device state exclusively through this class.

---

[← Back to ESP32 MCU Documentation](../readme.md)  
[← Back to main README](../../README.md)
