---
description: Embedded C++ constraints for ESP32 — no exceptions, no RTTI, minimal heap
---

# Embedded C++ Rules

ESP-IDF disables exceptions and RTTI by default (`-fno-exceptions -fno-rtti`).
These are hard toolchain constraints — never write code that requires them.

## Exceptions — FORBIDDEN

- Never use `try`, `catch`, `throw`
- Never use types or functions from `<stdexcept>`, `<exception>`
- Error propagation: use `esp_err_t` + `ESP_RETURN_ON_ERROR`, or `std::expected<T, esp_err_t>`
- Constructors that can fail must use a factory function returning `std::expected` or `esp_err_t`

```cpp
// WRONG
class Sensor {
public:
    Sensor() { if (!init()) throw std::runtime_error("fail"); }
};

// RIGHT
class Sensor {
public:
    [[nodiscard]] static std::expected<Sensor, esp_err_t> create();
private:
    Sensor() = default;
};
```

## RTTI — FORBIDDEN

- Never use `dynamic_cast<>` — use `static_cast<>` with explicit type knowledge
- Never use `typeid()` or `std::type_info`
- Never use `std::any`, `std::variant` with type-based dispatch via RTTI
- Design polymorphic hierarchies so virtual dispatch is sufficient; avoid downcasting

## Heap Allocation — AVOID, JUSTIFY EVERY USE

Default: **stack or static storage**. Heap is the last resort.

### Prefer static / stack over heap:
```cpp
// Prefer: static buffer
static uint8_t s_rx_buf[256];

// Prefer: stack array for small, bounded sizes
uint8_t frame[64];

// Prefer: statically-allocated objects
static MyDriver s_driver;

// Acceptable heap use (must be justified in a comment):
// Reason: lifetime exceeds stack frame, size determined at runtime
auto buf = std::make_unique<uint8_t[]>(dynamic_size);
```

### Rules:
- Every `new` / `std::make_unique` / `std::make_shared` must have a comment explaining why heap is required
- Prefer `std::array<T, N>` over `std::vector<T>` for fixed-size collections
- Never use `std::vector`, `std::string`, `std::map`, `std::list` in:
  - ISR handlers
  - Tasks with stack < 4 KB
  - Code paths called at > 100 Hz (hot paths)
- See also `.claude/rules/code-style.md` ISR Context Rules and `.claude/rules/memory.md` Forbidden list for the full ISR constraint set
- `std::string` — avoid entirely; use `std::string_view` for read-only string access, `char[]` for buffers
- `std::map` / `std::unordered_map` — avoid; use sorted `std::array` + binary search for small tables

## Heavy STL — RESTRICTED

| Container / Type | Status | Alternative |
|---|---|---|
| `std::vector` | Avoid without justification | `std::array<T,N>`, static buffer |
| `std::string` | Avoid | `std::string_view`, `char[]` |
| `std::map` | Avoid | sorted `std::array` + `std::lower_bound` |
| `std::unordered_map` | Forbidden in firmware | sorted array or custom hash table |
| `std::list` / `std::deque` | Forbidden | ring buffer, FreeRTOS queue |
| `std::shared_ptr` | Use sparingly | `std::unique_ptr` preferred |
| `std::function` | Avoid | function pointer or template |
| `std::regex` | Forbidden | manual parsing |
| `std::filesystem` | Forbidden | POSIX / SPIFFS / LittleFS APIs |
| `std::thread` | Forbidden | FreeRTOS `xTaskCreate` |
| `std::mutex` (bare) | Acceptable in tasks | FreeRTOS primitive in ISR context |

Allowed without restriction: `std::array`, `std::span`, `std::optional`, `std::expected`,
`std::string_view`, `std::pair`, `std::tuple`, `std::variant` (no RTTI dispatch),
`std::bitset`, `std::atomic`, algorithm headers (`<algorithm>`, `<numeric>`, `<ranges>`).

## Virtual Functions

- Virtual functions are allowed but carry vtable overhead (~4 bytes/pointer + vtable per class)
- Prefer non-virtual interfaces in hot paths; use `final` to enable devirtualization
- Use `virtual ~Base() = default;` in abstract base classes; use `virtual ~Base() = 0` with an out-of-line definition only when no other pure virtual function exists to mark the class abstract
- Limit virtual dispatch depth in interrupt-driven paths

## Templates

- Use templates freely for zero-cost abstractions
- Avoid deep recursive template instantiation (increases binary size)
- Prefer `if constexpr` over SFINAE for readability
- Use `constexpr` / `consteval` aggressively to move computation to compile time

## Object Sizes and Alignment

- Be aware of struct padding — use `static_assert(sizeof(T) == N)` to verify ABI-sensitive layouts
- Use `__attribute__((packed))` only for wire-format structs; never for general use
- Large objects (> 512 bytes) should not live on the stack — use static or heap with justification

## Logging
- Never use `std::cout` or C++ streams; use ESP-IDF logging macros (`ESP_LOGI`, `ESP_LOGE`, etc.)
- Avoid logging in hot paths or ISRs; use flags to enable/disable verbose logging
- Avoid logging under mutex locks to prevent deadlocks
- Log messages should be concise and informative; avoid large formatted strings in performance-critical code
