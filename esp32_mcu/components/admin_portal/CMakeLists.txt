# Collect all C sources in this component
file(GLOB srcs "${CMAKE_CURRENT_LIST_DIR}/*.c")
file(GLOB page_srcs "${CMAKE_CURRENT_LIST_DIR}/pages/*.c")
list(APPEND srcs ${page_srcs})

# Embed static assets that need to be served by the HTTP service
set(EMBED_ASSETS
    assets/logo.png
)

# Register component (single idf_component_register!)
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS . pages
    REQUIRES
        esp_http_server
        esp_wifi
        esp_netif
        wpa_supplicant
        wifi_manager
        common
        logs
    EMBED_FILES
        ${EMBED_ASSETS}
)
