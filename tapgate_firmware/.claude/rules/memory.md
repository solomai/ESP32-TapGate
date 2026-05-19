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
Wrap all FreeRTOS handles in RAII structs (`std::unique_ptr<void>` does not compile — `void` is an incomplete type):
```cpp
// Task RAII wrapper
struct OwnedTask {
    TaskHandle_t handle{nullptr};
    explicit OwnedTask(TaskHandle_t h) : handle(h) {}
    ~OwnedTask() { if (handle) vTaskDelete(handle); }
    OwnedTask(const OwnedTask&) = delete;
    OwnedTask& operator=(const OwnedTask&) = delete;
};

// Queue RAII wrapper
struct OwnedQueue {
    QueueHandle_t handle{nullptr};
    explicit OwnedQueue(QueueHandle_t h) : handle(h) {}
    ~OwnedQueue() { if (handle) vQueueDelete(handle); }
    OwnedQueue(const OwnedQueue&) = delete;
    OwnedQueue& operator=(const OwnedQueue&) = delete;
};
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
    nvs_handle_t get() const { return m_handle; }
    NvsHandle(const NvsHandle&) = delete;
    NvsHandle& operator=(const NvsHandle&) = delete;
private:
    nvs_handle_t m_handle;
};
```

## Stack Sizes
- Default `app_main` stack: 8 KB — increase via `CONFIG_ESP_MAIN_TASK_STACK_SIZE` in `sdkconfig.defaults`
- Size custom tasks: start at **4096 bytes** for simple tasks; **8192 bytes** for tasks using networking, C++ STL, or JSON parsing
- Profile with `uxTaskGetStackHighWaterMark(nullptr)` during development; trim to leave ≥ 20% headroom

## Safety Checks (Host Tests)
For host-side unit tests, run AddressSanitizer + UBSan:
```powershell
# Requires Clang or GCC — run under WSL or a MinGW toolchain on Windows (MSVC does not support -fsanitize)
cmake -B build_tests_host -G Ninja `
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" `
      -DCMAKE_BUILD_TYPE=Debug tests_host/
cmake --build build_tests_host
ctest --test-dir build_tests_host --output-on-failure
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

## DMA Buffers
Buffers passed to DMA peripherals (SPI, I2S, USB) have strict requirements:
- Allocate with `heap_caps_malloc(size, MALLOC_CAP_DMA)` or declare with `DMA_ATTR` macro
- On ESP32-S3 with PSRAM: flush cache before DMA write, invalidate after DMA read via `esp_cache_msync()`
- Never use stack-allocated buffers for DMA — the stack may be in IRAM or non-DMA-accessible DRAM

```cpp
// Heap: DMA-capable buffer required for SPI transfer
DMA_ATTR static uint8_t s_tx_buf[64];

// OR dynamic allocation with DMA capability
// Heap: DMA-capable, size determined by protocol frame size
uint8_t* dma_buf = static_cast<uint8_t*>(heap_caps_malloc(len, MALLOC_CAP_DMA));
```
See also: `.claude/rules/esp-idf.md` for peripheral API patterns.

## Forbidden
- `reinterpret_cast` on non-POD types without justification comment
- Returning pointer/reference to local variable
- `std::shared_ptr` cycles without `std::weak_ptr` breaker
- Dynamic allocation in ISR context
- Blocking calls in ISR context (`vTaskDelay`, mutex lock, etc.)
- Unbounded stack growth in recursive functions running in tasks
- Heap allocation without a justification comment
- `std::vector` / `std::string` / `std::map` in ISR handlers or tasks with stack < 4 KB
- See also ISR constraints: `.claude/rules/code-style.md` ISR Context Rules and `.claude/rules/embedded-cpp.md` Heap Allocation rules
