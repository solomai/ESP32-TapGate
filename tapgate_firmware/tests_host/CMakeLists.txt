cmake_minimum_required(VERSION 3.16)

project(tests_host LANGUAGES C CXX)

enable_testing()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Shared include paths
# ---------------------------------------------------------------------------

set(MOCK_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/common/nvm  # overrides production nvm.h
)

set(PROD_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common        # types.h, device_err.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common/nvm    # nvm_partition.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common/uuid   # uuid.h
)

# ---------------------------------------------------------------------------
# host_tests_device_ctx — DeviceContext unit tests
# ---------------------------------------------------------------------------

add_executable(host_tests_device_ctx
    test_device_ctx.cpp
    mocks/common/nvm/nvm_mock.cpp
    mocks/uuid_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/ctx_device/device_ctx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common/uuid/uuid_str.cpp
    unity/unity.c
)

target_compile_features(host_tests_device_ctx PRIVATE cxx_std_23)

# Mock paths must come before production paths so mock headers take priority
target_include_directories(host_tests_device_ctx PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
    ${MOCK_INCLUDES}
    ${PROD_INCLUDES}
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/ctx_device
)

target_compile_definitions(host_tests_device_ctx PRIVATE
    CONFIG_TAPGATE_DEVICE_DEFAULT_NAME="TapGate v1"
    TAPGATE_TEST_SILENT_LOG
)

target_link_libraries(host_tests_device_ctx PRIVATE Threads::Threads)

add_test(NAME host-tests.device_ctx COMMAND host_tests_device_ctx)

# ---------------------------------------------------------------------------
# host_tests_event_journal — EventJournal macro unit tests
# ---------------------------------------------------------------------------

add_executable(host_tests_event_journal
    test_event_journal.cpp
    unity/unity.c
)

target_compile_features(host_tests_event_journal PRIVATE cxx_std_23)

target_include_directories(host_tests_event_journal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common/event_journal
)

add_test(NAME host-tests.event_journal COMMAND host_tests_event_journal)

# ---------------------------------------------------------------------------
# host_tests_event_journal_debug — EventJournal debug-mode (counter) tests
# ---------------------------------------------------------------------------

add_executable(host_tests_event_journal_debug
    test_event_journal_debug.cpp
    unity/unity.c
)

target_compile_features(host_tests_event_journal_debug PRIVATE cxx_std_23)

target_include_directories(host_tests_event_journal_debug PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common/event_journal
)

add_test(NAME host-tests.event_journal_debug COMMAND host_tests_event_journal_debug)

# ---------------------------------------------------------------------------
# host_tests_uuid — uid_to_str / str_to_uid unit tests (pure, no hardware)
# ---------------------------------------------------------------------------

add_executable(host_tests_uuid
    test_uuid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common/uuid/uuid_str.cpp
    unity/unity.c
)

target_compile_features(host_tests_uuid PRIVATE cxx_std_23)

target_include_directories(host_tests_uuid PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common
    ${CMAKE_CURRENT_SOURCE_DIR}/../main/common/uuid
)

add_test(NAME host-tests.uuid COMMAND host_tests_uuid)
