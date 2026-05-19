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
- Always define `static constexpr char TAG[] = "module_name";` at file scope
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

// ctx must outlive the task — use static storage or heap with justification comment
xTaskCreate(task_worker, "worker", 4096, ctx, tskIDLE_PRIORITY + 1, nullptr);
```
- Always call `vTaskDelay` in infinite task loops — never busy-wait
- Use `pdMS_TO_TICKS(ms)` instead of raw tick counts
- Delete a task from within itself: `vTaskDelete(nullptr)` at end of function
- Name tasks descriptively (visible in `idf.py monitor` task list)

### Task Priority Conventions
Define named priority levels as `constexpr` constants to prevent priority inversion:
```cpp
constexpr UBaseType_t PRIORITY_LOW    = tskIDLE_PRIORITY + 1;
constexpr UBaseType_t PRIORITY_NORMAL = tskIDLE_PRIORITY + 2;
constexpr UBaseType_t PRIORITY_HIGH   = tskIDLE_PRIORITY + 3;  // ISR-deferred tasks
constexpr UBaseType_t PRIORITY_URGENT = configMAX_PRIORITIES - 1;
```
- ISR-deferred tasks must run at higher priority than their consumers to avoid deadline misses
- Never assign two unrelated tasks the same priority without a documented reason

## Event Groups / Queues / Semaphores
- Prefer event groups for broadcasting state to multiple tasks
- Prefer queues for passing data between tasks
- Prefer `xTaskNotifyFromISR` / `xTaskNotifyWait` over binary semaphores for single-consumer ISR-to-task signalling — lower overhead, no handle to manage
- Use binary semaphores only for multi-consumer notification or when a notify count is needed
- Always specify a finite timeout — never `portMAX_DELAY` in production code without justification

## NVS (Non-Volatile Storage)
```cpp
esp_err_t save_config(const Config& cfg) {
    nvs_handle_t h;
    ESP_RETURN_ON_ERROR(nvs_open("config", NVS_READWRITE, &h), TAG, "nvs_open");
    // Wrap h in RAII before any early-return paths; use handle.get() for subsequent calls
    NvsHandle handle(h);
    ESP_RETURN_ON_ERROR(nvs_set_u32(handle.get(), "value", cfg.value), TAG, "nvs_set");
    return nvs_commit(handle.get());
}
```
`NvsHandle` RAII definition is in `.claude/rules/memory.md` — it exposes `get()` for raw handle access.

## Kconfig / sdkconfig
- Expose tunable parameters via Kconfig — never hardcode values that operators may need to change
- Access config values via `CONFIG_MY_COMPONENT_VALUE` macros (generated from Kconfig)
- Group related options under a single `menu` in Kconfig
- Always provide sensible defaults in `sdkconfig.defaults`

## Wi-Fi / Network
- Use ESP-IDF event loop (`esp_event_loop_create_default`) for network events
- Register handlers with `esp_event_handler_instance_register` (returns an `esp_event_handler_instance_t` handle); unregister with `esp_event_handler_instance_unregister` on teardown — the deprecated `esp_event_handler_register` does not support reliable per-instance unregistration
- Never block the event loop handler — defer heavy work to a task via queue

## OTA Updates
When OTA is required, use two OTA app partitions and the `esp_ota_ops.h` API:
```cpp
esp_err_t perform_ota(const char* url) {
    esp_ota_handle_t handle;
    const esp_partition_t* target = esp_ota_get_next_update_partition(nullptr);
    ESP_RETURN_ON_ERROR(esp_ota_begin(target, OTA_WITH_SEQUENTIAL_WRITES, &handle), TAG, "ota_begin");
    // ... write chunks via esp_ota_write(handle, data, len) ...
    ESP_RETURN_ON_ERROR(esp_ota_end(handle), TAG, "ota_end");
    ESP_RETURN_ON_ERROR(esp_ota_set_boot_partition(target), TAG, "ota_set_boot");
    esp_restart();
    return ESP_OK;
}
```
- Enable rollback: set `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` in `sdkconfig.defaults`
- Call `esp_ota_mark_app_valid_cancel_rollback()` after confirming the new firmware is functional
- Always verify the downloaded image via `esp_image_verify()` before calling `esp_ota_set_boot_partition`
- Partition table must include two `app` OTA entries (`ota_0`, `ota_1`) and an `otadata` partition

## Partition Table
- Default partition table fits most projects; use custom `partitions.csv` only when needed
- OTA requires a custom partition table — add `ota_0`, `ota_1`, and `otadata` entries
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
