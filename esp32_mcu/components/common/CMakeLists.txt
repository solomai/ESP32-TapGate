# All comments are in English.

# If you have .c/.cpp sources in this folder, list them explicitly:
# set(srcs "${CMAKE_CURRENT_LIST_DIR}/common.c" "${CMAKE_CURRENT_LIST_DIR}/util.c")

# Or automatically collect all .c/.cpp here (simple, but globbing won't re-run cmake on new files automatically):
file(GLOB srcs
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
)

# If you have only headers (no .c/.cpp), keep one tiny dummy source so the component links:
if(NOT srcs)
    set(srcs "${CMAKE_CURRENT_LIST_DIR}/types.c")
endif()

# Headers are in the same folder as sources:
set(include_dirs "${CMAKE_CURRENT_LIST_DIR}")

idf_component_register(
    SRCS         "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
)
