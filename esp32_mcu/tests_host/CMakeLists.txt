# Minimal, self-contained host test project (no IDF toolchain). When included
# from the top-level CMake project the configuration already defines the
# project(), so the logic below needs to behave both as a standalone project and
# as a subdirectory.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(esp32_tapgate_host_tests LANGUAGES C)
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

# Interface library exposing ESP-IDF shims used when compiling production
# sources on the host.
add_library(tapgate_idf_mocks INTERFACE)

target_include_directories(tapgate_idf_mocks
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/mocks/include
)


# Mock implementations
add_library(tapgate_mocks STATIC
    ${CMAKE_CURRENT_LIST_DIR}/mocks/nvs_mock.c
)

target_link_libraries(tapgate_mocks
    PUBLIC
        tapgate_idf_mocks
)

# Static library with production sources that are safe to build on the host.
add_library(tapgate_production STATIC)

target_link_libraries(tapgate_production
    PUBLIC
        tapgate_idf_mocks
        tapgate_mocks
)

target_compile_features(tapgate_production
    PUBLIC
        c_std_11
)

target_include_directories(tapgate_production
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../components
        ${CMAKE_CURRENT_LIST_DIR}/../components/common
        ${CMAKE_CURRENT_LIST_DIR}/../components/logs
        ${CMAKE_CURRENT_LIST_DIR}/../components/common/nvm
)

set(_tapgate_component_sources
    ${CMAKE_CURRENT_LIST_DIR}/../components/logs/logs.c
    ${CMAKE_CURRENT_LIST_DIR}/../components/common/nvm/nvm.c
)

file(GLOB _tapgate_common_sources
    "${CMAKE_CURRENT_LIST_DIR}/../components/common/*.c"
)

list(APPEND _tapgate_component_sources ${_tapgate_common_sources})

# Drop empty strings and zero-length sources to avoid compiler warnings from
# placeholder files that ship with the firmware tree.
set(_tapgate_filtered_sources)
foreach(_tapgate_source IN LISTS _tapgate_component_sources)
    if(NOT _tapgate_source)
        continue()
    endif()
    if(NOT EXISTS "${_tapgate_source}")
        continue()
    endif()
    file(SIZE "${_tapgate_source}" _tapgate_source_size)
    if(_tapgate_source_size EQUAL 0)
        continue()
    endif()
    list(APPEND _tapgate_filtered_sources "${_tapgate_source}")
endforeach()

set(_tapgate_component_sources ${_tapgate_filtered_sources})

if(_tapgate_component_sources)
    target_sources(tapgate_production
        PRIVATE
            ${_tapgate_component_sources}
    )
endif()

if(MSVC)
    target_compile_options(tapgate_production PRIVATE /W4)
else()
    target_compile_options(tapgate_production PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Helper function to register Unity-based executables as CTest targets.
function(tapgate_add_unity_test TEST_NAME)
    if(NOT ARGN)
        message(FATAL_ERROR "No source file provided for test ${TEST_NAME}")
    endif()

    add_executable(${TEST_NAME} ${ARGN})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            unity
            tapgate_production
    )
    if(MSVC)
        target_compile_options(${TEST_NAME} PRIVATE /W4)
    else()
        target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    list(GET ARGN 0 _tapgate_first_source)
    if(_tapgate_first_source)
        get_filename_component(_tapgate_first_source "${_tapgate_first_source}" ABSOLUTE)
    else()
        set(_tapgate_first_source "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    file(RELATIVE_PATH _tapgate_source_label "${CMAKE_SOURCE_DIR}" "${_tapgate_first_source}")

    set_target_properties(${TEST_NAME}
        PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TEST_NAME}>"
    )

    add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}>)
    set_tests_properties(${TEST_NAME}
        PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TEST_NAME}>"
            LABELS "tests_host;${TEST_NAME};${_tapgate_source_label}"
    )
endfunction()

# Individual test binaries
tapgate_add_unity_test(test_types ${CMAKE_CURRENT_LIST_DIR}/test_types.c)
tapgate_add_unity_test(test_logs ${CMAKE_CURRENT_LIST_DIR}/test_logs.c)

