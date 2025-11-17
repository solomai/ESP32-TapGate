#pragma once

#include "esp_err.h"
#include "types.h"
#include <stdbool.h>

/**
 * Device context structure - holds global device state
 * Singleton pattern - use device_context_get() to access
 */
typedef struct
{
    uid_t device_uid;                    // Unique device identifier
    // Add more device context fields here as needed:
    // - Current connector channel
    // - Device state
    // - Configuration parameters
    // etc.
} device_context_t;

/**
 * Initialize the device context singleton
 * Must be called once in app_main() before using device_context_get()
 * Thread-safe: Safe to call multiple times (subsequent calls ignored)
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NO_MEM if mutex creation fails
 *         Other error codes from NVS operations
 */
esp_err_t device_context_init(void);

/**
 * Get pointer to the singleton device context instance
 * Thread-safe: Multiple threads can safely read from the context
 * 
 * @return Pointer to device context, or NULL if not initialized
 * @note Always check for NULL return value
 * @note For modifications, use device_context_update()
 */
device_context_t* device_context_get(void);

/**
 * Update device context with new values
 * Thread-safe: Uses mutex to protect concurrent access
 * 
 * @param new_context Pointer to new context values to copy
 * @param save_to_nvs If true, persist changes to NVS storage
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_STATE if context not initialized
 *         ESP_ERR_INVALID_ARG if new_context is NULL
 *         Other error codes from NVS operations
 */
esp_err_t device_context_update(const device_context_t *new_context, bool save_to_nvs);

/**
 * Cleanup and deinitialize device context
 * Not typically needed in production firmware (device never "stops")
 * Useful for unit tests or special shutdown scenarios
 */
void device_context_deinit(void);

// Internal functions - not for public use
esp_err_t device_context_load_from_nvs(device_context_t *context);
esp_err_t device_context_store_to_nvs(const device_context_t *context);