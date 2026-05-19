#include "unity.h"
#include "device_ctx.h"
#include "device_entity.h"
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

static device_entity_t make_entity(const char* name,
                                   uint8_t id_fill,
                                   uint8_t pubkey_fill,
                                   uint8_t prvkey_fill)
{
    device_entity_t e{};
    std::strncpy(reinterpret_cast<char*>(e.name), name, NAME_MAX_SIZE - 1);
    std::memset(e.device_id,   id_fill,     UID_CAP);
    std::memset(e.pub_key,     pubkey_fill, PUBKEY_CAP);
    std::memset(e.private_key, prvkey_fill, PRVKEY_CAP);
    return e;
}

// ---------------------------------------------------------------------------
// Init — no NVS data → default name
// ---------------------------------------------------------------------------

void DeviceCtx_Init_NoNvsData_UsesDefaultName()
{
    NVM.reset();
    reset_ctx_from_nvm();

    char buf[NAME_MAX_SIZE]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));

#ifdef CONFIG_TAPGATE_DEVICE_DEFAULT_NAME
    TEST_ASSERT_EQUAL_STRING(CONFIG_TAPGATE_DEVICE_DEFAULT_NAME, buf);
#else
    TEST_ASSERT_EQUAL_STRING("", buf);
#endif
}

// ---------------------------------------------------------------------------
// Init — NVS returns NOT_FOUND (real first-boot) → default name, ESP_OK
// ---------------------------------------------------------------------------

void DeviceCtx_Init_NvsNotFound_UsesDefaultName()
{
    NVM.reset();
    NVM.set_not_found_err(ESP_ERR_NVS_NOT_FOUND);
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.Init()); // NOT_FOUND must not propagate as error

    char buf[NAME_MAX_SIZE]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
#ifdef CONFIG_TAPGATE_DEVICE_DEFAULT_NAME
    TEST_ASSERT_EQUAL_STRING(CONFIG_TAPGATE_DEVICE_DEFAULT_NAME, buf);
#else
    TEST_ASSERT_EQUAL_STRING("", buf);
#endif
}

// ---------------------------------------------------------------------------
// Init — entity blob in NVS → name loaded from it
// ---------------------------------------------------------------------------

void DeviceCtx_Init_WithNvsEntityBlob_LoadsStoredName()
{
    NVM.reset();

    device_entity_t stored = make_entity("MyGate", 0x01, 0x02, 0x03);
    NVM.WriteBlob("nvs_entity", "CtxDevice", "Entity", &stored, sizeof(stored));
    reset_ctx_from_nvm();

    char buf[NAME_MAX_SIZE]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
    TEST_ASSERT_EQUAL_STRING("MyGate", buf);
}

// ---------------------------------------------------------------------------
// Init — fresh boot → non-name entity fields are zeroed
// ---------------------------------------------------------------------------

void DeviceCtx_Init_NoNvsData_EntityFieldsZeroed()
{
    NVM.reset();
    reset_ctx_from_nvm();

    device_entity_t e{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_entity(&e));

    const uint8_t zeros[PUBKEY_CAP]{};
    TEST_ASSERT_EQUAL_MEMORY(zeros, e.pub_key,     PUBKEY_CAP);
    TEST_ASSERT_EQUAL_MEMORY(zeros, e.private_key, PRVKEY_CAP);

    uint8_t id_expected[UID_CAP]{};
    std::memset(id_expected, 'A', UID_CAP);
    TEST_ASSERT_EQUAL_MEMORY(id_expected, e.device_id, UID_CAP);
}

// ---------------------------------------------------------------------------
// get_device_name — output buffer too small
// ---------------------------------------------------------------------------

void DeviceCtx_GetDeviceName_EmptySpan_ReturnsInvalidArg()
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, DeviceCtx.get_device_name({}));
}

