idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        logs
        common
        rsapem
        entitystore        
        diagnostic
        esp_common
)