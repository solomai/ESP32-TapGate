file(GLOB srcs
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS .
    REQUIRES
        admin_portal
        common
        logs
        esp_wifi
        esp_netif
        esp_event
)
