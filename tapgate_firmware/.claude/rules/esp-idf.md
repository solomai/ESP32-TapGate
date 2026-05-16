---
description: ESP-IDF v6 API conventions and FreeRTOS patterns
---

# ESP-IDF v6 Rules

## Error Handling
- All ESP-IDF C APIs return `esp_err_t`; always check the return value
- Use `ESP_ERROR_CHECK(expr)` for initialization that must succeed (aborts on error)
- Use `ESP_RETURN_ON_ERROR(expr, TAG, fmt, ...)` in functions that can fail gracefully
- Use `ESP_GOTO_ON_ERROR(expr, label, TAG, fmt, ...)` when cleanup is needed
- Never silently discard `esp_err_t` — annotate intentional ignores with `(void)`

```cpp
esp_err_t my_init() {
    ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "NVS init failed");
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "WiFi init failed");
    return ESP_OK;
}
```

## Logging
- Always define `static const char* TAG = "module_name";` at file scope
- Use the correct severity level:
  - `ESP_LOGE` — errors that require attention
  - `ESP_LOGW` — warnings
  - `ESP_LOGI` — important informational events (boot, connections)
  - `ESP_LOGD` — debug (compiled out in release by default)
  - `ESP_LOGV` — verbose (compiled out unless explicitly enabled)
- Set per-component log levels in `sdkconfig.defaults`:
  ```
  CONFIG_LOG_DEFAULT_LEVEL_INFO=y
  ```

## FreeRTOS Tasks
```cpp
void task_worker(void* arg) {
    auto* ctx = static_cast<WorkerContext*>(arg);
    for (;;) {
        // do work
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Create task
xTaskCreate(task_worker, "worker", 4096, &ctx, tskIDLE_PRIORITY + 1, nullptr);
```
- Always call `vTaskDelay` in infinite task loops — never busy-wait
- Use `pdMS_TO_TICKS(ms)` instead of raw tick counts
- Delete a task from within itself: `vTaskDelete(nullptr)` at end of function
- Name tasks descriptively (visible in `idf.py monitor` task list)

## Event Groups / Queues / Semaphores
- Prefer event groups for broadcasting state to multiple tasks
- Prefer queues for passing data between tasks
- Use binary semaphores for task notification from ISR
- Always specify a finite timeout — never `portMAX_DELAY` in production code without justification

## NVS (Non-Volatile Storage)
```cpp
esp_err_t save_config(const Config& cfg) {
    nvs_handle_t h;
    ESP_RETURN_ON_ERROR(nvs_open("config", NVS_READWRITE, &h), TAG, "nvs_open");
    // Wrap h in RAII before any early-return paths
    NvsHandle handle(h);
    ESP_RETURN_ON_ERROR(nvs_set_u32(h, "value", cfg.value), TAG, "nvs_set");
    return nvs_commit(h);
}
```

## Kconfig / sdkconfig
- Expose tunable parameters via Kconfig — never hardcode values that operators may need to change
- Access config values via `CONFIG_MY_COMPONENT_VALUE` macros (generated from Kconfig)
- Group related options under a single `menu` in Kconfig
- Always provide sensible defaults in `sdkconfig.defaults`

## Wi-Fi / Network
- Use ESP-IDF event loop (`esp_event_loop_create_default`) for network events
- Register handlers with `esp_event_handler_register` — unregister on teardown
- Never block the event loop handler — defer heavy work to a task via queue

## Partition Table
- Default partition table fits most projects; use custom `partitions.csv` only when needed
- Always account for OTA partitions if OTA updates are required
- Use `CONFIG_PARTITION_TABLE_CUSTOM=y` in `sdkconfig.defaults` to activate custom table

## IDF Component Manager
For reusable components, publish via `idf_component.yml`:
```yaml
version: "1.0.0"
description: My reusable component
dependencies:
  idf:
    version: ">=6.0.0"
```

## Chip-Specific Code
- Use `#if CONFIG_IDF_TARGET_ESP32S3` guards for chip-specific code
- Prefer `idf_target` CMake property over preprocessor guards where possible
- Document which chips are supported in each component's README
