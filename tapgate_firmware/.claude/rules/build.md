---
description: ESP-IDF v6 build system conventions
---

# Build Rules — ESP-IDF v6

## idf.py (primary build tool)
- Always use `idf.py` — never invoke `cmake` / `ninja` directly for firmware builds
- The IDF environment must be active: `source $IDF_PATH/export.sh` (Linux/macOS), `. $env:IDF_PATH\export.ps1` (PowerShell), or `export.bat` (cmd.exe only) — on this project all `.scripts/*.ps1` wrappers source `.scripts/_idf_env.ps1` automatically
- Set the target chip once: `idf.py set-target <chip>` — persists to `sdkconfig`
- Use `-p <PORT>` for flash/monitor, or set the `ESPPORT` env var to avoid repetition
- Supported chips: `esp32`, `esp32s2`, `esp32s3`, `esp32c3`, `esp32c6`, `esp32h2`

## CMakeLists.txt — Top Level
```cmake
cmake_minimum_required(VERSION 3.28)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(my_project)
```

## CMakeLists.txt — Component
```cmake
idf_component_register(
    SRCS "src/my_module.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_log nvs_flash
    PRIV_REQUIRES esp_wifi
)
```
- Use `REQUIRES` for public deps (propagated to users), `PRIV_REQUIRES` for private deps
- Never use bare `include_directories()` or `link_libraries()`
- List every `.cpp` source explicitly in `SRCS`

## Component Layout
```
components/my_component/
  CMakeLists.txt
  include/my_component/my_module.hpp   # namespaced header path
  src/my_module.cpp
  Kconfig                              # optional component Kconfig menu
```

## sdkconfig
- Commit `sdkconfig.defaults` (baseline values), never commit `sdkconfig` itself
- Add project-level Kconfig in `main/Kconfig.projbuild`
- Add component Kconfig in `components/<name>/Kconfig`

## Build Commands
On Windows always use the `.scripts/idf_build.ps1` wrapper (sets up the IDF environment automatically).
Direct `idf.py` invocations require the IDF environment to be active in the current shell first.
```powershell
powershell -File .scripts/idf_build.ps1                       # incremental build
powershell -File .scripts/idf_build.ps1 -Cmd "set-target esp32s3"  # one-time chip selection
powershell -File .scripts/idf_build.ps1 -Cmd "fullclean"      # clean build artifacts
powershell -File .scripts/idf_build.ps1 -Cmd "size"           # firmware size report
powershell -File .scripts/idf_build.ps1 -Cmd "size-components" # per-component size
powershell -File .scripts/idf_build.ps1 -Cmd "size-files"     # per-object-file size
```

## Flash & Monitor
```powershell
powershell -File .scripts/idf_build.ps1 -Cmd "-p COM8 flash"          # flash only
powershell -File .scripts/idf_build.ps1 -Cmd "-p COM8 monitor"        # serial monitor (Ctrl+] to quit)
powershell -File .scripts/idf_build.ps1 -Cmd "-p COM8 flash monitor"  # flash then monitor
powershell -File .scripts/idf_build.ps1 -Cmd "-p COM8 -b 2000000 flash" # faster baud rate
```

## Partition Table
For custom layouts, create `partitions.csv` and add to `sdkconfig.defaults`:
```
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
```

## compile_commands.json
Generated at `build/compile_commands.json` after `idf.py build`.
Symlink to project root for IDE / clang-tidy:
```powershell
# Requires Developer Mode or an elevated prompt
New-Item -ItemType SymbolicLink -Path compile_commands.json -Target build\compile_commands.json
```

## Host-Side Tests (no hardware required)
```powershell
# Preferred: use the project script wrapper
powershell -File .scripts/run_host_tests.ps1

# Manual CMake build
cmake -B build_tests_host -DCMAKE_BUILD_TYPE=Debug tests_host/
cmake --build build_tests_host
ctest --test-dir build_tests_host --output-on-failure

# With ASan + UBSan (requires Clang or GCC — run under WSL or MinGW on Windows; MSVC does not support -fsanitize)
cmake -B build_tests_host -DCMAKE_BUILD_TYPE=Debug `
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" tests_host/
cmake --build build_tests_host
ctest --test-dir build_tests_host --output-on-failure
```

## Scripts (Windows PowerShell)
```powershell
# Build (default)
powershell -File .scripts/idf_build.ps1

# Full clean rebuild
powershell -File .scripts/idf_build.ps1 -Cmd "fullclean"
powershell -File .scripts/idf_build.ps1

# Flash
powershell -File .scripts/idf_build.ps1 -Cmd "-p COM8 flash"

# Flash + monitor
powershell -File .scripts/idf_build.ps1 -Cmd "-p COM8 flash monitor"

# Serial monitor only
powershell -File .scripts/idf_build.ps1 -Cmd "-p COM8 monitor"

# Firmware size report
powershell -File .scripts/idf_build.ps1 -Cmd "size"
powershell -File .scripts/idf_build.ps1 -Cmd "size-components"

# On-target Unity tests (board must be connected)
powershell -File .scripts/run_device_tests.ps1
powershell -File .scripts/run_device_tests.ps1 -Filter "test_diag*"

# Host unit tests (no hardware)
powershell -File .scripts/run_host_tests.ps1
```

| Script | Purpose |
|---|---|
| `.scripts/idf_build.ps1` | Build / flash / monitor / size — wraps `idf.py` with correct env |
| `.scripts/run_host_tests.ps1` | Host Unity tests (no hardware) — sets up IDF env and delegates to `run_host_tests.py` |
| `.scripts/run_device_tests.ps1` | On-target Unity tests; auto-detects COM port from `.vscode/settings.json` |
| `.scripts/_idf_env.ps1` | ESP-IDF v6 environment setup (dot-sourced by the scripts above) |

## Artifacts
- place all generated artifacts (like `build_output.txt`) in `build/` — never commit generated files
