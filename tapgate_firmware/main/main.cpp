#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_app_trace.h"
#include "esp_app_desc.h"
#include "esp_log.h"

#include "constants.h"
#include "event_journal.h"
#include "nvm.h"
#include "datetime.h"

#include "diag/reset_reason.h"

#ifdef CONFIG_APP_WAIT_FOR_DEBUGGER
#include "diag/wait_dbg.h"
#endif

extern "C" void app_main(void)
{
#ifdef CONFIG_APP_WAIT_FOR_DEBUGGER
    wait_debugger_connection();
#endif

    // Global Initialization section
    // Follow sequence of steps to ensure predictable startup behavior and proper error handling.

    // Initialize NVS partitions.
    // Critical for application operation. Will be before any NVM access.
    esp_err_t err = NVM.Init();
    if (err != ESP_OK)
    {
        // Critical error
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR,
                          TAG_MAIN,
                          "Internal NVM access failed: " ERR_FORMAT, esp_err_to_str(err), err);
        esp_system_abort("Verify NVM partition label in 'nvm_partition.h' and 'partitions.csv'");
    }

    // Initialize DateTime subsystem.
    err = DateTime.Init();
    if (err != ESP_OK)
    {
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR,
                          TAG_MAIN,
                          "DateTime initialization failed: " ERR_FORMAT, esp_err_to_str(err), err);
    }
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
                    TAG_MAIN,
                    "DateTime: %s", DateTime.FormatFixed<32>(DT_FMT_HUMAN_FULL).data()); 

    // Report last reset reason
    const esp_reset_reason_t reason = esp_reset_reason();
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
                      TAG_MAIN,
                      "Last reset reason: \"%s\"", get_reset_reason_text(reason));

    // Almost all initialization steps are complete. 
    // Report startup complete before entering main loop.
#ifdef APP_DEBUG_MODE
    // Report debug mode enabled - this is a warning since it may impact security and performance,
    // but it's not an error since it's intentional in some cases (e.g. development builds).
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING,
        TAG_MAIN,
        "Debug Mode enabled");
#endif

    // Initialization complete.
    // Log app info
    const esp_app_desc_t *app_desc = esp_app_get_description();
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
        TAG_MAIN,
        "Firmware Version: %s, Build Time: %s, IDF Version: %s",
        app_desc->version, app_desc->date, app_desc->idf_ver);

    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
        TAG_MAIN,
        "TapGate firmware startup complete");
    
    // Main app loop
    while (true)
    {
        // TODO:
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}