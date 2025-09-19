#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char TAG[] = "DIAG";

void diagnostic_task(void *pvParameter)
{
	for(;;){
		ESP_LOGI(TAG, "free heap: %d bytes",esp_get_free_heap_size());
		vTaskDelay( pdMS_TO_TICKS(10000) );
	}
}