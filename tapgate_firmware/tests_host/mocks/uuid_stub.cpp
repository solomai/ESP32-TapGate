#include "uuid.h"

#include <cstring>

// Stub: fills out_uid with zeros. Allows device_ctx host tests to exercise
// the NVS-not-found path without hardware TRNG or PSA crypto.
esp_err_t generate_rng_uid(tg_uid_t out_uid)
{
    if (out_uid == nullptr)
        return ESP_ERR_INVALID_ARG;
    std::memset(out_uid, 'A', UID_CAP);
    return ESP_OK;
}
