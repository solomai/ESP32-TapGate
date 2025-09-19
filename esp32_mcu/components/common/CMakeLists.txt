file(GLOB srcs
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/nvm/*.c"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS . nvm
    REQUIRES
        logs
        nvs_flash
)
