
### `nvs_nonce` Partition — Sizing Rationale
This section justifies the size of the dedicated nvs_nonce partition used to persist frequently-updated counters (one nonce per client). NVS is log-structured: each flash sector (page) is 4096 bytes and holds 126 entries of 32 bytes (the rest is page header and entry state bitmap). Each update of an integer key (e.g., u32 nonce) consumes one 32-byte entry. When a page fills, NVS migrates live entries to a fresh page and erases the old one, distributing wear across all pages. Typical NOR SPI flash endurance is ~100,000 erase cycles per sector (50k–100k common).

**Workload modeled:** 50 clients with one nonce each (M = 50). Update scenarios: 500/day, 50/day, and 10/day total. With M = 50, the useful entries per page between garbage collections is 126 − M = 76. Thus, every ~76 updates trigger one erase distributed across the partition pages.

**Recommendation:** A **24 KiB (P=6 pages)** nvs_nonce partition is sufficient for all considered workloads, offering centuries of endurance even in the heaviest update scenario. Smaller partitions (e.g., 16 KiB) are still viable, but 24 KiB provides a balanced choice with headroom for more keys or higher update rates.

**Lifetime comparison (for a 16 KiB partition with 4 pages):**

| Updates/day (total) | Erases/day/partition | Erases/day/page | Expected lifetime @100k cycles (years) | Expected lifetime @50k cycles (years) |
|---------------------|----------------------:|----------------:|---------------------------------------:|--------------------------------------:|
| 500/day             | ~6.58                 | ~1.645          | ≈166.6                                 | ≈83.3                                 |
| 50/day              | ~0.658                | ~0.1645         | ≈1,665.8                               | ≈832.9                                |
| 10/day              | ~0.132                | ~0.0329         | ≈8,328.8                               | ≈4,164.4                              |

**Lifetime comparison (for a 24 KiB partition with 6 pages):**

| Updates/day (total) | Erases/day/partition | Erases/day/page | Expected lifetime @100k cycles (years) | Expected lifetime @50k cycles (years) |
|---------------------|----------------------:|----------------:|---------------------------------------:|--------------------------------------:|
| 500/day             | ~6.58                 | ~1.10           | ≈250                                   | ≈125                                  |
| 50/day              | ~0.658                | ~0.110          | ≈2,500                                 | ≈1,250                                |
| 10/day              | ~0.132                | ~0.022          | ≈12,500                                | ≈6,250                                |

Notes: Values assume uniform wear leveling across all pages. For a pessimistic 50k endurance, lifetimes are halved but still exceed device lifetimes by orders of magnitude. If the number of live keys grows beyond 50, fewer updates fit per page before GC, reducing lifetime proportionally.

**References:** [ESP-IDF NVS documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html) — page/entry structure, wear leveling. [ESP-IDF Flash wear considerations](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/flash_psram.html) — NOR flash endurance (~100k cycles/sector).


---

[← Back to ESP32 MCU Documentation](../../esp32_mcu/README.md)  
[← Back to main README](../../README.md)
