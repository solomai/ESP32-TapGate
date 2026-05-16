---
description: ESP-IDF v6 build system conventions
---

# Build Rules — ESP-IDF v6

## idf.py (primary build tool)
- Always use `idf.py` — never invoke `cmake` / `ninja` directly for firmware builds
- The IDF environment must be active: `source $IDF_PATH/export.sh` (Linux/macOS) or `export.bat` (Windows)
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
```bash
idf.py set-target esp32s3          # one-time chip selection
idf.py build                       # incremental firmware build
idf.py fullclean && idf.py build   # full rebuild from scratch
idf.py size                        # firmware size report
idf.py size-components             # per-component size breakdown
idf.py size-files                  # per-object-file size breakdown
```

## Flash & Monitor
```bash
idf.py -p /dev/ttyUSB0 flash              # flash only
idf.py -p /dev/ttyUSB0 monitor            # serial monitor (Ctrl+] to quit)
idf.py -p /dev/ttyUSB0 flash monitor      # flash then monitor
idf.py -p /dev/ttyUSB0 -b 2000000 flash   # faster baud rate
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
```bash
ln -sf build/compile_commands.json compile_commands.json   # Linux/macOS
```

## Host-Side Tests (no hardware required)
```bash
# IDF built-in host-test (preferred for components with IDF dependencies)
idf.py -C test host-test

# Standalone GoogleTest build (preferred for pure C++ logic)
cmake -B test/build -G Ninja -DCMAKE_BUILD_TYPE=Debug test/
cmake --build test/build -j$(nproc)
ctest --test-dir test/build --output-on-failure
```

## Scripts
- `./.scripts/build.sh` — incremental `idf.py build`
- `./.scripts/flash.sh [PORT]` — flash firmware
- `./.scripts/monitor.sh [PORT]` — serial monitor
- `./.scripts/flash_monitor.sh [PORT]` — flash then monitor
- `./.scripts/test_host.sh` — host-side unit tests
- `./.scripts/test_hw.sh [PORT] [TARGET]` — pytest hardware tests
- `./.scripts/clean.sh` — `idf.py fullclean`
