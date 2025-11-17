idf_component_register(
    SRCS "main.c" "device_context.c" ${CONNECTOR_SRCS}
    REQUIRES
        common
        event_journal
        entity_store
        ecies_crypto
        diagnostic
        messages
        esp_common
        driver
        esp_timer
)