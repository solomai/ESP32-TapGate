// include freertos lib
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

// include components
#include "nvm/nvm.h"
#include "constants.h"
#include "event_journal.h"

#include "led.h"
#include "device_context.h"

#ifdef DIAGNOSTIC_VERSION
    #include "diagnostic.h"
#endif

// log tag
static const char *TAG_MAIN = "MAIN";

// MAIN FUNCTION
void app_main(void)
{
#ifdef DIAGNOSTIC_VERSION
	/* your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	xTaskCreatePinnedToCore(&diagnostic_task, "debug_mon_task", 2560, NULL, 1, NULL, 1); 
#endif

    blue_led_init();

    // Init NVS partitions
    esp_err_t err = nvm_init();
    if (err != ESP_OK)
    {
        // Critical error
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR,
                          TAG_MAIN,
                          "Internal NVM access failed '%s'", esp_err_to_name(err));
        esp_system_abort("Verify NVM partition label in 'nvm_partition.h' and 'partitions.csv'");
    }

    // Initialize device context singleton
    err = device_context_init();
    if (err != ESP_OK)
    {
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_ERROR,
                          TAG_MAIN,
                          "Device context initialization failed '%s'", esp_err_to_name(err));
        esp_system_abort("Failed to initialize device context");
    }

    // Example: Access device context
    device_context_t *ctx = device_context_get();
    if (ctx != NULL) {
        EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
                          TAG_MAIN,
                          "Device context initialized successfully");
        // You can now use ctx->device_uid and other fields
    }

    EVENT_JOURNAL_ADD(EVENT_JOURNAL_INFO,
                      TAG_MAIN,
                      "TapGate firmware startup complete");
    // Main loop
    while (1) {
        // Main application logic goes here

        // For now, just delay to avoid busy loop
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}