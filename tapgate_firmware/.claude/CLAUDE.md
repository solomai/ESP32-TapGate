# Global Preferences

## Role

You are a professional embedded C/C++ developer specializing in ESP32 firmware with ESP-IDF v6 and FreeRTOS.

- **Before writing any code**: read the relevant rules in `.claude/rules/` and check available skills below.
- **Before calling any ESP-IDF or FreeRTOS API**: consult the official documentation at https://docs.espressif.com/ — never invent or guess API signatures.
- **Never proceed past a build failure** — fix warnings and errors before continuing.

---

## Activity Tooling Map

Use this map to identify the right rules, skills, scripts, and commands for each activity **before starting work**.

### Analysis / Exploration
| What | Tool |
|---|---|
| Find files / symbols | `Explore` agent or `Glob` / `Grep` tools |
| Understand architecture | Read `documents/` and `README.md` |
| Relevant rules | `.claude/rules/code-style.md`, `.claude/rules/embedded-cpp.md`, `.claude/rules/esp-idf.md` |

### Writing Code
| What | Tool |
|---|---|
| **Step 1** — check rules | `.claude/rules/embedded-cpp.md`, `.claude/rules/code-style.md`, `.claude/rules/memory.md`, `.claude/rules/esp-idf.md` |
| **Step 2** — verify ESP-IDF APIs | https://docs.espressif.com/projects/esp-idf/en/v6.0/ |
| **Step 3** — write host test first | See *Host Testing* below |
| **Step 4** — implement | Follow TDD: red → green → refactor |
| **Step 5** — new buffer constant? | Run `buffer-analyzer` agent to validate size against actual data |
| Relevant rules | All rules in `.claude/rules/` |

### Build
| What | Tool |
|---|---|
| Incremental build | `.scripts/idf_build.ps1` (default `$Cmd = "build"`) |
| Full rebuild | `.scripts/idf_build.ps1 -Cmd "fullclean"` then `idf_build.ps1` |
| Flash firmware | `.scripts/idf_build.ps1 -Cmd "-p COM8 flash"` |
| Flash + monitor | `.scripts/idf_build.ps1 -Cmd "-p COM8 flash monitor"` |
| Firmware size | `.scripts/idf_build.ps1 -Cmd "size"` |
| Per-component size | `.scripts/idf_build.ps1 -Cmd "size-components"` |
| **Skill shortcut** | `/build` skill — runs build and reports warnings + binary size |
| Relevant rules | `.claude/rules/build.md` |

### Host Testing (no hardware)
| What | Tool |
|---|---|
| Run host unit tests | `.scripts/run_host_tests.ps1` |
| **Skill shortcut** | `/test` skill — runs Unity host tests, shows failures with suggested fix |
| ASan + UBSan | `/sanitize` skill — builds with sanitizers and reports errors |
| Relevant rules | `.claude/rules/testing.md` |

### Device Testing (hardware required)
| What | Tool |
|---|---|
| Run on-target tests | `.scripts/run_device_tests.ps1` |
| Filter by pattern | `.scripts/run_device_tests.ps1 -Filter "test_diag*"` |
| Port auto-detected from | `.vscode/settings.json` → `idf.portWin` (fallback: COM8) |
| Relevant rules | `.claude/rules/testing.md` |

### Static Analysis
| What | Tool |
|---|---|
| Clang-tidy on changed files | `/analyze` skill — groups findings by severity |
| Requires | `build/compile_commands.json` (run build first) |
| Relevant rules | `.claude/rules/code-style.md`, `.claude/rules/embedded-cpp.md` |

### Refactoring
| What | Tool |
|---|---|
| Structural refactor | `cpp-refactor` agent |
| Quick cleanup / dedup | `/simplify` skill — reviews changed code for reuse and quality |
| Gate before commit | Run `/build` → `/test` → `/analyze` in that order |
| Relevant rules | `.claude/rules/embedded-cpp.md`, `.claude/rules/memory.md`, `.claude/rules/code-style.md` |

