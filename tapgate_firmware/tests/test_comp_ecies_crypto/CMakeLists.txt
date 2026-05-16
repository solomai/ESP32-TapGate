cmake_minimum_required(VERSION 3.16)
set(CMAKE_OBJECT_PATH_MAX 250)

set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_SOURCE_DIR}/../common/sdkconfig.defaults")

# Add only the components we need for this test
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../components/ecies_crypto")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_system)