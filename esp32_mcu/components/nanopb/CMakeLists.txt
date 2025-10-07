# nanopb component

set(nanopb_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/nanopb-0.4.9.1")

idf_component_register(
    SRCS 
        "${nanopb_src_dir}/pb_common.c"
        "${nanopb_src_dir}/pb_decode.c" 
        "${nanopb_src_dir}/pb_encode.c"
    INCLUDE_DIRS 
        "${nanopb_src_dir}"
)