#include "uuid.h"

#include <cstring>

#include "esp_log.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "esp_chip_info.h"
#include "bootloader_random.h"
#include "psa/crypto.h"

static const char *TAG = "uuid";

static constexpr size_t RNG_BYTES = 32;
static constexpr size_t MAC_BYTES = 6;
static constexpr size_t INPUT_LEN = RNG_BYTES + MAC_BYTES + sizeof(uint32_t);

// RFC 4122 §4.4 UUIDv4 bit-field constants
static constexpr uint8_t UUID_VERSION_MASK    = 0x0Fu;
static constexpr uint8_t UUID_VERSION_4       = 0x40u;
static constexpr uint8_t UUID_VARIANT_MASK    = 0x3Fu;
static constexpr uint8_t UUID_VARIANT_RFC4122 = 0x80u;

// UID_CAP must fit within SHA-256 output (32 bytes)
static_assert(UID_CAP <= 32, "UID_CAP exceeds SHA-256 output size");
static_assert(PSA_HASH_LENGTH(PSA_ALG_SHA_256) == 32, "unexpected SHA-256 output length");

// Volatile scrub prevents dead-store elimination of sensitive buffers by the optimiser.
static void secure_zero(void *buf, size_t len) noexcept
{
    volatile auto *p = static_cast<volatile uint8_t *>(buf);
    for (size_t i = 0; i < len; ++i) { p[i] = 0; }
}

esp_err_t generate_rng_uid(tg_uid_t out_uid)
{
    if (out_uid == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    // Enable RF oscillator to guarantee cryptographic-quality TRNG entropy.
    // Without an active RF subsystem the ESP32 TRNG degrades to a pseudo-random
    // source; bootloader_random_enable() activates the RF clock without a full
    // Wi-Fi / BT stack initialisation.
    bootloader_random_enable();

    uint8_t rng_buf[RNG_BYTES];
    esp_fill_random(rng_buf, RNG_BYTES);

    bootloader_random_disable();

    // Hardware salt: eFuse base MAC (hard error — no MAC means no unique salt)
    uint8_t mac[MAC_BYTES] = {};
    esp_err_t err = esp_efuse_mac_get_default(mac);
    if (err != ESP_OK) {
        secure_zero(rng_buf, sizeof(rng_buf));
        ESP_LOGE(TAG, "esp_efuse_mac_get_default failed: %s", esp_err_to_name(err));
        return err;
    }

    // Silicon revision is salt only, not a primary entropy source.
    // esp_chip_info() is void and reads directly from chip registers — cannot fail.
    esp_chip_info_t chip_info{};
    esp_chip_info(&chip_info);
    uint32_t revision = chip_info.revision;

    // Input buffer layout: [ rng(32) | mac(6) | revision(4) ]
    uint8_t input[INPUT_LEN];
    memcpy(input,                          rng_buf,  RNG_BYTES);
    memcpy(input + RNG_BYTES,              mac,       MAC_BYTES);
    memcpy(input + RNG_BYTES + MAC_BYTES, &revision, sizeof(revision));

    secure_zero(rng_buf, sizeof(rng_buf));

    // SHA-256 via PSA Crypto API (mbedTLS 4.x / ESP-IDF v6)
    uint8_t hash[32];
    size_t  hash_len = 0;

    psa_status_t status = psa_hash_compute(
        PSA_ALG_SHA_256,
        input, INPUT_LEN,
        hash, sizeof(hash),
        &hash_len
    );

    secure_zero(input, sizeof(input));

    if (status != PSA_SUCCESS) {
        secure_zero(hash, sizeof(hash));
        ESP_LOGE(TAG, "psa_hash_compute failed: %d", static_cast<int>(status));
        return ESP_FAIL;
    }

    // Copy first UID_CAP bytes of the hash into the output buffer
    memcpy(out_uid, hash, UID_CAP);
    secure_zero(hash, sizeof(hash));

    out_uid[6] = (out_uid[6] & UUID_VERSION_MASK) | UUID_VERSION_4;
    out_uid[8] = (out_uid[8] & UUID_VARIANT_MASK) | UUID_VARIANT_RFC4122;

    return ESP_OK;
}
