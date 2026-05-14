cmake_minimum_required(VERSION 3.16)

set(CMAKE_OBJECT_PATH_MAX 250)
set(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 CACHE INTERNAL "")

# ESP-IDF compile warnings
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wno-error=volatile>")
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-tapgate)