// include freertos lib
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// include components
#include "logs.h"
#include "clients.h"

#include "diagnostic.h"

// log tag
static const char log_tag[] = "APP_MAIN";

void app_main(void)
{
#ifdef DIAGNOSTIC_VERSION
    print_memory_inventory();
#endif
    LOGI(log_tag, "Application started");
    // Init section

    // Main loop
    while (1) {
        // Main application logic goes here

        // For now, just delay to avoid busy loop
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}