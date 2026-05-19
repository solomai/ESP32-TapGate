cmake_minimum_required(VERSION 3.16)
set(CMAKE_OBJECT_PATH_MAX 250)

set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_SOURCE_DIR}/../common/sdkconfig.defaults")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_base)
