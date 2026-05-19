file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# Production source under test
set(PROD_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../main/common/uuid/uuid.cpp"
)

idf_component_register(
    SRCS ${TEST_SOURCES} ${PROD_SOURCES}
    INCLUDE_DIRS
        "."
        "../../../tests/common"
        "../../../main/common"
        "../../../main/common/uuid"
    REQUIRES unity bootloader_support esp_system mbedtls
    WHOLE_ARCHIVE
)
