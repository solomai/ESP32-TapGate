# Embedded C++ Guidelines for ESP32

This document describes the C++ constraints enforced in this project. They exist to keep firmware
reliable, deterministic, and within the memory budget of ESP32-family devices.

## Why These Restrictions?

ESP-IDF compiles all firmware with:
- `-fno-exceptions` — exceptions are disabled
- `-fno-rtti` — run-time type information is disabled

Both are disabled by default because on a microcontroller with 256–512 KB RAM they add
unacceptable overhead and unpredictable latency. The rules below follow from these constraints
and from general embedded best practices.

## Exceptions — Not Available

The toolchain will reject any code that uses `try`, `catch`, or `throw`.

**Use instead:**
- `esp_err_t` + `ESP_RETURN_ON_ERROR` for functions that can fail
- `std::expected<T, esp_err_t>` for C++ API return values (project-approved; see Platform in `.claude/CLAUDE.md`)
- Factory functions for constructors that can fail

```cpp
// Constructors cannot throw — use a factory
class WifiManager {
public:
    [[nodiscard]] static std::expected<WifiManager, esp_err_t> create(const Config& cfg);
private:
    explicit WifiManager(const Config& cfg);
};
```

## RTTI — Not Available

`dynamic_cast<>` and `typeid()` are not available.

**Use instead:**
- `static_cast<>` — only when you know the actual type at the call site
- Virtual dispatch — design hierarchies so downcasting is never needed
- Tag-dispatch or `std::variant` — for explicit type discrimination without RTTI

## Heap Allocation — Use Sparingly

The ESP32 heap is shared between FreeRTOS, WiFi/BT stack, and application code.
Heap fragmentation in long-running firmware causes hard-to-reproduce crashes.

**Default to stack or static storage:**

```cpp
// Prefer: static for module-level objects
static SpiDriver s_spi;

// Prefer: stack arrays for small buffers
uint8_t frame[64];

// Prefer: std::array for fixed-size collections
std::array<Sensor, 4> sensors;

// Heap is acceptable when:
//   - lifetime must exceed the enclosing scope, AND
//   - size is not known at compile time
// Always add a comment explaining why:
// Reason: buffer size depends on NVS config value read at startup
auto rx_buf = std::make_unique<uint8_t[]>(rx_buf_size);
```

**Forbidden without justification:**
- `new` / `delete` directly
- `std::vector`, `std::string`, `std::map` in ISR handlers or hot paths (> 100 Hz)
- Any heap allocation inside ISR handlers

## STL Container Reference

| Container | Guidance |
|---|---|
| `std::array<T, N>` | Preferred for fixed-size sequences |
| `std::span<T>` | Preferred for non-owning views |
| `std::string_view` | Preferred for read-only string access |
| `std::optional<T>` | Allowed; use for nullable values |
| `std::expected<T, E>` | Preferred for error-returning APIs |
| `std::variant<...>` | Allowed; avoid type-based dispatch via RTTI |
| `std::bitset<N>` | Allowed |
| `std::atomic<T>` | Allowed; use for shared variables between tasks/ISR |
| `std::vector<T>` | Avoid; use `std::array` or static buffer |
| `std::string` | Avoid; use `std::string_view` or `char[]` |
| `std::map` / `std::set` | Avoid; use sorted array + `std::lower_bound` |
| `std::unordered_map` | Forbidden in firmware |
| `std::list` / `std::deque` | Forbidden; use ring buffer or FreeRTOS queue |
| `std::function` | Avoid in hot paths (heap alloc for captures); use function pointer |
| `std::thread` | Forbidden; use FreeRTOS `xTaskCreate` |
| `std::regex` | Forbidden; implement manual parsing |
| `std::filesystem` | Forbidden; use POSIX / SPIFFS / LittleFS APIs |

## Virtual Functions and Polymorphism

Virtual functions are allowed but carry overhead:
- A vtable pointer per object (4–8 bytes)
- One vtable per polymorphic class (stored in flash, not RAM)
- Indirect call prevents some compiler optimizations

Guidelines:
- Use `final` on leaf classes to allow devirtualization
- Avoid virtual dispatch on the critical path (ISR, > 1 kHz loops)
- Prefer CRTP (Curiously Recurring Template Pattern) for zero-cost polymorphism in hot paths

## Object Size and Stack Usage

- Objects larger than 512 bytes should not live on the task stack
- Default `app_main` stack is 8 KB; custom tasks typically get 4–8 KB
- Use `uxTaskGetStackHighWaterMark(nullptr)` during development to measure actual stack usage
- Use `static_assert(sizeof(T) == N)` to guard ABI-sensitive structs

## Compile-Time Computation

Prefer `constexpr` and `consteval` to move work to compile time:

```cpp
constexpr uint32_t ms_to_ticks(uint32_t ms) {
    return ms * configTICK_RATE_HZ / 1000u;
}

static_assert(ms_to_ticks(100) == 10);   // verified at compile time
```

## Further Reading

- [ESP-IDF C++ Support](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/cplusplus.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- `.claude/rules/embedded-cpp.md` — Claude Code enforcement rules
- `.claude/rules/memory.md` — RAII and memory management rules
