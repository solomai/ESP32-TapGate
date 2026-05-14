file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

idf_component_register(
    SRCS ${TEST_SOURCES}
    INCLUDE_DIRS "." "../../../tests/common"
    REQUIRES unity esp_timer
    WHOLE_ARCHIVE
)