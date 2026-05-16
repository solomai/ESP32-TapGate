# Global Preferences

## Platform
- Target: ESP32-family, ESP-IDF v6, FreeRTOS
- Toolchain: Xtensa/RISC-V, C++23 subset supported by ESP-IDF

## Hard Compiler Constraints (enforced by toolchain)
- **No exceptions** — `-fno-exceptions` is active; never use `try`/`catch`/`throw`
- **No RTTI** — `-fno-rtti` is active; never use `dynamic_cast<>` or `typeid()`
- See `.claude/rules/embedded-cpp.md` for full embedded C++ constraints

## Memory Discipline
- Default to stack or static storage; heap allocation requires a justification comment
- Never use heap-allocating STL containers in ISR context or tasks with stack < 4 KB
- Avoid `std::vector`, `std::string`, `std::map` without justification
- See `.claude/rules/memory.md` and `.claude/rules/embedded-cpp.md`

## Code Quality
- Code must compile with zero warnings under the ESP-IDF toolchain
- Follow the C++ Core Guidelines; adapt where embedded constraints apply
- Always prefer the "return early" pattern to reduce if-else nesting
- After every code change: build → host-test → clang-tidy (in that order)
- DO NOT invent non-existent ESP-IDF APIs or FreeRTOS functions — check docs first

## Error Handling
- Use `esp_err_t` + `ESP_RETURN_ON_ERROR` for C-style APIs
- Use `std::expected<T, esp_err_t>` for C++ layer APIs
- Use `ESP_ERROR_CHECK` only for unrecoverable initialization failures
- Never silently discard `esp_err_t`

## Logging
- Use `ESP_LOGx` macros exclusively — never `printf` in application code
- Define `static const char* TAG = "module_name";` at file scope

## Workflow
- TDD: write host tests before implementation; hardware tests where needed
- Conventional Commits: `feat / fix / chore / refactor / test / docs`
- Never leave `TODO` without a GitHub issue reference
- Never commit secrets, API keys, Wi-Fi credentials, or certificates
- Be direct — output only the diff or the requested class
