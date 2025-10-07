# messages component - contains pre-generated protobuf files
# To regenerate files, run: ../regenerate-protobuf.sh (Linux/macOS) or ../regenerate-protobuf.bat (Windows)

# Messages component - nanopb protobuf files with auto-generation

# Set paths - use absolute paths to avoid Windows build issues
get_filename_component(MESSAGES_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
set(PROTO_ROOT "${MESSAGES_DIR}/../../proto")
set(PROTO_FILE "${PROTO_ROOT}/messages.proto")
set(PROTO_OPTIONS "${PROTO_ROOT}/messages.options")
set(NANOPB_DIR "${MESSAGES_DIR}/../components/nanopb")
set(PROTOC_NANOPB "${NANOPB_DIR}/generator/protoc-gen-nanopb")

# Generated files - ALWAYS in the messages directory, not build directory
set(PB_H "${MESSAGES_DIR}/messages.pb.h")
set(PB_C "${MESSAGES_DIR}/messages.pb.c")

message(STATUS "Messages component paths:")
message(STATUS "  MESSAGES_DIR: ${MESSAGES_DIR}")
message(STATUS "  PROTO_ROOT: ${PROTO_ROOT}")
message(STATUS "  PB_H: ${PB_H}")
message(STATUS "  PB_C: ${PB_C}")

# Check if we need to regenerate files
set(NEED_REGEN FALSE)
if(NOT EXISTS "${PB_H}" OR NOT EXISTS "${PB_C}")
    set(NEED_REGEN TRUE)
    message(STATUS "Protobuf files missing, will regenerate")
else()
    # Check if proto files are newer than generated files
    file(TIMESTAMP "${PROTO_FILE}" PROTO_TIME)
    file(TIMESTAMP "${PB_H}" PB_TIME)
    if("${PROTO_TIME}" IS_NEWER_THAN "${PB_TIME}")
        set(NEED_REGEN TRUE)
        message(STATUS "Proto files newer than generated files, will regenerate")
    endif()
endif()

# Find protoc - ESP-IDF includes protoc in its toolchain
find_program(PROTOC_EXECUTABLE protoc PATHS 
    "$ENV{IDF_PATH}/components/protobuf-c/protoc-c"
    "$ENV{IDF_PATH}/tools"
    "$ENV{HOME}/.espressif/tools"
    "C:/Espressif/tools"
    "/opt/esp/tools"
)

# Also check for system protoc
if(NOT PROTOC_EXECUTABLE)
    find_program(PROTOC_EXECUTABLE protoc)
endif()

# If still not found, try to generate files anyway using manual approach
if(NOT PROTOC_EXECUTABLE)
    message(STATUS "protoc not found in standard locations, will try manual script approach")
    set(USE_SCRIPT TRUE)
else()
    message(STATUS "Found protoc: ${PROTOC_EXECUTABLE}")
    set(USE_SCRIPT FALSE)
endif()

# Generate files if needed
if(NEED_REGEN)
    message(STATUS "Generating nanopb files...")
    
    # Make sure nanopb plugin is executable
    if(WIN32)
        set(PLUGIN_CMD "python" "${PROTOC_NANOPB}")
    else()
        execute_process(COMMAND chmod +x "${PROTOC_NANOPB}")
        set(PLUGIN_CMD "${PROTOC_NANOPB}")
    endif()
    
    set(GENERATION_SUCCESS FALSE)
    
    # Try protoc approach if available
    if(NOT USE_SCRIPT AND PROTOC_EXECUTABLE)
        message(STATUS "Trying protoc generation...")
        execute_process(
            COMMAND ${PROTOC_EXECUTABLE}
                --plugin=protoc-gen-nanopb=${PLUGIN_CMD}
                --nanopb_out=${MESSAGES_DIR}
                --proto_path=${PROTO_ROOT}
                ${PROTO_FILE}
            WORKING_DIRECTORY ${MESSAGES_DIR}
            RESULT_VARIABLE PROTOC_RESULT
            OUTPUT_VARIABLE PROTOC_OUTPUT
            ERROR_VARIABLE PROTOC_ERROR
        )
        
        if(PROTOC_RESULT EQUAL 0)
            set(GENERATION_SUCCESS TRUE)
            message(STATUS "✓ Protobuf files generated successfully with protoc")
        else()
            message(STATUS "protoc failed: ${PROTOC_ERROR}")
        endif()
    endif()
    
    # Fall back to manual script if protoc failed or not available
    if(NOT GENERATION_SUCCESS)
        message(STATUS "Trying manual script approach...")
        message(STATUS "Script path: ${MESSAGES_DIR}/../regenerate-protobuf.bat")
        message(STATUS "Working directory: ${MESSAGES_DIR}/..")
        
        if(WIN32)
            execute_process(
                COMMAND cmd /c "${MESSAGES_DIR}/../regenerate-protobuf.bat" auto
                WORKING_DIRECTORY "${MESSAGES_DIR}/.."
                RESULT_VARIABLE SCRIPT_RESULT
                OUTPUT_VARIABLE SCRIPT_OUTPUT
                ERROR_VARIABLE SCRIPT_ERROR
            )
        else()
            execute_process(
                COMMAND bash "${MESSAGES_DIR}/../regenerate-protobuf.sh"
                WORKING_DIRECTORY "${MESSAGES_DIR}/.."
                RESULT_VARIABLE SCRIPT_RESULT
                OUTPUT_VARIABLE SCRIPT_OUTPUT
                ERROR_VARIABLE SCRIPT_ERROR
            )
        endif()
        
        message(STATUS "Script result: ${SCRIPT_RESULT}")
        message(STATUS "Script output: ${SCRIPT_OUTPUT}")
        if(SCRIPT_ERROR)
            message(STATUS "Script error: ${SCRIPT_ERROR}")
        endif()
        
        # Check if files were actually created after script
        if(EXISTS "${PB_H}" AND EXISTS "${PB_C}")
            set(GENERATION_SUCCESS TRUE)
            message(STATUS "✓ Files found after script execution")
        else()
            message(STATUS "✗ Files still missing after script:")
            message(STATUS "  Looking for: ${PB_H}")
            message(STATUS "  Looking for: ${PB_C}")
            # List what's actually in the messages directory
            file(GLOB MESSAGES_FILES "${MESSAGES_DIR}/*")
            message(STATUS "  Files in messages dir: ${MESSAGES_FILES}")
        endif()
        
        if(SCRIPT_RESULT EQUAL 0)
            if(GENERATION_SUCCESS)
                message(STATUS "✓ Protobuf files generated successfully with script")
            else()
                message(STATUS "✗ Script reported success but files not found")
            endif()
        else()
            message(STATUS "Script failed: ${SCRIPT_ERROR}")
        endif()
    endif()
    
    # Final check
    if(NOT GENERATION_SUCCESS)
        message(STATUS "All generation methods failed. Output: ${SCRIPT_OUTPUT}")
        message(STATUS "Error: ${SCRIPT_ERROR}")
        message(FATAL_ERROR "Could not generate protobuf files. Please run 'regenerate-protobuf.bat' manually from esp32_mcu directory")
    endif()
endif()

# Verify files exist before registering component
if(NOT EXISTS "${PB_H}" OR NOT EXISTS "${PB_C}")
    message(FATAL_ERROR "Generated protobuf files not found: ${PB_H}, ${PB_C}")
endif()

idf_component_register(
    SRCS "messages.pb.c"
    INCLUDE_DIRS "."
    REQUIRES nanopb
)