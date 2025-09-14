// include freertos lib
#include "esp_log.h"

// include components
#include "clients.h"

// log tag
static const char log_tag[] = "APP_MAIN";

void app_main(void)
{
    ESP_LOGI(log_tag, "Application started");
    // Init section

    // Main loop
    while (1) {
        // Main application logic goes here

        // For now, just delay to avoid busy loop
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}