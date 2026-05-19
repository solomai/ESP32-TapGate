#include "uuid.h"

#include <cstdio>

static int hex_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

const char *uid_to_str(const tg_uid_t uid, std::span<char> out)
{
    if (uid == nullptr || out.size() < UID_STR_LEN)
        return nullptr;

    for (std::size_t i = 0; i < UID_CAP; ++i)
        std::snprintf(out.data() + i * 2, 3, "%02x", uid[i]);

    return out.data();
}

esp_err_t str_to_uid(std::string_view str, tg_uid_t out_uid)
{
    if (out_uid == nullptr || str.size() != UID_STR_LEN - 1u)
        return ESP_ERR_INVALID_ARG;

    for (std::size_t i = 0; i < UID_CAP; ++i) {
        const int hi = hex_val(str[i * 2]);
        const int lo = hex_val(str[i * 2 + 1]);
        if (hi < 0 || lo < 0)
            return ESP_ERR_INVALID_ARG;
        out_uid[i] = static_cast<uint8_t>((hi << 4) | lo);
    }

    return ESP_OK;
}
