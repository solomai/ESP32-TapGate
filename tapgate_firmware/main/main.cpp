#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_app_trace.h"
#include "esp_log.h"

#include "common/constants.h"
#include "diag/reset_reason.h"
#include "event_journal/event_journal.h"

#ifdef CONFIG_APP_WAIT_FOR_DEBUGGER
#include "diag/wait_dbg.h"
#endif

extern "C" void app_main(void)
{
#ifdef CONFIG_APP_WAIT_FOR_DEBUGGER
    wait_debugger_connection();
#endif

#ifdef APP_DEBUG_MODE
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING,
        TAG_MAIN,
        "Debug Mode enabled");
#endif

    LogLastResetReason();

    // Initialization complete. Main application loop
    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
        TAG_MAIN,
        "TapGate firmware startup complete");
    
    while (true) {

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}