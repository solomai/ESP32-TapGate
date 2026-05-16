---
description: Memory safety and resource management rules for ESP-IDF / FreeRTOS
---

# Memory Management Rules

## Ownership (C++ layer)
- Owning raw pointer → `std::unique_ptr<T>`
- Shared ownership → `std::shared_ptr<T>` (justify in comment; watch heap fragmentation)
- Non-owning reference → raw pointer `T*` or `std::span<T>`
- Never call `new`/`delete` directly outside custom allocators
- Do not use C-style casts — use `static_cast` / `reinterpret_cast`

## ESP32 Heap Considerations
- ESP32 has multiple heap regions: DRAM, IRAM, PSRAM (if present), RTC
- Use `heap_caps_malloc(size, MALLOC_CAP_*)` only when a specific capability is required:
  - `MALLOC_CAP_DMA` — DMA-accessible memory
  - `MALLOC_CAP_EXEC` — executable memory (IRAM)
  - `MALLOC_CAP_SPIRAM` — external PSRAM
- Default `new` / `malloc` allocates from DRAM; this is correct for most cases
- Monitor heap fragmentation in long-running applications: `heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)`
- Prefer static or stack allocation for fixed-size buffers in tasks

## FreeRTOS Resource Ownership
Wrap all FreeRTOS handles in RAII classes:
```cpp
// Task
struct TaskDeleter { void operator()(TaskHandle_t h) { vTaskDelete(h); } };
using TaskHandle = std::unique_ptr<void, TaskDeleter>;

// Queue
struct QueueDeleter { void operator()(QueueHandle_t h) { vQueueDelete(h); } };
using QueueHandle = std::unique_ptr<void, QueueDeleter>;
```
- Use `std::mutex` / `std::lock_guard` when possible; fall back to FreeRTOS primitives only when interrupt-context access is needed
- Use `SemaphoreHandle_t` with `xSemaphoreCreateMutex()` only for ISR-accessible mutexes; wrap in RAII
- Never use raw `TaskHandle_t` as an owning handle

## RAII Pattern for ESP-IDF Resources
```cpp
class NvsHandle {
public:
    explicit NvsHandle(nvs_handle_t h) : m_handle(h) {}
    ~NvsHandle() { nvs_close(m_handle); }
    NvsHandle(const NvsHandle&) = delete;
    NvsHandle& operator=(const NvsHandle&) = delete;
private:
    nvs_handle_t m_handle;
};
```

## Stack Sizes
- Default `app_main` stack: 8 KB — increase via `CONFIG_ESP_MAIN_TASK_STACK_SIZE` in `sdkconfig.defaults`
- Size custom tasks appropriately: `configMINIMAL_STACK_SIZE * 4` as a starting baseline
- Use `uxTaskGetStackHighWaterMark(NULL)` during development to tune stack sizes

## Safety Checks (Host Tests)
For host-side unit tests, run AddressSanitizer + UBSan:
```bash
cmake -B test/build -G Ninja \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
      -DCMAKE_BUILD_TYPE=Debug test/
cmake --build test/build -j$(nproc)
ctest --test-dir test/build --output-on-failure
```
Note: ASan/UBSan cannot run on the target chip. Run sanitizers on host tests only.

## Default Allocation Strategy

Stack and static are the default. Heap is the exception, not the rule.

| Storage class | When to use |
|---|---|
| Stack (`T obj;`) | Short-lived, bounded size (< 512 bytes), single task |
| Static (`static T obj;`) | Module-level singletons, fixed-size buffers, long lifetime |
| Heap (`std::make_unique`) | Dynamic size OR lifetime must exceed enclosing scope — add a justification comment |

Every heap allocation (`new`, `std::make_unique`, `std::make_shared`, `heap_caps_malloc`) must be
accompanied by a comment: `// Heap: <reason why stack/static is not viable>`

## Forbidden
- `reinterpret_cast` on non-POD types without justification comment
- Returning pointer/reference to local variable
- `std::shared_ptr` cycles without `std::weak_ptr` breaker
- Dynamic allocation in ISR context
- Blocking calls in ISR context (`vTaskDelay`, mutex lock, etc.)
- Unbounded stack growth in recursive functions running in tasks
- Heap allocation without a justification comment
- `std::vector` / `std::string` / `std::map` in ISR handlers or tasks with stack < 4 KB
