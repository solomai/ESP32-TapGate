#pragma once

// call.h — Convenience macros for esp_err_t-returning calls with automatic
// warning or error logging on failure.
//
// Each macro evaluates the given expression exactly once, assigns the result
// to an internal variable, and logs on failure using the stringified call
// expression as the operation name.  The macro itself evaluates to the
// esp_err_t result so it can be assigned or passed directly.
//
// Variants:
//   CALLW(tag, expr)   — ESP_LOGW on failure
//   CALLE(tag, expr)   — ESP_LOGE on failure
//
// Requirements:
//   device_err.h (ERR_FORMAT, esp_err_to_str) must be reachable.
//
// Examples:
//
//   char buf[DEVICE_NAME_CAPACITY]{};
//   err = CALLW(TAG_MAIN, DeviceCtx.get_device_name({buf, sizeof(buf)}));
//
//   err = CALLE(TAG_MAIN, nvs_flash_init());
//   if (err != ESP_OK) return err;
//
// Log output format (on failure):
//   W [TAG_MAIN] Failed DeviceCtx.get_device_name({buf, sizeof(buf)}): "ESP_ERR_INVALID_STATE" (0x103)

#include "esp_log.h"
#include "device_err.h"  // ERR_FORMAT, esp_err_to_str

#define CALLW(tag_, expr_)                                                            \
    [&]() -> esp_err_t {                                                              \
        const esp_err_t _err = (expr_);                                               \
        if (_err != ESP_OK) {                                                         \
            ESP_LOGW((tag_), "Failed " #expr_ ": " ERR_FORMAT,                       \
                     esp_err_to_str(_err), _err);                                     \
        }                                                                             \
        return _err;                                                                  \
    }()

#define CALLE(tag_, expr_)                                                            \
    [&]() -> esp_err_t {                                                              \
        const esp_err_t _err = (expr_);                                               \
        if (_err != ESP_OK) {                                                         \
            ESP_LOGE((tag_), "Failed " #expr_ ": " ERR_FORMAT,                       \
                     esp_err_to_str(_err), _err);                                     \
        }                                                                             \
        return _err;                                                                  \
    }()
