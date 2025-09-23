file(GLOB srcs "${CMAKE_CURRENT_LIST_DIR}/*.c")
file(GLOB_RECURSE page_srcs "${CMAKE_CURRENT_LIST_DIR}/pages/*.c")
list(APPEND srcs ${page_srcs})

set(WEB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)

file(GLOB WEB_TEXT_SOURCES
    ${WEB_DIR}/*.html
    ${WEB_DIR}/*.css
    ${WEB_DIR}/*.js
)
file(GLOB WEB_BIN_SOURCES
    ${WEB_DIR}/*.png
    ${WEB_DIR}/*.jpg
    ${WEB_DIR}/*.jpeg
    ${WEB_DIR}/*.gif
    ${WEB_DIR}/*.ico
    ${WEB_DIR}/*.svg
)

set(WEB_TEXT_FILES "")
set(WEB_BIN_FILES "")
foreach(src ${WEB_TEXT_SOURCES})
    file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${src})
    file(TO_CMAKE_PATH "${rel}" rel_norm)
    list(APPEND WEB_TEXT_FILES ${rel_norm})
endforeach()

foreach(src ${WEB_BIN_SOURCES})
    file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${src})
    file(TO_CMAKE_PATH "${rel}" rel_norm)
    list(APPEND WEB_BIN_FILES ${rel_norm})
endforeach()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(GZ_FILES "")
foreach(src ${WEB_TEXT_SOURCES})
    get_filename_component(fname ${src} NAME)
    set(dst ${CMAKE_CURRENT_BINARY_DIR}/${fname}.gz)

    add_custom_command(
        OUTPUT ${dst}
        COMMAND ${CMAKE_COMMAND} -E echo "Gzip ${fname}"
        COMMAND ${Python3_EXECUTABLE} -c "import gzip, pathlib; src=r'''${src}'''; dst=r'''${dst}'''; data=pathlib.Path(src).read_bytes(); pathlib.Path(dst).write_bytes(gzip.compress(data, compresslevel=9))"
        DEPENDS ${src}
        VERBATIM
    )

    list(APPEND GZ_FILES ${dst})
endforeach()

if(GZ_FILES)
    add_custom_target(admin_portal_gzip ALL DEPENDS ${GZ_FILES})
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS . pages
    PRIV_INCLUDE_DIRS pages
    REQUIRES
        esp_http_server
        esp_wifi
        esp_netif
        wpa_supplicant
        wifi_manager
        esp_timer
        common
        logs
    EMBED_TXTFILES ${WEB_TEXT_FILES}
    EMBED_FILES ${GZ_FILES} ${WEB_BIN_FILES}
)

if(TARGET admin_portal_gzip)
    idf_component_get_property(admin_portal_lib admin_portal COMPONENT_LIB)
    add_dependencies(${admin_portal_lib} admin_portal_gzip)
endif()
