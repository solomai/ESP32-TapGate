# Minimal, self-contained host test project (no IDF toolchain). When included
# from the top-level CMake project the configuration already defines the
# project(), so the logic below needs to behave both as a standalone project and
# as a subdirectory.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(esp32_tapgate_host_tests C)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(CTest)
if(NOT BUILD_TESTING)
    message(STATUS "Host unit tests are disabled (BUILD_TESTING=OFF)")
    return()
endif()

enable_testing()

# Unity test framework (minimal implementation for host tests)
add_library(unity STATIC
    ${CMAKE_CURRENT_LIST_DIR}/unity/unity.c
)

target_include_directories(unity
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/unity
)

# Interface library that exposes production headers and IDF mocks to the tests.
add_library(tapgate_headers INTERFACE)

target_include_directories(tapgate_headers
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../components
        ${CMAKE_CURRENT_LIST_DIR}/../components/clients
        ${CMAKE_CURRENT_LIST_DIR}/../components/common
        ${CMAKE_CURRENT_LIST_DIR}/mocks/include
)

# Static library with the production sources that are safe to build on the host.
add_library(tapgate_components STATIC
    ${CMAKE_CURRENT_LIST_DIR}/../components/clients/clients.c
)

# Link headers (with mocks) to the production library for tests.
target_link_libraries(tapgate_components
    PUBLIC
        tapgate_headers
)

# Helper function to register Unity-based executables as CTest targets.
function(add_unity_test TEST_NAME)
    if(NOT ARGN)
        message(FATAL_ERROR "No source file provided for test ${TEST_NAME}")
    endif()
    add_executable(${TEST_NAME} ${ARGN})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            unity
            tapgate_components
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Individual test binaries
add_unity_test(test_clients ${CMAKE_CURRENT_LIST_DIR}/test_clients.c)
add_unity_test(test_types ${CMAKE_CURRENT_LIST_DIR}/test_types.c)
