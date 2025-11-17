#include "device_context.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "DEVICE_CTX";

// Singleton instance - static, not exported
static device_context_t g_device_context = {0};
static bool g_is_initialized = false;
static SemaphoreHandle_t g_context_mutex = NULL;

/**
 * Initialize device context singleton
 * Must be called once before using device_context_get()
 * Thread-safe: Can be called multiple times, but only first call does initialization
 */
esp_err_t device_context_init(void)
{
    if (g_is_initialized) {
        ESP_LOGW(TAG, "Device context already initialized");
        return ESP_OK;
    }

    // Create mutex for thread safety
    if (g_context_mutex == NULL) {
        g_context_mutex = xSemaphoreCreateMutex();
        if (g_context_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    // Take mutex
    if (xSemaphoreTake(g_context_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }

    // Double-check pattern
    if (!g_is_initialized) {
        // Initialize context with defaults
        memset(&g_device_context, 0, sizeof(device_context_t));
        
        // Try to load from NVS
        esp_err_t err = device_context_load_from_nvs(&g_device_context);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Device context loaded from NVS");
        } else if (err == ESP_ERR_NOT_FOUND) {
            ESP_LOGI(TAG, "No saved context found, using defaults");
            // Set default values here if needed
        } else {
            ESP_LOGE(TAG, "Failed to load context: %s", esp_err_to_name(err));
            xSemaphoreGive(g_context_mutex);
            return err;
        }
        
        g_is_initialized = true;
        ESP_LOGI(TAG, "Device context initialized");
    }

    xSemaphoreGive(g_context_mutex);
    return ESP_OK;
}

/**
 * Get pointer to the singleton device context
 * Returns NULL if not initialized
 * Thread-safe: Multiple threads can safely call this
 */
device_context_t* device_context_get(void)
{
    if (!g_is_initialized) {
        ESP_LOGE(TAG, "Device context not initialized! Call device_context_init() first");
        return NULL;
    }
    return &g_device_context;
}

/**
 * Update device context and optionally save to NVS
 * Thread-safe
 */
esp_err_t device_context_update(const device_context_t *new_context, bool save_to_nvs)
{
    if (!g_is_initialized) {
        ESP_LOGE(TAG, "Device context not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (new_context == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(g_context_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take mutex");
        return ESP_FAIL;
    }

    memcpy(&g_device_context, new_context, sizeof(device_context_t));

    esp_err_t err = ESP_OK;
    if (save_to_nvs) {
        err = device_context_store_to_nvs(&g_device_context);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save context to NVS: %s", esp_err_to_name(err));
        }
    }

    xSemaphoreGive(g_context_mutex);
    return err;
}

/**
 * Load device context from NVS
 * Internal helper function
 */
esp_err_t device_context_load_from_nvs(device_context_t *context)
{
    if (context == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // TODO: Implement NVS loading
    // For now, return NOT_FOUND to indicate no saved data
    ESP_LOGW(TAG, "NVS load not implemented yet");
    return ESP_ERR_NOT_FOUND;
}

/**
 * Store device context to NVS
 * Internal helper function
 */
esp_err_t device_context_store_to_nvs(const device_context_t *context)
{
    if (context == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // TODO: Implement NVS storing
    ESP_LOGW(TAG, "NVS store not implemented yet");
    return ESP_OK;
}

/**
 * Cleanup and deinitialize (for testing or before restart)
 * Not typically needed in production firmware
 */
void device_context_deinit(void)
{
    if (g_context_mutex != NULL) {
        xSemaphoreTake(g_context_mutex, portMAX_DELAY);
        
        g_is_initialized = false;
        memset(&g_device_context, 0, sizeof(device_context_t));
        
        xSemaphoreGive(g_context_mutex);
        vSemaphoreDelete(g_context_mutex);
        g_context_mutex = NULL;
    }
    
    ESP_LOGI(TAG, "Device context deinitialized");
}
