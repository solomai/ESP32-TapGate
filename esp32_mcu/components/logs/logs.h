#pragma once
#include "constants.h"
#include "esp_log.h"

void disable_esp_log(const char *tag);

void set_esp_log_level(const char *tag, esp_log_level_t level);

// ---------------------------------------------------------
// Debug log. lowest priority.
// does not go into the device event log.
// called ESP_LOGD
void LOGD(const char *tag, const char *fmt, ...);

// Info level is also used for debugging purposes.
// does not go into the device event log.
// called ESP_LOGI.
void LOGI(const char *tag, const char *fmt, ...);

// ---------------------------------------------------------
// Notice level.
// goes into the device event log.
// called ESP_LOGI.
void LOGN(const char *tag, const char *fmt, ...);

// Warning
// goes into the device event log.
// called ESP_LOGW.
void LOGW(const char *tag, const char *fmt, ...);

// Error
// goes into the device event log.
// called ESP_LOGE.
void LOGE(const char *tag, const char *fmt, ...);

// Critical Error
// goes into the device event log.
// called ESP_LOGE.
void LOGC(const char *tag, const char *fmt, ...);

// ---------------------------------------------------------

#define ERROR_CHECK(x, msg)                                                  \
    do {                                                                     \
        esp_err_t __err_rc = (x);                                            \
        if (__err_rc != ESP_OK) {                                            \
            LOGC(TAG, "%s : %s", (msg), esp_err_to_name(__err_rc));          \
        }                                                                    \
    } while (0)