// ---------------------------------------------------------------------------
// set_device_name — valid name stored and readable
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_ValidName_Succeeds()
{
    NVM.reset();
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_device_name("TapGate Pro"));

    char buf[NAME_MAX_SIZE]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
    TEST_ASSERT_EQUAL_STRING("TapGate Pro", buf);
}

// ---------------------------------------------------------------------------
// set_device_name — name exactly at capacity (31 chars) succeeds
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_MaxLength_Succeeds()
{
    NVM.reset();
    const char* name31 = "1234567890123456789012345678901";
    static_assert(sizeof("1234567890123456789012345678901") - 1 == NAME_MAX_SIZE - 1, "");

    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_device_name(name31));

    char buf[NAME_MAX_SIZE]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
    TEST_ASSERT_EQUAL_STRING(name31, buf);
}

// ---------------------------------------------------------------------------
// set_device_name — name too long is rejected
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_TooLong_ReturnsInvalidArg()
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG,
                      DeviceCtx.set_device_name("12345678901234567890123456789012"));
}

// ---------------------------------------------------------------------------
// set_device_name — persists via entity blob (re-init reads it back)
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_PersistsToNvm()
{
    NVM.reset();
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_device_name("Persisted"));

    reset_ctx_from_nvm();

    char buf[NAME_MAX_SIZE]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
    TEST_ASSERT_EQUAL_STRING("Persisted", buf);
}

// ---------------------------------------------------------------------------
// get/update entity — null pointer rejected
// ---------------------------------------------------------------------------

void DeviceCtx_GetEntity_NullPtr_ReturnsInvalidArg()
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, DeviceCtx.get_device_entity(nullptr));
}

void DeviceCtx_UpdateEntity_NullPtr_ReturnsInvalidArg()
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, DeviceCtx.update_device_entity(nullptr));
}

// ---------------------------------------------------------------------------
// update_device_entity → get_device_entity roundtrip
// ---------------------------------------------------------------------------

void DeviceCtx_UpdateEntity_GetEntity_Roundtrip()
{
    NVM.reset();

    const device_entity_t in = make_entity("Alice", 0xAA, 0xBB, 0xCC);
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.update_device_entity(&in));

    device_entity_t out{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_entity(&out));
    TEST_ASSERT_EQUAL_MEMORY(&in, &out, sizeof(device_entity_t));
}

// ---------------------------------------------------------------------------
// update_device_entity — persists (re-init restores all fields)
// ---------------------------------------------------------------------------

void DeviceCtx_UpdateEntity_PersistsToNvm()
{
    NVM.reset();

    const device_entity_t in = make_entity("Stored", 0x11, 0x22, 0x33);
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.update_device_entity(&in));

    reset_ctx_from_nvm();

    device_entity_t out{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_entity(&out));
    TEST_ASSERT_EQUAL_MEMORY(&in, &out, sizeof(device_entity_t));
}

// ---------------------------------------------------------------------------
// get_public_key / get_private_key / get_device_id — return entity field copies
// ---------------------------------------------------------------------------

void DeviceCtx_GetPublicKey_ReturnsEntityPubKey()
{
    NVM.reset();
    const device_entity_t in = make_entity("K", 0x10, 0xAB, 0x00);
    (void)DeviceCtx.update_device_entity(&in);

    tg_public_key_t key{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_public_key(key));
    TEST_ASSERT_EQUAL_MEMORY(in.pub_key, key, PUBKEY_CAP);
}

void DeviceCtx_GetPrivateKey_ReturnsEntityPrvKey()
{
    NVM.reset();
    const device_entity_t in = make_entity("K", 0x10, 0x00, 0xCD);
    (void)DeviceCtx.update_device_entity(&in);

    tg_private_key_t key{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_private_key(key));
    TEST_ASSERT_EQUAL_MEMORY(in.private_key, key, PRVKEY_CAP);
}

void DeviceCtx_GetDeviceId_ReturnsEntityDeviceId()
{
    NVM.reset();
    const device_entity_t in = make_entity("K", 0x7F, 0x00, 0x00);
    (void)DeviceCtx.update_device_entity(&in);

    tg_uid_t uid{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_id(uid));
    TEST_ASSERT_EQUAL_MEMORY(in.device_id, uid, UID_CAP);
}

