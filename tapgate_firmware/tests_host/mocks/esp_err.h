#pragma once

// Minimal esp_err definitions for host unit tests
using esp_err_t = int;
#define ESP_OK                  0
#define ESP_FAIL                1
#define ESP_ERR_INVALID_ARG     0x102
#define ESP_ERR_INVALID_SIZE    0x104
#define ESP_ERR_NVS_NOT_FOUND   0x1102

inline const char* esp_err_to_name(esp_err_t code) {
    switch (code) {
        case ESP_OK:               return "ESP_OK";
        case ESP_FAIL:             return "ESP_FAIL";
        case ESP_ERR_INVALID_ARG:  return "ESP_ERR_INVALID_ARG";
        case ESP_ERR_INVALID_SIZE:  return "ESP_ERR_INVALID_SIZE";
        case ESP_ERR_NVS_NOT_FOUND: return "ESP_ERR_NVS_NOT_FOUND";
        default:                    return "UNKNOWN";
    }
}
