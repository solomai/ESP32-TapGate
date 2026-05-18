#include "unity.h"
#include "device_ctx.h"
#include "nvm.h"

#include <atomic>
#include <cstring>
#include <cstdio>
#include <thread>
#include <vector>

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void reset_ctx_from_nvm()
{
    DeviceCtx.Init();
}

// ---------------------------------------------------------------------------
// Init — no NVS data → default name
// ---------------------------------------------------------------------------

void DeviceCtx_Init_NoNvsData_UsesDefaultName()
{
    NVM.reset();
    reset_ctx_from_nvm();

    char buf[DEVICE_NAME_CAPACITY]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));

#ifdef CONFIG_TAPGATE_DEVICE_DEFAULT_NAME
    TEST_ASSERT_EQUAL_STRING(CONFIG_TAPGATE_DEVICE_DEFAULT_NAME, buf);
#else
    TEST_ASSERT_EQUAL_STRING("", buf);
#endif
}

// ---------------------------------------------------------------------------
// Init — NVS returns NOT_FOUND (real first-boot behaviour) → default name, ESP_OK
// ---------------------------------------------------------------------------

void DeviceCtx_Init_NvsNotFound_UsesDefaultName()
{
    // Simulate real first-boot: NVS returns ESP_ERR_NVS_NOT_FOUND (namespace not yet created)
    NVM.reset();
    NVM.set_not_found_err(ESP_ERR_NVS_NOT_FOUND);
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.Init()); // NOT_FOUND must not propagate as error

    char buf[DEVICE_NAME_CAPACITY]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
#ifdef CONFIG_TAPGATE_DEVICE_DEFAULT_NAME
    TEST_ASSERT_EQUAL_STRING(CONFIG_TAPGATE_DEVICE_DEFAULT_NAME, buf);
#else
    TEST_ASSERT_EQUAL_STRING("", buf);
#endif
}

// ---------------------------------------------------------------------------
// Init — NVS has a stored name → that name is loaded
// ---------------------------------------------------------------------------

void DeviceCtx_Init_WithNvsData_LoadsStoredName()
{
    NVM.reset();

    // Pre-populate NVM as if a previous session had saved this name
    NVM.WriteString("nvs_entity", "CtxDevice", "DeviceName", "MyGate");
    reset_ctx_from_nvm();

    char buf[DEVICE_NAME_CAPACITY]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
    TEST_ASSERT_EQUAL_STRING("MyGate", buf);
}

// ---------------------------------------------------------------------------
// get_device_name — output buffer too small (size == 0)
// ---------------------------------------------------------------------------

void DeviceCtx_GetDeviceName_EmptySpan_ReturnsInvalidArg()
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, DeviceCtx.get_device_name({}));
}

// ---------------------------------------------------------------------------
// set_device_name — valid name is stored in memory and NVM
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_ValidName_Succeeds()
{
    NVM.reset();
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_device_name("TapGate Pro"));

    char buf[DEVICE_NAME_CAPACITY]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
    TEST_ASSERT_EQUAL_STRING("TapGate Pro", buf);
}

// ---------------------------------------------------------------------------
// set_device_name — name exactly at capacity (31 chars) succeeds
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_MaxLength_Succeeds()
{
    NVM.reset();
    // 31 chars — fits in 32-byte buffer (including '\0')
    const char* name31 = "1234567890123456789012345678901";
    static_assert(sizeof("1234567890123456789012345678901") - 1 == DEVICE_NAME_CAPACITY - 1, "");

    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_device_name(name31));

    char buf[DEVICE_NAME_CAPACITY]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
    TEST_ASSERT_EQUAL_STRING(name31, buf);
}

// ---------------------------------------------------------------------------
// set_device_name — name too long is rejected
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_TooLong_ReturnsInvalidArg()
{
    // 32 chars — one byte over capacity (no room for '\0')
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      DeviceCtx.set_device_name("12345678901234567890123456789012"));
}

// ---------------------------------------------------------------------------
// set_device_name — persists to NVM (re-init reads it back)
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_PersistsToNvm()
{
    NVM.reset();
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_device_name("Persisted"));

    // Simulate a reboot: re-init from NVM
    reset_ctx_from_nvm();

    char buf[DEVICE_NAME_CAPACITY]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
    TEST_ASSERT_EQUAL_STRING("Persisted", buf);
}

// ---------------------------------------------------------------------------
// Multithreaded — concurrent get/set produces no corruption
// ---------------------------------------------------------------------------

void DeviceCtx_Multithreaded_ConcurrentSetGet_NoCorruption()
{
    constexpr int NUM_THREADS  = 8;
    constexpr int ITERATIONS   = 200;

    // Atomic flag avoids data races on unity's internal state from concurrent threads
    std::atomic<bool> corruption_detected{false};

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i, &corruption_detected]() {
            char name[DEVICE_NAME_CAPACITY]{};
            std::snprintf(name, sizeof(name), "Dev%d", i);

            for (int j = 0; j < ITERATIONS; ++j) {
                (void)DeviceCtx.set_device_name(name);

                char out[DEVICE_NAME_CAPACITY]{};
                (void)DeviceCtx.get_device_name({out, sizeof(out)});

                // Value may have been overwritten by any thread, but must be
                // a properly null-terminated string within capacity.
                if (std::strlen(out) >= DEVICE_NAME_CAPACITY)
                    corruption_detected.store(true, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads)
        t.join();

    TEST_ASSERT_TRUE(!corruption_detected.load());
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main(void)
{
    UNITY_BEGIN();

    UnityDefaultTestRun(DeviceCtx_Init_NoNvsData_UsesDefaultName,
                        "DeviceCtx_Init_NoNvsData_UsesDefaultName", __FILE__);

    UnityDefaultTestRun(DeviceCtx_Init_NvsNotFound_UsesDefaultName,
                        "DeviceCtx_Init_NvsNotFound_UsesDefaultName", __FILE__);

    UnityDefaultTestRun(DeviceCtx_Init_WithNvsData_LoadsStoredName,
                        "DeviceCtx_Init_WithNvsData_LoadsStoredName", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetDeviceName_EmptySpan_ReturnsInvalidArg,
                        "DeviceCtx_GetDeviceName_EmptySpan_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_ValidName_Succeeds,
                        "DeviceCtx_SetDeviceName_ValidName_Succeeds", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_MaxLength_Succeeds,
                        "DeviceCtx_SetDeviceName_MaxLength_Succeeds", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_TooLong_ReturnsInvalidArg,
                        "DeviceCtx_SetDeviceName_TooLong_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_PersistsToNvm,
                        "DeviceCtx_SetDeviceName_PersistsToNvm", __FILE__);

    UnityDefaultTestRun(DeviceCtx_Multithreaded_ConcurrentSetGet_NoCorruption,
                        "DeviceCtx_Multithreaded_ConcurrentSetGet_NoCorruption", __FILE__);

    return UNITY_END();
}
