#include "page_common.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char *value_for_key(const admin_template_pair_t *pairs,
                                 size_t pair_count,
                                 const char *key,
                                 size_t key_len)
{
    if (!pairs || !key)
        return "";

    for (size_t i = 0; i < pair_count; ++i)
    {
        if (!pairs[i].key)
            continue;
        size_t candidate_len = strlen(pairs[i].key);
        if (candidate_len != key_len)
            continue;
        if (strncmp(pairs[i].key, key, key_len) == 0)
            return pairs[i].value ? pairs[i].value : "";
    }
    return "";
}

esp_err_t admin_page_send_template(httpd_req_t *req,
                                   const uint8_t *template_start,
                                   const uint8_t *template_end,
                                   const admin_template_pair_t *pairs,
                                   size_t pair_count)
{
    if (!req || !template_start || !template_end)
        return ESP_ERR_INVALID_ARG;

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    size_t template_len = (size_t)(template_end - template_start);
    if (template_len == 0)
        return httpd_resp_send(req, "", HTTPD_RESP_USE_STRLEN);

    const char *template_bytes = (const char *)template_start;

    size_t extra = 0;
    if (pairs)
    {
        for (size_t i = 0; i < pair_count; ++i)
        {
            if (pairs[i].value)
                extra += strlen(pairs[i].value);
        }
    }

    char *buffer = malloc(template_len + extra + 1);
    if (!buffer)
        return ESP_ERR_NO_MEM;

    size_t out = 0;
    size_t pos = 0;
    while (pos < template_len)
    {
        if (template_bytes[pos] == '{' && pos + 1 < template_len && template_bytes[pos + 1] == '{')
        {
            size_t close = pos + 2;
            while (close + 1 < template_len && !(template_bytes[close] == '}' && template_bytes[close + 1] == '}'))
            {
                ++close;
            }

            if (close + 1 >= template_len)
            {
                buffer[out++] = template_bytes[pos++];
                continue;
            }

            size_t key_start = pos + 2;
            size_t key_len = close - key_start;
            while (key_len > 0 && isspace((unsigned char)template_bytes[key_start]))
            {
                ++key_start;
                --key_len;
            }
            while (key_len > 0 && isspace((unsigned char)template_bytes[key_start + key_len - 1]))
            {
                --key_len;
            }

            const char *replacement = value_for_key(pairs, pair_count, template_bytes + key_start, key_len);
            if (replacement)
            {
                size_t rep_len = strlen(replacement);
                memcpy(buffer + out, replacement, rep_len);
                out += rep_len;
            }

            pos = close + 2;
        }
        else
        {
            buffer[out++] = template_bytes[pos++];
        }
    }

    buffer[out] = '\0';
    esp_err_t err = httpd_resp_send(req, buffer, out);
    free(buffer);
    return err;
}

