// include freertos lib
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

// include components
#include "nvm/nvm.h"
#include "logs.h"
#include "constants.h"

#include "led.h"

// include protobuf messages
#include "messages.pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

#ifdef DIAGNOSTIC_VERSION
    #include "diagnostic.h"
#endif

// log tag
static const char *TAG_MAIN = "APP MAIN";

void cb_ap_start(void *pvParameter)
{
    esp_err_t err = (esp_err_t)pvParameter;
    if (err==ESP_OK){
        LOGN(TAG_MAIN, "WiFi Access Point started");
        turn_on_blue_led();
    }
}

void cb_ap_stop(void *pvParameter)
{
    LOGN(TAG_MAIN, "WiFi Access Point stopped");
    turn_off_blue_led();
}

// MAIN FUNCTION
void app_main(void)
{
#ifdef DIAGNOSTIC_VERSION
	/* your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	xTaskCreatePinnedToCore(&diagnostic_task, "debug_mon_task", 2560, NULL, 1, NULL, 1); 
#endif

    LOGN(TAG_MAIN, "Application bootup");
    blue_led_init();

    // Init NVS partitions
    esp_err_t err = nvm_init();
    if (err != ESP_OK)
    {
        // Critical error
        LOGC(TAG_MAIN, "Error accessing internal memory, device cannot start properly");
        #ifdef DIAGNOSTIC_VERSION
            esp_system_abort("Check NVM Partition naming 'nvm_partition.h' and 'partitions.csv'");
        #endif
    }

    // Main loop
    while (1) {
        // Main application logic goes here

        // For now, just delay to avoid busy loop
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}