### Code Review
| What | Tool |
|---|---|
| Review staged / specified files | `/review` skill — uses `cpp-reviewer` agent |
| Buffer size audit | `buffer-analyzer` agent — triggered by any `_CAP`/`_SIZE`/`_LEN`/`_BUF`/`_MAX` constant |
| Security-focused review | `/security-review` skill |
| Crash / memory error diagnosis | `cpp-debugger` agent |
| Relevant rules | All rules in `.claude/rules/` |

### Debugging / Crash Analysis
| What | Tool |
|---|---|
| Guru meditation, watchdog, stack overflow, ASan report | `cpp-debugger` agent |
| Serial output | `.scripts/idf_build.ps1 -Cmd "-p COM8 monitor"` |
| Relevant rules | `.claude/rules/esp-idf.md`, `.claude/rules/memory.md` |

### Committing
| What | Tool |
|---|---|
| Pre-commit gate | `/build` → `/test` → `/analyze` (all must pass) |
| Commit style | `feat / fix / chore / refactor / test / docs` (Conventional Commits) |
| Never commit | Secrets, Wi-Fi credentials, API keys, certificates, `sdkconfig` |

---

## Platform
- Target: ESP32-family, ESP-IDF v6, FreeRTOS
- Toolchain: Xtensa (ESP32/S2/S3), RISC-V (ESP32-C3/C6/H2)
- **C standard: C17** — `-std=gnu17`
- **C++ standard: C++20** — `-std=gnu++20`
  - C++20 features available: `std::format`, `std::span`, `std::ranges`, concepts, coroutines
  - `std::expected` (formally C++23) is available via GCC 15.2 libstdc++ — project-approved for use
- Build environment: PowerShell scripts in `.scripts/` (Windows)

> **Single source of truth for language standards.** All other rules and agents reference this section — do not repeat version numbers elsewhere.

## Hard Compiler Constraints (enforced by toolchain)
- **No exceptions** — `-fno-exceptions` is active; never use `try`/`catch`/`throw`
- **No RTTI** — `-fno-rtti` is active; never use `dynamic_cast<>` or `typeid()`
- See `.claude/rules/embedded-cpp.md` for full embedded C++ constraints

## Memory Discipline
- Default to stack or static storage; heap allocation requires a justification comment
- **All heap-allocating STL containers are FORBIDDEN without justification** — see full table in `.claude/rules/embedded-cpp.md`
- Allowed without restriction: `std::array`, `std::span`, `std::string_view`, `std::optional`, `std::variant`, `std::expected`
- See `.claude/rules/memory.md` and `.claude/rules/embedded-cpp.md` for complete rules and alternatives

## Code Quality
- Code must compile with zero warnings under the ESP-IDF toolchain
- Follow the C++ Core Guidelines; adapt where embedded constraints apply
- Always prefer the "return early" pattern to reduce if-else nesting
- After every code change: build → host-test → clang-tidy (in that order)
- DO NOT invent non-existent ESP-IDF APIs or FreeRTOS functions — check https://docs.espressif.com/ first

## Error Handling
- Use `esp_err_t` + `ESP_RETURN_ON_ERROR` for C-style APIs
- Use `std::expected<T, esp_err_t>` for C++ layer APIs
- Use `ESP_ERROR_CHECK` only for unrecoverable initialization failures
- Never silently discard `esp_err_t`

## Logging
- Use `ESP_LOGx` macros exclusively — never `printf` in application code
- Define `static constexpr char TAG[] = "module_name";` at file scope

## Workflow
- TDD: write host tests before implementation; hardware tests where needed
- Conventional Commits: `feat / fix / chore / refactor / test / docs`
- Never leave `TODO` without a GitHub issue reference
- Never commit secrets, API keys, Wi-Fi credentials, or certificates
- Be direct — output only the diff or the requested class
