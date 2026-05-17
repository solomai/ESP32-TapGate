---
description: Testing conventions for ESP-IDF projects (host, on-target, HW-in-the-loop)
---

# Testing Rules — ESP-IDF v6

## Three Test Tiers

| Tier | Framework | Location | Hardware needed | When to run |
|---|---|---|---|---|
| Host unit tests | Unity | `tests_host/` | No | Every commit |
| On-target unit tests | Unity (ESP-IDF) | `tests/` | Yes | PR / release |
| HW-in-the-loop | pytest-embedded | `pytest_tests/` | Yes | Release gate |

## Test / Production Boundary — HARD RULES

**Production code must not know about tests.**

- Never add `#ifdef UNIT_TEST`, `#ifdef TEST`, or any test-only branch to production source files (`main/`, `components/`). The only permitted exception is `#ifdef UNIT_TEST` inside a module's own `.c`/`.cpp` to redirect a system header (e.g. `<stdio.h>`) to a mock — and only when there is no cleaner alternative.
- Mocks, stubs, and test helpers live exclusively under `tests_host/mocks/`. They must never be included by or compiled into firmware builds.
- A mock is activated through the CMake test target only (via `target_include_directories`, `COMPILE_DEFINITIONS`, or `set_source_files_properties`) — never through a change to the production CMakeLists.txt.
- If production code requires a seam for testing (e.g. dependency injection, factory function), add the seam in production code as a proper API — not as a compile-time test flag.

## Tier 1 — Host Unit Tests (Unity)
Test files: `tests_host/test_*.cpp`

### Conventions
- One logical group of `UnityDefaultTestRun` calls per module under test
- Test name format: `MethodName_StateUnderTest_ExpectedBehavior`
- No test interdependency — each test must pass in isolation
- Mock all ESP-IDF / hardware dependencies with stub headers under `tests_host/mocks/`
- Use `TEST_ASSERT_*` macros from Unity

### Host CMakeLists.txt (`tests_host/CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.16)
project(tests_host LANGUAGES C CXX)
enable_testing()

add_executable(host_tests
    test_my_module.cpp
    unity/unity.c
)

target_compile_features(host_tests PRIVATE cxx_std_20)
target_include_directories(host_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../components/my_component
)

add_test(NAME host-tests.unit COMMAND host_tests)
```

### Commands
```bash
# Via script (recommended)
python run_host_tests.py

# Manual
cmake -B build_tests_host -DCMAKE_BUILD_TYPE=Debug tests_host/
cmake --build build_tests_host
ctest -C Debug --verbose --test-dir build_tests_host
ctest --test-dir build_tests_host -R MyTest --verbose
```

## Tier 2 — On-Target Unity Tests (ESP-IDF)
See [ESP-IDF Unit Testing docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/unit-tests.html).

Each test group is a standalone IDF project under `tests/test_<name>/`.

Test files follow Unity conventions:
```cpp
TEST_CASE("my function returns expected value", "[my_module]")
{
    TEST_ASSERT_EQUAL(42, my_function());
}
```

### Commands
```bash
# Via script (recommended) — builds, flashes, reads serial for each group
python run_device_tests.py                   # all groups
python run_device_tests.py test_comp_crc32   # one group
python run_device_tests.py "test_comp*"      # by mask
```

## Tier 3 — Hardware-in-the-Loop (pytest-embedded)
Test files: `pytest_tests/test_*.py`

### Setup
```bash
pip install pytest pytest-embedded pytest-embedded-serial-esp
```

### Example test
```python
def test_hello_world(dut):
    dut.expect("Hello from ESP32", timeout=10)
```

### Commands
```bash
pytest pytest_tests/ --target esp32 --port COM8 -v
pytest pytest_tests/ -k test_wifi --port COM8
```

## TDD Workflow
1. Write a failing host unit test
2. Write minimal implementation to pass
3. Refactor under green tests
4. Add on-target / pytest test for hardware-specific paths
5. Run full host suite before every commit
