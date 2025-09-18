idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        logs
        common
        rsapem
        clients
        esp_common
        diagnostic
)