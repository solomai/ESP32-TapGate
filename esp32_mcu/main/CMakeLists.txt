idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        logs
        common
        entitystore
        ecies_crypto
        diagnostic
        esp_common
)