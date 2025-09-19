#include "memory_inventory.h"
#include <stddef.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"

static const char* log_tag = "DIAG";

static void print_heap_line(const char* name, uint32_t caps)
{
    size_t free_sz    = heap_caps_get_free_size(caps);
    size_t total_sz   = heap_caps_get_total_size(caps);
    ESP_LOGI(log_tag,"%-10s: free=%7u B, total=%7u B",
             name, (unsigned)free_sz, (unsigned)total_sz);
}

void print_memory_inventory(void)
{
    // ---- Chip basics
    esp_chip_info_t ci;
    esp_chip_info(&ci);
    ESP_LOGI(log_tag, "Chip: ESP32, cores=%d, features: %s%s%s",
           ci.cores,
           (ci.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi " : "",
           (ci.features & CHIP_FEATURE_BT)       ? "BT "   : "",
           (ci.features & CHIP_FEATURE_BLE)      ? "BLE "  : "");
    
    // ---- Break down by capability
    ESP_LOGI(log_tag, "Memory capability:");
    print_heap_line("INTERNAL",   MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    print_heap_line("DMA",        MALLOC_CAP_DMA      | MALLOC_CAP_8BIT);
    print_heap_line("EXEC(IRAM)", MALLOC_CAP_EXEC);
    print_heap_line("PSRAM", MALLOC_CAP_SPIRAM);
  
}