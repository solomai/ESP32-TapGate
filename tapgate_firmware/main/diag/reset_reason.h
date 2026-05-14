#pragma once

#include "esp_system.h"

#include "common/constants.h"

/**
 * @brief Converts esp_reset_reason_t to a human-readable string.
 * 
 * @param reason The reset reason code.
 * @return const char* String representation of the reset reason.
 */
static inline const char* get_reset_reason_text(esp_reset_reason_t reason)
{
    switch (reason) {
        case ESP_RST_UNKNOWN:   return "Unknown";
        case ESP_RST_POWERON:   return "Power-On";
        case ESP_RST_EXT:       return "External Pin";
        case ESP_RST_SW:        return "Software";
        case ESP_RST_PANIC:     return "Exception/Panic";
        case ESP_RST_INT_WDT:   return "Interrupt Watchdog";
        case ESP_RST_TASK_WDT:  return "Task Watchdog";
        case ESP_RST_WDT:       return "Other Watchdog";
        case ESP_RST_DEEPSLEEP: return "Deep Sleep Exit";
        case ESP_RST_BROWNOUT:  return "Brownout (Voltage dip)";
        case ESP_RST_SDIO:      return "SDIO Reset";
        default:                return "Undefined";
    }
}

static void LogLastResetReason(void)
{
    const esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI(TAG_MAIN, "Last reset reason: \"%s\"", get_reset_reason_text(reason));
}