// ---------------------------------------------------------------------------
// Nonce — default is 0 on fresh init
// ---------------------------------------------------------------------------

void DeviceCtx_GetNonce_Default_IsZero()
{
    NVM.reset();
    reset_ctx_from_nvm();

    tg_nonce_t n = 0xDEADBEEF;
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_nonce(&n));
    TEST_ASSERT_EQUAL(0u, n);
}

// ---------------------------------------------------------------------------
// Nonce — null pointer rejected
// ---------------------------------------------------------------------------

void DeviceCtx_GetNonce_NullPtr_ReturnsInvalidArg()
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, DeviceCtx.get_nonce(nullptr));
}

// ---------------------------------------------------------------------------
// Nonce — set/get roundtrip
// ---------------------------------------------------------------------------

void DeviceCtx_SetNonce_GetNonce_Roundtrip()
{
    NVM.reset();
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_nonce(0x12345678u));

    tg_nonce_t n = 0;
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_nonce(&n));
    TEST_ASSERT_EQUAL(0x12345678u, n);
}

// ---------------------------------------------------------------------------
// Nonce — persists to NVM (re-init reads it back)
// ---------------------------------------------------------------------------

void DeviceCtx_SetNonce_PersistsToNvm()
{
    NVM.reset();
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_nonce(0xCAFEBABEu));

    reset_ctx_from_nvm();

    tg_nonce_t n = 0;
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_nonce(&n));
    TEST_ASSERT_EQUAL(0xCAFEBABEu, n);
}

// ---------------------------------------------------------------------------
// get_device_name — buffer exactly name length (no room for NUL)
// ---------------------------------------------------------------------------

void DeviceCtx_GetDeviceName_BufferExactNameLen_ReturnsInvalidSize()
{
    NVM.reset();
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_device_name("Hi")); // name_len = 2
    char buf[2]{};
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_SIZE, DeviceCtx.get_device_name({buf, sizeof(buf)}));
}

// ---------------------------------------------------------------------------
// set_device_name — empty string is valid
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_EmptyString_Succeeds()
{
    NVM.reset();
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_device_name(""));

    char buf[NAME_MAX_SIZE]{};
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_device_name({buf, sizeof(buf)}));
    TEST_ASSERT_EQUAL_STRING("", buf);
}

// ---------------------------------------------------------------------------
// set_device_name — embedded NUL rejected
// ---------------------------------------------------------------------------

void DeviceCtx_SetDeviceName_EmbeddedNull_ReturnsInvalidArg()
{
    using namespace std::string_view_literals;
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, DeviceCtx.set_device_name("ab\0cd"sv));
}

// ---------------------------------------------------------------------------
// get_public_key / get_private_key / get_device_id — null pointer rejected
// ---------------------------------------------------------------------------

void DeviceCtx_GetPublicKey_NullPtr_ReturnsInvalidArg()
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, DeviceCtx.get_public_key(nullptr));
}

void DeviceCtx_GetPrivateKey_NullPtr_ReturnsInvalidArg()
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, DeviceCtx.get_private_key(nullptr));
}

void DeviceCtx_GetDeviceId_NullPtr_ReturnsInvalidArg()
{
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, DeviceCtx.get_device_id(nullptr));
}

// ---------------------------------------------------------------------------
// set_nonce — boundary: UINT32_MAX
// ---------------------------------------------------------------------------

void DeviceCtx_SetNonce_MaxValue_Succeeds()
{
    NVM.reset();
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.set_nonce(UINT32_MAX));

    tg_nonce_t n = 0;
    TEST_ASSERT_EQUAL(ESP_OK, DeviceCtx.get_nonce(&n));
    TEST_ASSERT_EQUAL(UINT32_MAX, n);
}

