idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        logs
        common
        entity_store
        ecies_crypto
        wifi_manager
        admin_portal
        diagnostic
        esp_common
        driver
)