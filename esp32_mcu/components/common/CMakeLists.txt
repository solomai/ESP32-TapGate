file(GLOB srcs
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/nvm/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/crc32/*.c"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS . nvm crc32
    REQUIRES
        nvs_flash
)
