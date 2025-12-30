# Automatically find all .cpp and .c files in main directory and subdirectories
file(GLOB_RECURSE COMPONENT_SRCS "*.cpp" "*.c")

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS "."
)

# Set C++ standard to C++20
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)

# Compiler opptions
target_compile_options(${COMPONENT_LIB} PRIVATE 
    -fno-asynchronous-unwind-tables 
    -fno-exceptions
)