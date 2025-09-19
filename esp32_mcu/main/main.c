// include freertos lib
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

// include components
#include "nvm/nvm.h"
#include "logs.h"
#include "clients.h"

#include "memory_inventory.h"

// log tag
static const char *TAG_MAIN = "APP_MAIN";

void app_main(void)
{
    LOGN(TAG_MAIN, "Application bootup");

    // Init NVS partitions
    esp_err_t err = nvm_init();
    if (err != ESP_OK)
    {
        // Critical error
        LOGC(TAG_MAIN, "Error accessing internal memory, device cannot start properly");
    }

    // Main loop
    while (1) {
        // Main application logic goes here

        // For now, just delay to avoid busy loop
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}