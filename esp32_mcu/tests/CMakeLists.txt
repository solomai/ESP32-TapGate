cmake_minimum_required(VERSION 3.16)
project(native_unit_tests C)

enable_testing()

file(GLOB TEST_SOURCES "test_*.c")

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_include_directories(${test_name} PRIVATE 
        ${PROJECT_ROOT}/components
        ${PROJECT_ROOT}/components/clients 
        ${PROJECT_ROOT}/components/common 
        ${CMAKE_CURRENT_LIST_DIR})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
