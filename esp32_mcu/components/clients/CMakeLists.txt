file(GLOB srcs
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS .
    REQUIRES common
    PRIV_REQUIRES nvs_flash
)
