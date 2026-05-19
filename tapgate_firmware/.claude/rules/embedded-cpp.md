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
- All heap-allocating STL containers are **FORBIDDEN** without an explicit justification comment — see table below
- See also `.claude/rules/code-style.md` ISR Context Rules and `.claude/rules/memory.md` Forbidden list for the full ISR constraint set

## Heap-Allocating STL Containers — FORBIDDEN Without Justification

**Default answer is NO.** Any container that can dynamically allocate memory is forbidden unless justified.
Every heap-allocating container in firmware introduces non-deterministic allocation time, fragmentation
risk, and silent failure when memory is exhausted (`bad_alloc` is disabled with `-fno-exceptions`).

### Forbidden — require heap allocation

| Container | Reason | Required replacement |
|---|---|---|
| `std::vector<T>` | Dynamic array, grows on heap | `std::array<T, N>`, static buffer |
| `std::string` | Dynamic string on heap | `std::string_view` (read-only), `char[]` (mutable) |
| `std::map<K, V>` | Red-black tree, every node heap-allocated | Sorted `std::array` + `std::lower_bound` |
| `std::multimap<K, V>` | Same as `std::map` | Sorted `std::array` |
| `std::set<T>` | Red-black tree, every node heap-allocated | Sorted `std::array` + binary search |
| `std::multiset<T>` | Same as `std::set` | Sorted `std::array` |
| `std::unordered_map<K, V>` | Hash table, heap-allocated + non-deterministic | Sorted `std::array` or open-address table |
| `std::unordered_set<T>` | Same as `std::unordered_map` | Sorted `std::array` |
| `std::list<T>` | Linked list, every node heap-allocated | Ring buffer, `std::array` with index wrap |
| `std::forward_list<T>` | Same as `std::list` | Ring buffer |
| `std::deque<T>` | Segmented array on heap | `std::array` with index wrap |
| `std::queue<T>` | Backed by `std::deque` by default | FreeRTOS queue (`xQueueCreate`) |
| `std::stack<T>` | Backed by `std::deque` by default | `std::array` used as a stack |
| `std::priority_queue<T>` | Backed by `std::vector` by default | Fixed `std::array` + `std::push_heap` |
| `std::function` | Heap-allocates for closures exceeding SBO | Function pointer or template parameter |
| `std::regex` | Heap-heavy, non-deterministic | Manual parsing |
| `std::thread` | OS thread, forbidden on FreeRTOS | `xTaskCreate` |
| `std::filesystem` | Not available on ESP32 | POSIX / SPIFFS / LittleFS APIs |

### Allowed — no dynamic allocation

| Type | Notes |
|---|---|
| `std::array<T, N>` | Fixed-size; lives on stack or static storage |
| `std::span<T>` | Non-owning view; zero overhead |
| `std::string_view` | Non-owning view over char data |
| `std::optional<T>` | In-place, no heap |
| `std::variant<T...>` | In-place; avoid RTTI-based type dispatch |
| `std::pair<A, B>` | In-place |
| `std::tuple<T...>` | In-place |
| `std::bitset<N>` | Fixed-size bit array |
| `std::atomic<T>` | Lock-free, no heap |
| `std::expected<T, E>` | In-place; project-approved (see Platform in `.claude/CLAUDE.md`) |
| `std::mutex` | Acceptable in tasks; use FreeRTOS primitive in ISR context |
| Algorithm headers | `<algorithm>`, `<numeric>`, `<ranges>` — no allocation |

### Smart pointers — allowed with mandatory justification

The pointer itself is stack-sized; it *manages* a heap object. Every use must carry a comment.

| Type | Rule |
|---|---|
| `std::unique_ptr<T>` | Allowed — mandatory `// Heap: <reason>` comment at use site |
| `std::shared_ptr<T>` | Use sparingly — adds reference-count heap overhead; prefer `unique_ptr` |

### When heap allocation is unavoidable

If a heap-allocating container is genuinely necessary, the use site must carry:
```cpp
// Heap: <specific reason — e.g., size determined at runtime from NVS config, max bounded by X>
std::vector<uint8_t> buf(size);
```
Prefer allocating once at startup and reusing, rather than allocating repeatedly at runtime.

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
