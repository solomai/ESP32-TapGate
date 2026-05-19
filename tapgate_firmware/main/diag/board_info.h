#pragma once

#include <array>
#include <cstdio>
#include <cstring>
#include "esp_chip_info.h"
#include "esp_efuse.h"
#include "esp_mac.h"
#include "esp_log.h"

// ── Chip model ───────────────────────────────────────────────────────────────

static inline const char* get_chip_model_name(esp_chip_model_t model)
{
    switch (model) {
        case CHIP_ESP32:        return "ESP32";
        case CHIP_ESP32S2:      return "ESP32-S2";
        case CHIP_ESP32S3:      return "ESP32-S3";
        case CHIP_ESP32C3:      return "ESP32-C3";
        case CHIP_ESP32C2:      return "ESP32-C2";
        case CHIP_ESP32C6:      return "ESP32-C6";
        case CHIP_ESP32H2:      return "ESP32-H2";
        case CHIP_ESP32P4:      return "ESP32-P4";
        case CHIP_ESP32C61:     return "ESP32-C61";
        case CHIP_ESP32C5:      return "ESP32-C5";
        case CHIP_ESP32H21:     return "ESP32-H21";
        case CHIP_ESP32H4:      return "ESP32-H4";
        case CHIP_POSIX_LINUX:  return "POSIX/Linux";
        default:                return "Unknown";
    }
}

// ── Package variant ──────────────────────────────────────────────────────────
// ESP32 only: maps pkg_ver eFuse + major revision → variant string matching
// esptool output (e.g. "D0WD-V3", "PICO-D4").
// All other chips return "" so the model name is used as-is.

#if CONFIG_IDF_TARGET_ESP32
static inline const char* get_esp32_pkg_name(uint32_t pkg, uint8_t major_rev)
{
    switch (pkg) {
        case 0: return (major_rev >= 3) ? "D0WD-V3" : "D0WD";
        case 1: return "D0WD-R0";
        case 2: return "D2WD";
        case 4: return "PICO-D4";
        case 5: return "PICO-V3";
        case 6: return "PICO-V3-02";
        default: return "";
    }
}
#endif

// Returns the package variant string for the current chip, e.g. "D0WD-V3".
// Other chips return "" (caller must handle empty string in format strings).
static inline const char* get_chip_pkg_str()
{
    esp_chip_info_t info{};
    esp_chip_info(&info);
    const uint8_t major = static_cast<uint8_t>(info.revision / 100);
#if CONFIG_IDF_TARGET_ESP32
    return get_esp32_pkg_name(esp_efuse_get_pkg_ver(), major);
#else
    (void)major;
    return "";
#endif
}

// ── eFuse coding scheme ──────────────────────────────────────────────────────

static inline const char* get_coding_scheme_name(esp_efuse_coding_scheme_t scheme)
{
    switch (scheme) {
        case EFUSE_CODING_SCHEME_NONE:   return "None";
        case EFUSE_CODING_SCHEME_3_4:    return "3/4";
        case EFUSE_CODING_SCHEME_REPEAT: return "Repeat";
#ifdef EFUSE_CODING_SCHEME_RS
        case EFUSE_CODING_SCHEME_RS:     return "RS(4,8)";
#endif
        default:                         return "Unknown";
    }
}

// ── Features string ──────────────────────────────────────────────────────────
// Builds a comma-separated feature list into caller-supplied buffer,
// matching esptool style: "Wi-Fi, BT, Dual Core + ULP Core, 240MHz, Coding Scheme None"

