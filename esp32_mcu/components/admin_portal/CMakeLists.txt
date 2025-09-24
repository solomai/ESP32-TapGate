# Collect all C sources in this component
file(GLOB CONFIGURE_DEPENDS srcs "${CMAKE_CURRENT_LIST_DIR}/*.c")
file(GLOB CONFIGURE_DEPENDS page_srcs "${CMAKE_CURRENT_LIST_DIR}/pages/*.c")
list(APPEND srcs ${page_srcs})

# Path to web folder
set(WEB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)

# Collect text assets to gzip and binary assets to embed as-is
file(GLOB WEB_TEXT_FILES
    ${WEB_DIR}/*.html
    ${WEB_DIR}/*.css
    ${WEB_DIR}/*.js
)
file(GLOB WEB_BIN_FILES
    ${WEB_DIR}/*.png
    ${WEB_DIR}/*.jpg
    ${WEB_DIR}/*.jpeg
    ${WEB_DIR}/*.gif
    ${WEB_DIR}/*.ico
    ${WEB_DIR}/*.svg
)

# Use Python (from ESP-IDF env) to gzip files (works on Win/Linux/macOS)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GZ_FILES "")
foreach(src ${WEB_TEXT_FILES})
    get_filename_component(fname ${src} NAME)
    set(dst ${CMAKE_CURRENT_BINARY_DIR}/${fname}.gz)

    add_custom_command(
        OUTPUT ${dst}
        COMMAND ${CMAKE_COMMAND} -E echo "Gzip ${fname}"
        COMMAND ${Python3_EXECUTABLE} -c
                "import gzip, pathlib; data = pathlib.Path(r'${src}').read_bytes(); pathlib.Path(r'${dst}').write_bytes(gzip.compress(data, compresslevel=9))"
        DEPENDS ${src}
        VERBATIM
    )
    

    list(APPEND GZ_FILES ${dst})
endforeach()

# Register component (single idf_component_register!)
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS .
    REQUIRES
        esp_http_server
        esp_wifi
        esp_netif
        esp_timer
        wpa_supplicant
        wifi_manager
        common
        logs
    EMBED_FILES
        ${GZ_FILES}
        ${WEB_BIN_FILES}
)
