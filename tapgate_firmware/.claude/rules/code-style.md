---
description: C++ coding style and idioms for ESP-IDF projects
---

# C++ Style Rules

## Naming
- File names: `snake_case`
- Types/Classes: `PascalCase`
- Functions: `snake_case`
- Variables: `snake_case`
- Constants/Enums: `UPPER_SNAKE_CASE`
- Member variables: `m_name`
- Template parameters: `T`, `TValue`, `TKey`
- FreeRTOS task functions: `task_<name>` with signature `void task_<name>(void* arg)`

## Modern C++ Idioms (ESP-IDF toolchain supports C++23)
- Prefer `auto` when type is obvious from context
- Use range-for over index loops unless index is needed
- Use structured bindings: `auto [key, val] = pair`
- Use `std::span` for non-owning array views
- Use `[[nodiscard]]` on functions returning `esp_err_t` or error codes
- Use `constexpr` / `consteval` for compile-time computation
- Use `std::format` (C++23) instead of `printf` / `sprintf` in non-ISR code
- Prefer `std::expected<T, esp_err_t>` for error handling in C++ APIs

## ESP-IDF Specific Style
- Use `ESP_LOGx(TAG, ...)` macros for all logging — never `printf` in app code
- Define `static const char* TAG = "module_name";` at file scope for log tags
- Use `ESP_ERROR_CHECK(expr)` for unrecoverable errors (aborts on failure)
- Use `ESP_RETURN_ON_ERROR(expr, TAG, fmt, ...)` for recoverable errors in functions returning `esp_err_t`
- Use `ESP_GOTO_ON_ERROR` when cleanup via `goto` is needed
- Return `esp_err_t` from C-style APIs; use `std::expected` in C++ layer APIs
- Wrap C ESP-IDF handles in RAII classes rather than managing them manually

## Classes
- Rule of Zero preferred; Rule of Five if manual resource management
- Mark single-arg constructors `explicit`
- Mark overriding methods `override`; mark non-overridable `final`
- Prefer free functions over member functions when no private access needed
- FreeRTOS resources (tasks, queues, semaphores) must be owned by RAII wrappers

## Includes
- Order: own header → C++ stdlib → ESP-IDF headers → third-party → project headers
- Always include what you use (IWYU)
- Do not use `using namespace std` in headers

## Header Files
- Always add `#pragma once` at the top of header files

## ISR Context Rules
- No dynamic allocation (`new`, `malloc`, STL containers) in ISR handlers
- No blocking calls (`vTaskDelay`, mutex lock, `printf`) in ISR handlers
- Use `portYIELD_FROM_ISR()` / `BaseType_t higher_priority_task_woken` pattern
- Prefer `xQueueSendFromISR` / `xSemaphoreGiveFromISR` to communicate from ISR

## Forbidden
- C-style casts: use `static_cast` / `reinterpret_cast` (never `dynamic_cast` — RTTI disabled)
- `using namespace std` in any header file
- Global mutable state without explicit justification comment
- Magic numbers — use named `constexpr` constants
- `printf` in application code — use `ESP_LOGx` instead
- Direct `malloc`/`free` — use `heap_caps_malloc` if specific heap is needed, otherwise RAII
- `try` / `catch` / `throw` — exceptions are disabled (`-fno-exceptions`)
- `dynamic_cast<>` / `typeid()` — RTTI is disabled (`-fno-rtti`)
- `std::vector`, `std::string`, `std::map` without justification comment explaining why heap is needed
- `std::unordered_map`, `std::list`, `std::deque`, `std::regex`, `std::thread`, `std::filesystem` — forbidden in firmware
- `std::function` in hot paths or ISR — use function pointer or template instead
- Heap allocation in ISR handlers — use static buffers and FreeRTOS queues