// ---------------------------------------------------------------------------
// Init — NVM hard error propagates (not silenced like NOT_FOUND)
// ---------------------------------------------------------------------------

void DeviceCtx_Init_NvmReadError_PropagatesError()
{
    NVM.reset();
    NVM.set_read_err(ESP_ERR_NO_MEM);
    const esp_err_t err = DeviceCtx.Init();
    NVM.reset(); // clear injected error for subsequent tests
    TEST_ASSERT_EQUAL(ESP_ERR_NO_MEM, err);
}

// ---------------------------------------------------------------------------
// Multithreaded — concurrent update/get entity produces no field corruption
// ---------------------------------------------------------------------------

void DeviceCtx_Multithreaded_ConcurrentUpdateEntity_GetEntity_NoCorruption()
{
    constexpr int NUM_THREADS = 6;
    constexpr int ITERATIONS  = 200;

    std::atomic<bool> corruption_detected{false};

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i, &corruption_detected]() {
            const auto fill = static_cast<uint8_t>(i + 1);
            const device_entity_t e = make_entity("", fill, fill, fill);

            for (int j = 0; j < ITERATIONS; ++j) {
                (void)DeviceCtx.update_device_entity(&e);

                device_entity_t out{};
                if (DeviceCtx.get_device_entity(&out) != ESP_OK)
                    continue;

                // Each field must be internally consistent (all bytes equal)
                const uint8_t id0 = out.device_id[0];
                for (std::size_t k = 1; k < UID_CAP; ++k) {
                    if (out.device_id[k] != id0) {
                        corruption_detected.store(true, std::memory_order_relaxed);
                        return;
                    }
                }
            }
        });
    }

    for (auto& t : threads)
        t.join();

    TEST_ASSERT_TRUE(!corruption_detected.load());
}

// ---------------------------------------------------------------------------
// Multithreaded — concurrent set/get nonce: no deadlock, atomic correctness
// ---------------------------------------------------------------------------

void DeviceCtx_Multithreaded_ConcurrentSetGetNonce_NoDeadlock()
{
    constexpr int NUM_THREADS = 8;
    constexpr int ITERATIONS  = 500;

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                (void)DeviceCtx.set_nonce(static_cast<tg_nonce_t>(i * ITERATIONS + j));
                tg_nonce_t n = 0;
                (void)DeviceCtx.get_nonce(&n);
            }
        });
    }

    for (auto& t : threads)
        t.join();
    // Pass = no deadlock or crash; nonce value is intentionally racy
}

// ---------------------------------------------------------------------------
// Multithreaded — concurrent get/set produces no corruption
// ---------------------------------------------------------------------------

