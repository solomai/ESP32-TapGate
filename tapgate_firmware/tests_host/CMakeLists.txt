cmake_minimum_required(VERSION 3.16)

project(tests_host LANGUAGES C CXX)

enable_testing()

add_executable(host_tests
    test_base.cpp
    unity/unity.c
)

target_compile_features(host_tests PRIVATE cxx_std_20)
target_include_directories(host_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/unity)

add_test(
    NAME host-tests.unit
    COMMAND host_tests
)