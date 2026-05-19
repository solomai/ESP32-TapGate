#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_app_trace.h"
#include "esp_app_desc.h"
#include "esp_log.h"

#include "constants.h"
#include "event_journal.h"
#include "fcall.h"
#include "nvm.h"
#include "datetime.h"
#include "device_ctx.h"
#include "uuid.h"

#include "diag/reset_reason.h"
#include "diag/board_info.h"

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

    // Initialize DeviceContext — loads persisted configuration from NVS.
    err = DeviceCtx.Init();
    if (err != ESP_OK)
    {
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR,
                          TAG_MAIN,
                          "DeviceCtx initialization failed: " ERR_FORMAT, esp_err_to_str(err), err);
    }

    // Almost all initialization steps are complete. 
    // Report startup complete before entering main loop.
    {
        // Initialization complete. Log app info

        char device_name_buf[NAME_MAX_SIZE]{};
        CALLW(TAG_MAIN, DeviceCtx.get_device_name({device_name_buf, sizeof(device_name_buf)}));

        char device_id_buf[UID_STR_LEN]{};
        {
            tg_uid_t device_id{};
            CALLW(TAG_MAIN, DeviceCtx.get_device_id(device_id));
            uid_to_str(device_id, {device_id_buf, sizeof(device_id_buf)});
        }

        const esp_reset_reason_t reason = esp_reset_reason();
        const esp_app_desc_t *app_desc = esp_app_get_description();

        EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
            TAG_MAIN, "Boot Info [%s]:"
            "\nDevice name: \"%s\""
            "\nDevice Id:   \"%s\""
            "\nFirmware Ver: \"%s\""
            "\nIDF Version: %s"
            "\nLast reset reason: \"%s\""
            "\nDateTime: %s",
            get_current_chip_name(),
            device_name_buf,
            device_id_buf,
            app_desc->version,
            app_desc->idf_ver,
            get_reset_reason_text(reason),
            DateTime.FormatFixed<32>(DT_FMT_HUMAN_FULL).data());

#ifdef CONFIG_APP_DEBUG_MODE
        // Report debug mode enabled - this is a warning since it may impact security and performance,
        // but it's not an error since it's intentional in some cases (e.g. development builds).
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_WARNING,
            TAG_MAIN,
            "Debug Mode enabled");
#endif           
    }
    
    // Main app loop
    ESP_LOGI(TAG_MAIN, "Entering main loop");
    while (true)
    {
        // TODO:
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}