file(GLOB_RECURSE EXTRA_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

idf_component_register(
    SRCS ${EXTRA_SOURCES}
    INCLUDE_DIRS "."
                 "common"
                 "common/event_journal"
                 "common/datetime"
                 "common/uuid"
                 "common/nvm"
                 "ctx_device"
                 "ctx_client"
    PRIV_REQUIRES
        esp_system
        nvs_flash
        app_trace
        esp_app_format
        esp_timer
        bootloader_support
        esp_hw_support
        efuse
        mbedtls
)