static inline void build_chip_features_str(char* out, size_t out_size,
                                            const esp_chip_info_t& info)
{
    out[0] = '\0';
    size_t pos = 0;

    auto append = [&](const char* s) {
        if (pos > 0 && pos + 2 < out_size) {
            out[pos++] = ',';
            out[pos++] = ' ';
            out[pos]   = '\0';
        }
        size_t n = strnlen(s, out_size - pos - 1);
        memcpy(out + pos, s, n);
        pos += n;
        out[pos] = '\0';
    };

    if (info.features & CHIP_FEATURE_WIFI_BGN)    append("Wi-Fi");
    if (info.features & CHIP_FEATURE_BT)          append("BT");
    if (info.features & CHIP_FEATURE_BLE)         append("BLE");
    if (info.features & CHIP_FEATURE_IEEE802154)  append("802.15.4");
    if (info.features & CHIP_FEATURE_EMB_FLASH)   append("Embedded Flash");
#ifdef CHIP_FEATURE_EMB_PSRAM
    if (info.features & CHIP_FEATURE_EMB_PSRAM)   append("Embedded PSRAM");
#endif

    // Core count label: "Single Core" / "Dual Core"
    // ESP32 / S2 / S3 also expose a ULP (Ultra Low Power) co-processor.
    const char* core_label = (info.cores > 1) ? "Dual Core" : "Single Core";
    char cores_str[48];
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
    snprintf(cores_str, sizeof(cores_str), "%s + ULP Core", core_label);
#else
    snprintf(cores_str, sizeof(cores_str), "%s", core_label);
#endif
    append(cores_str);

    char freq_str[16];
    snprintf(freq_str, sizeof(freq_str), "%uMHz", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    append(freq_str);

    // Coding scheme appended last, matching esptool: "Coding Scheme None"
    char scheme_str[32];
    const esp_efuse_coding_scheme_t scheme = esp_efuse_get_coding_scheme(EFUSE_BLK1);
    snprintf(scheme_str, sizeof(scheme_str), "Coding Scheme %s", get_coding_scheme_name(scheme));
    append(scheme_str);
}

// ── Full chip-type string ────────────────────────────────────────────────────
// Writes "ESP32-D0WD-V3" (ESP32 with package) or "ESP32-S3" (other chips)
// into caller-supplied buffer.

static inline void build_chip_type_str(char* out, size_t out_size)
{
    esp_chip_info_t info{};
    esp_chip_info(&info);

    const uint8_t major_rev = static_cast<uint8_t>(info.revision / 100);
    const char*   model     = get_chip_model_name(info.model);

#if CONFIG_IDF_TARGET_ESP32
    const uint32_t pkg = esp_efuse_get_pkg_ver();
    const char* pkg_str = get_esp32_pkg_name(pkg, major_rev);
    if (pkg_str[0] != '\0') {
        snprintf(out, out_size, "%s-%s", model, pkg_str);
        return;
    }
#endif

    snprintf(out, out_size, "%s", model);
}

// ── log_board_info ───────────────────────────────────────────────────────────
// Logs chip information equivalent to esptool's chip detection output.
//
// Example output:
//   Chip type:    ESP32-D0WD-V3 (rev v3.1)
//   Features:     Wi-Fi, BT, 2 Cores, 240MHz
//   Crystal:      40MHz
//   MAC:          a0:a3:b3:28:09:dc
//   Coding Scheme: None

static inline void log_board_info(const char* tag)
{
    esp_chip_info_t info{};
    esp_chip_info(&info);

    const uint8_t major_rev = static_cast<uint8_t>(info.revision / 100);
    const uint8_t minor_rev = static_cast<uint8_t>(info.revision % 100);

    char chip_type[32];
    build_chip_type_str(chip_type, sizeof(chip_type));

    char features[96];
    build_chip_features_str(features, sizeof(features), info);

    std::array<uint8_t, 6> mac{};
    esp_read_mac(mac.data(), ESP_MAC_WIFI_STA);

    const esp_efuse_coding_scheme_t scheme = esp_efuse_get_coding_scheme(EFUSE_BLK1);

    ESP_LOGI(tag,
        "Chip type:     %s (rev v%u.%u)\n"
        "  Features:    %s\n"
        "  Crystal:     %dMHz\n"
        "  MAC:         %02x:%02x:%02x:%02x:%02x:%02x\n"
        "  Coding:      %s",
        chip_type, major_rev, minor_rev,
        features,
        CONFIG_XTAL_FREQ,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        get_coding_scheme_name(scheme));
}

// ── Compatibility shim ───────────────────────────────────────────────────────
// Retained for call sites that only need the model name string.

static inline const char* get_current_chip_name()
{
    esp_chip_info_t info{};
    esp_chip_info(&info);
    return get_chip_model_name(info.model);
}
