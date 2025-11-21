# Unified entry point for both ESP-IDF firmware build and host-side tests.
cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME esp32-tapgate)
set(PROJECT_VER "1.0.0")

# Detect which toolchain is driving CMake. ESP-IDF passes ESP_PLATFORM whereas
# a native host configuration (e.g. Visual Studio, Ninja with the local
# compiler) leaves it unset. Allow manual override through
# -DTAPGATE_BUILD_MODE={idf,host} so developers can choose explicitly.
set(_tapgate_default_mode "host")
if(DEFINED ESP_PLATFORM AND ESP_PLATFORM)
    set(_tapgate_default_mode "idf")
endif()

if(NOT DEFINED TAPGATE_BUILD_MODE)
    set(TAPGATE_BUILD_MODE "${_tapgate_default_mode}")
endif()

string(TOLOWER "${TAPGATE_BUILD_MODE}" TAPGATE_BUILD_MODE_LOWER)

if(TAPGATE_BUILD_MODE_LOWER STREQUAL "host")
    project(${PROJECT_NAME}_host_tests LANGUAGES C)

    # Disable specific warnings project-wide (C & C++) - must be after project()
    if(MSVC)
        # MSVC warning flags
        add_compile_options(/wd4101 /wd4189 /wd4996)  # Disable unused variable warnings and deprecation warnings
    else()
        # GCC/Clang warning flags
        add_compile_options(-Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable)
    endif()

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)

    include(CTest)
    if(NOT BUILD_TESTING)
        message(STATUS "Host unit tests are disabled (BUILD_TESTING=OFF)")
        return()
    endif()

    if(NOT DEFINED CTEST_CONFIGURATION_TYPE)
        set(CTEST_CONFIGURATION_TYPE "Debug" CACHE STRING
            "Default configuration for CTest when using multi-config generators")
    endif()

    enable_testing()

    if(CMAKE_CONFIGURATION_TYPES)
        file(CONFIGURE OUTPUT "${CMAKE_BINARY_DIR}/CTestCustom.cmake" CONTENT
"# Auto-generated to help IDE test runners pick the default multi-config profile.\nset(CTEST_CONFIGURATION_TYPE \"${CTEST_CONFIGURATION_TYPE}\")\nset(ENV{CTEST_CONFIGURATION_TYPE} \"${CTEST_CONFIGURATION_TYPE}\")\n"
        @ONLY)
    endif()

    add_subdirectory(tests_host)
    return()
elseif(NOT TAPGATE_BUILD_MODE_LOWER STREQUAL "idf")
    message(FATAL_ERROR "Unknown TAPGATE_BUILD_MODE value: ${TAPGATE_BUILD_MODE}."
                         " Expected 'idf' or 'host'.")
endif()

if(NOT DEFINED ENV{IDF_PATH} OR NOT EXISTS "$ENV{IDF_PATH}/tools/cmake/project.cmake")
    message(FATAL_ERROR
        "ESP-IDF environment is not available. Set TAPGATE_BUILD_MODE=host to "
        "configure only the host unit tests.")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})

# Disable specific warnings project-wide (C & C++) - must be after project()
if(MSVC)
    # MSVC warning flags
    add_compile_options(/wd4101 /wd4189 /wd4996)  # Disable unused variable warnings and deprecation warnings
else()
    # GCC/Clang warning flags
    add_compile_options(-Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
