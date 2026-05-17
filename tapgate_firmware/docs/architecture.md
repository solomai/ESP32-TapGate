# Software architecture

## Modules

- [Event Journal](#event-journal) — persistent audit log for operationally significant events with severity-based retention and LittleFS-backed storage.

- [DateTime](#datetime) — singleton wrapper for system date/time and timezone management with strftime-based formatting and a planned SNTP synchronization extension point.

---

## Event Journal

Event Journal is a persistent auditing component designed to record operationally significant events — system startup, command execution, operational faults, anomalous client activity, and similar runtime occurrences. It is not a debug logger; it is a structured event record intended for post-mortem analysis and behavioral auditing of the device over its operating lifetime.

Every event is classified by severity (`INFO`, `WARNING`, `ERROR`, `ALERT`) and written simultaneously to persistent storage and to the standard ESP-IDF log output, so entries remain visible in the serial monitor during normal operation.

**Storage.** The journal is built on the existing LittleFS partition (1920 KB) using plain POSIX file I/O over `esp_littlefs` — no additional libraries. All events flow into a pair of rolling segment files (`main_0.log` / `main_1.log`), preserving global event order without requiring timestamps or sequence counters. Rotation is implemented as delete-then-create rather than truncate, because LittleFS is a copy-on-write filesystem and truncation triggers a full internal file copy.

**Severity-specific retention.** `ERROR` and `ALERT` events are additionally mirrored into dedicated files (`error_0.log` / `error_1.log` and `alert_0.log` / `alert_1.log`) with larger, independent quotas. This decouples their retention lifetime from the main segment rotation and ensures high-severity records survive regardless of general event volume. Because these levels are infrequent by nature, the storage overhead of duplication is negligible.

**Quota isolation.** Each file carries a fixed byte quota. Files do not compete for space, so a burst of `INFO`/`WARNING` activity cannot displace `ERROR`/`ALERT` records and vice versa.

**Durability.** After every `ERROR` or `ALERT` write, `fflush()` is called explicitly to guarantee the record is committed to flash before a potential power loss.

---

## DateTime

`DateTimeWrapper` is a singleton that centralizes all date, time, and timezone operations for the device. It wraps the ESP-IDF POSIX time API (`gettimeofday` / `settimeofday` / `strftime`) and exposes a uniform interface for setting, querying, and formatting time in both UTC and local representations.

**Timezone.** The active timezone is stored as a POSIX TZ string (e.g. `"EET-2EEST,M3.5.0,M10.5.0/3"`) in a fixed internal buffer and applied immediately via `setenv("TZ")` + `tzset()`. UTC is the default when no timezone has been configured.

**Formatting.** Time-to-string conversion is based on `strftime`. A set of predefined `DT_FMT_*` macros covers the most common output patterns (ISO-8601 with offset, log timestamp, filename-safe sortable timestamp, human-readable variants). Both current-time and arbitrary `struct tm` overloads are provided; `FormatFixed<N>()` returns a stack-allocated array for zero-heap call sites.

**Synchronization state.** The `IsTimeSynchronized()` flag is set on the first successful `SetTime()` call and serves as an extension point for the planned SNTP integration. Online clock synchronization — time and timezone retrieval from a network time source — is not yet implemented but is architecturally reserved in `Init()` and the sync-state flag.

---

[← Back to ESP32 MCU Documentation](../readme.md)  
[← Back to main README](../../README.md)
