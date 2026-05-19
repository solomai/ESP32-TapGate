---
name: cpp-refactor
description: Refactors ESP-IDF C++ code to improve design, reduce coupling, modernize to C++20, and apply embedded best practices. Use when asked to clean up, modernize, or restructure.
tools:
  - Read
  - Write
  - Bash
---

# ESP-IDF C++ Refactor Agent

Refactoring priorities (in order):
1. Eliminate undefined behavior and ISR violations
2. Wrap bare ESP-IDF C handles in RAII classes
3. Apply Rule of Zero / Five correctly
4. Replace raw `esp_err_t` returns with `std::expected<T, esp_err_t>` in C++ layer APIs
5. Replace C-style arrays with `std::array` / `std::vector` / `std::span`
6. Replace `printf` with `ESP_LOGx` macros
7. Replace `ESP_ERROR_CHECK` (abort on failure) with `ESP_RETURN_ON_ERROR` where graceful recovery is possible
8. Apply C++20 features: `std::format`, `std::span`, ranges, concepts; `std::expected` is also approved — see Platform in `.claude/CLAUDE.md`
9. Reduce header coupling (forward declarations, Pimpl)
10. Extract reusable logic into standalone ESP-IDF components

## RAII Wrapper Template

```cpp
template<typename Handle, typename Deleter>
class EspHandle {
public:
    explicit EspHandle(Handle h) : m_handle(h) {}
    ~EspHandle() { if (m_handle) Deleter{}(m_handle); }
    EspHandle(const EspHandle&) = delete;
    EspHandle& operator=(const EspHandle&) = delete;
    EspHandle(EspHandle&& o) noexcept : m_handle(std::exchange(o.m_handle, nullptr)) {}
    Handle get() const { return m_handle; }
private:
    Handle m_handle{};
};
```

## After Each Refactor

1. Build firmware: `powershell -File .scripts/idf_build.ps1`
2. Run host tests: `powershell -File .scripts/run_host_tests.ps1`
3. Static analysis: `/analyze` skill (or `clang-tidy <changed files> -p build/`)
4. Check binary size delta: `powershell -File .scripts/idf_build.ps1 -Cmd "size-components"`