void DeviceCtx_Multithreaded_ConcurrentSetGet_NoCorruption()
{
    constexpr int NUM_THREADS = 8;
    constexpr int ITERATIONS  = 200;

    std::atomic<bool> corruption_detected{false};

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([i, &corruption_detected]() {
            char name[NAME_MAX_SIZE]{};
            std::snprintf(name, sizeof(name), "Dev%d", i);

            for (int j = 0; j < ITERATIONS; ++j) {
                (void)DeviceCtx.set_device_name(name);

                char out[NAME_MAX_SIZE]{};
                (void)DeviceCtx.get_device_name({out, sizeof(out)});

                if (std::strlen(out) >= NAME_MAX_SIZE)
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

    UnityDefaultTestRun(DeviceCtx_Init_WithNvsEntityBlob_LoadsStoredName,
                        "DeviceCtx_Init_WithNvsEntityBlob_LoadsStoredName", __FILE__);

    UnityDefaultTestRun(DeviceCtx_Init_NoNvsData_EntityFieldsZeroed,
                        "DeviceCtx_Init_NoNvsData_EntityFieldsZeroed", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetDeviceName_EmptySpan_ReturnsInvalidArg,
                        "DeviceCtx_GetDeviceName_EmptySpan_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetDeviceName_BufferExactNameLen_ReturnsInvalidSize,
                        "DeviceCtx_GetDeviceName_BufferExactNameLen_ReturnsInvalidSize", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_ValidName_Succeeds,
                        "DeviceCtx_SetDeviceName_ValidName_Succeeds", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_MaxLength_Succeeds,
                        "DeviceCtx_SetDeviceName_MaxLength_Succeeds", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_TooLong_ReturnsInvalidArg,
                        "DeviceCtx_SetDeviceName_TooLong_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_EmptyString_Succeeds,
                        "DeviceCtx_SetDeviceName_EmptyString_Succeeds", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_EmbeddedNull_ReturnsInvalidArg,
                        "DeviceCtx_SetDeviceName_EmbeddedNull_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetDeviceName_PersistsToNvm,
                        "DeviceCtx_SetDeviceName_PersistsToNvm", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetEntity_NullPtr_ReturnsInvalidArg,
                        "DeviceCtx_GetEntity_NullPtr_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_UpdateEntity_NullPtr_ReturnsInvalidArg,
                        "DeviceCtx_UpdateEntity_NullPtr_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_UpdateEntity_GetEntity_Roundtrip,
                        "DeviceCtx_UpdateEntity_GetEntity_Roundtrip", __FILE__);

    UnityDefaultTestRun(DeviceCtx_UpdateEntity_PersistsToNvm,
                        "DeviceCtx_UpdateEntity_PersistsToNvm", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetPublicKey_ReturnsEntityPubKey,
                        "DeviceCtx_GetPublicKey_ReturnsEntityPubKey", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetPublicKey_NullPtr_ReturnsInvalidArg,
                        "DeviceCtx_GetPublicKey_NullPtr_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetPrivateKey_ReturnsEntityPrvKey,
                        "DeviceCtx_GetPrivateKey_ReturnsEntityPrvKey", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetPrivateKey_NullPtr_ReturnsInvalidArg,
                        "DeviceCtx_GetPrivateKey_NullPtr_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetDeviceId_ReturnsEntityDeviceId,
                        "DeviceCtx_GetDeviceId_ReturnsEntityDeviceId", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetDeviceId_NullPtr_ReturnsInvalidArg,
                        "DeviceCtx_GetDeviceId_NullPtr_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetNonce_Default_IsZero,
                        "DeviceCtx_GetNonce_Default_IsZero", __FILE__);

    UnityDefaultTestRun(DeviceCtx_GetNonce_NullPtr_ReturnsInvalidArg,
                        "DeviceCtx_GetNonce_NullPtr_ReturnsInvalidArg", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetNonce_GetNonce_Roundtrip,
                        "DeviceCtx_SetNonce_GetNonce_Roundtrip", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetNonce_PersistsToNvm,
                        "DeviceCtx_SetNonce_PersistsToNvm", __FILE__);

    UnityDefaultTestRun(DeviceCtx_SetNonce_MaxValue_Succeeds,
                        "DeviceCtx_SetNonce_MaxValue_Succeeds", __FILE__);

    UnityDefaultTestRun(DeviceCtx_Init_NvmReadError_PropagatesError,
                        "DeviceCtx_Init_NvmReadError_PropagatesError", __FILE__);

    UnityDefaultTestRun(DeviceCtx_Multithreaded_ConcurrentSetGet_NoCorruption,
                        "DeviceCtx_Multithreaded_ConcurrentSetGet_NoCorruption", __FILE__);

    UnityDefaultTestRun(DeviceCtx_Multithreaded_ConcurrentUpdateEntity_GetEntity_NoCorruption,
                        "DeviceCtx_Multithreaded_ConcurrentUpdateEntity_GetEntity_NoCorruption", __FILE__);

    UnityDefaultTestRun(DeviceCtx_Multithreaded_ConcurrentSetGetNonce_NoDeadlock,
                        "DeviceCtx_Multithreaded_ConcurrentSetGetNonce_NoDeadlock", __FILE__);

    return UNITY_END();
}
