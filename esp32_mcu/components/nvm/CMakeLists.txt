idf_component_register(
    SRCS nvm.c
    INCLUDE_DIRS .
    REQUIRES
        log
        nvs_flash
)
