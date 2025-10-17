idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        common
        event_journal
        entity_store
        ecies_crypto
        diagnostic
        messages
        esp_common
        driver
)