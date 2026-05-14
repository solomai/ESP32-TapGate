#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_cpu.h"
#include "esp_timer.h"

#include "wait_dbg.h"

#ifdef CONFIG_APP_WAIT_FOR_DEBUGGER

static const char *TAG = "DEBUG_INIT";

// Volatile flag set by GDB to signal it's ready to break at dbg_resume_point()
volatile uint32_t dbg_ready = 0;

#endif // CONFIG_APP_WAIT_FOR_DEBUGGER

void __attribute__((noinline, noclone, used)) dbg_resume_point(void)
{
#ifdef CONFIG_APP_WAIT_FOR_DEBUGGER
    /* GDB breakpoint lands here */
    ESP_LOGI(TAG, "Debug breakpoint caught.");
#endif // CONFIG_APP_WAIT_FOR_DEBUGGER
}

void wait_debugger_connection(void)
{
#ifdef CONFIG_APP_WAIT_FOR_DEBUGGER

    if (!esp_cpu_dbgr_is_attached()) {
        ESP_LOGW(TAG, "OpenOCD not detected, skipping debug wait.");
        return;
    }

    const uint32_t timeout_ms = (uint32_t)CONFIG_APP_WAIT_FOR_DEBUGGER_TIMEOUT * 1000U;

    ESP_LOGI(TAG, "OpenOCD detected. Waiting for GDB (timeout %u seconds) ...",
             CONFIG_APP_WAIT_FOR_DEBUGGER_TIMEOUT);

    const int64_t start_us = esp_timer_get_time();

    while (dbg_ready == 0) {
        const uint32_t elapsed_ms = (uint32_t)((esp_timer_get_time() - start_us) / 1000LL);
        if (elapsed_ms >= timeout_ms) {
            ESP_LOGW(TAG, "GDB timeout after %u seconds, continuing execution.", CONFIG_APP_WAIT_FOR_DEBUGGER_TIMEOUT);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // GDB signaled ready, break at dbg_resume_point() for a clean debug start.
    dbg_resume_point();
#endif // CONFIG_APP_WAIT_FOR_DEBUGGER 

} // wait_debugger_connection
