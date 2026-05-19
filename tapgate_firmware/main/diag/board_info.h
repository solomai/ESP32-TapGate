#pragma once

#include "esp_chip_info.h"

// Returns the chip model name as a human-readable string.
//
// Usage example:
//    esp_chip_info_t info;
//    esp_chip_info(&info);
//    ESP_LOGI(TAG_MAIN, "Chip: %s", get_chip_model_name(info.model));

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

static inline const char* get_current_chip_name()
{
    esp_chip_info_t info{};
    esp_chip_info(&info);
    return get_chip_model_name(info.model);
}
