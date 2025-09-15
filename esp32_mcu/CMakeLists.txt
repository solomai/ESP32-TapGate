# Unified entry point for both ESP-IDF firmware build and host-side tests.
cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME esp32-tapgate)
set(PROJECT_VER "1.0.0")

# Determine which build flow should be enabled. When using Visual Studio
# generator we default to the host-only test configuration so that the IDE can
# discover the unit tests without the ESP-IDF toolchain. The mode can be
# overridden explicitly with -DTAPGATE_BUILD_MODE={idf,host}.
if(NOT DEFINED TAPGATE_BUILD_MODE)
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        set(TAPGATE_BUILD_MODE "host")
    else()
        set(TAPGATE_BUILD_MODE "idf")
    endif()
endif()

string(TOLOWER "${TAPGATE_BUILD_MODE}" TAPGATE_BUILD_MODE_LOWER)

if(TAPGATE_BUILD_MODE_LOWER STREQUAL "host")
    project(${PROJECT_NAME}_host_tests LANGUAGES C)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_EXTENSIONS OFF)

    include(CTest)
    if(NOT BUILD_TESTING)
        message(STATUS "Host unit tests are disabled (BUILD_TESTING=OFF)")
    else()
        enable_testing()
        add_subdirectory(tests_host)
    endif()

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

# The following boilerplate must remain in this order for ESP-IDF projects.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${PROJECT_NAME})

# Add your components
set(EXTRA_COMPONENT_DIRS components)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
