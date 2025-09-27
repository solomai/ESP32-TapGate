#include "nvs.h"
#include "nvs_flash.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MAX_CONTEXTS 8
#define MAX_ENTRIES 32
#define MAX_KEY_LENGTH 32
#define MAX_NAMESPACE_LENGTH 32
#define MAX_PARTITION_LENGTH 32
#define MAX_STRING_VALUE 128

typedef struct
{
    bool used;
    char partition[MAX_PARTITION_LENGTH];
    char ns[MAX_NAMESPACE_LENGTH];
} mock_context_t;

typedef struct
{
    bool used;
    char partition[MAX_PARTITION_LENGTH];
    char ns[MAX_NAMESPACE_LENGTH];
    char key[MAX_KEY_LENGTH];
    bool is_string;
    char string_value[MAX_STRING_VALUE];
    uint32_t u32_value;
} mock_entry_t;

static mock_context_t s_contexts[MAX_CONTEXTS];
static mock_entry_t s_entries[MAX_ENTRIES];

static size_t mock_strnlen(const char *str, size_t max_len)
{
    if (!str)
        return 0;
    size_t len = 0;
    while (len < max_len && str[len])
        ++len;
    return len;
}

static mock_context_t *find_context_by_handle(nvs_handle_t handle)
{
    if (handle == 0 || handle > MAX_CONTEXTS)
        return NULL;
    mock_context_t *ctx = &s_contexts[handle - 1];
    return ctx->used ? ctx : NULL;
}

static mock_entry_t *find_entry(const mock_context_t *ctx, const char *key, bool is_string)
{
    if (!ctx || !key)
        return NULL;
    for (size_t i = 0; i < MAX_ENTRIES; ++i)
    {
        mock_entry_t *entry = &s_entries[i];
        if (!entry->used)
            continue;
        if (entry->is_string != is_string)
            continue;
        if (strncmp(entry->partition, ctx->partition, sizeof(entry->partition)) != 0)
            continue;
        if (strncmp(entry->ns, ctx->ns, sizeof(entry->ns)) != 0)
            continue;
        if (strncmp(entry->key, key, sizeof(entry->key)) != 0)
            continue;
        return entry;
    }
    return NULL;
}

static mock_entry_t *allocate_entry(const mock_context_t *ctx, const char *key, bool is_string)
{
    if (!ctx || !key)
        return NULL;
    for (size_t i = 0; i < MAX_ENTRIES; ++i)
    {
        mock_entry_t *entry = &s_entries[i];
        if (entry->used)
            continue;
        entry->used = true;
        entry->is_string = is_string;
        strncpy(entry->partition, ctx->partition, sizeof(entry->partition) - 1);
        entry->partition[sizeof(entry->partition) - 1] = '\0';
        strncpy(entry->ns, ctx->ns, sizeof(entry->ns) - 1);
        entry->ns[sizeof(entry->ns) - 1] = '\0';
        strncpy(entry->key, key, sizeof(entry->key) - 1);
        entry->key[sizeof(entry->key) - 1] = '\0';
        entry->string_value[0] = '\0';
        entry->u32_value = 0;
        return entry;
    }
    return NULL;
}

static void clear_partition_entries(const char *partition)
{
    if (!partition)
        return;
    for (size_t i = 0; i < MAX_ENTRIES; ++i)
    {
        if (!s_entries[i].used)
            continue;
        if (strncmp(s_entries[i].partition, partition, sizeof(s_entries[i].partition)) == 0)
        {
            s_entries[i].used = false;
        }
    }
}

void nvs_mock_reset(void)
{
    memset(s_contexts, 0, sizeof(s_contexts));
    memset(s_entries, 0, sizeof(s_entries));
}

esp_err_t nvs_flash_init_partition(const char *partition_name)
{
    (void)partition_name;
    return ESP_OK;
}

esp_err_t nvs_flash_erase_partition(const char *partition_name)
{
    clear_partition_entries(partition_name);
    return ESP_OK;
}

esp_err_t nvs_open_from_partition(const char *partition_name,
                                  const char *namespace_name,
                                  nvs_open_mode open_mode,
                                  nvs_handle_t *out_handle)
{
    if (!partition_name || !namespace_name || !out_handle)
        return ESP_ERR_INVALID_ARG;

    for (size_t i = 0; i < MAX_CONTEXTS; ++i)
    {
        mock_context_t *ctx = &s_contexts[i];
        if (!ctx->used)
            continue;
        if (strncmp(ctx->partition, partition_name, sizeof(ctx->partition)) != 0)
            continue;
        if (strncmp(ctx->ns, namespace_name, sizeof(ctx->ns)) != 0)
            continue;
        *out_handle = (nvs_handle_t)(i + 1);
        return ESP_OK;
    }

    if (open_mode == NVS_READONLY)
        return ESP_ERR_NVS_NOT_FOUND;

    for (size_t i = 0; i < MAX_CONTEXTS; ++i)
    {
        mock_context_t *ctx = &s_contexts[i];
        if (ctx->used)
            continue;
        ctx->used = true;
        strncpy(ctx->partition, partition_name, sizeof(ctx->partition) - 1);
        ctx->partition[sizeof(ctx->partition) - 1] = '\0';
        strncpy(ctx->ns, namespace_name, sizeof(ctx->ns) - 1);
        ctx->ns[sizeof(ctx->ns) - 1] = '\0';
        *out_handle = (nvs_handle_t)(i + 1);
        return ESP_OK;
    }

    return ESP_ERR_NO_MEM;
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *out_value, size_t *length)
{
    mock_context_t *ctx = find_context_by_handle(handle);
    if (!ctx || !length)
        return ESP_ERR_INVALID_ARG;

    mock_entry_t *entry = find_entry(ctx, key, true);
    if (!entry)
        return ESP_ERR_NVS_NOT_FOUND;

    size_t required = mock_strnlen(entry->string_value, sizeof(entry->string_value)) + 1;
    if (*length < required)
    {
        *length = required;
        return ESP_ERR_NO_MEM;
    }

    if (out_value)
        memcpy(out_value, entry->string_value, required);
    *length = required;
    return ESP_OK;
}

esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *value)
{
    mock_context_t *ctx = find_context_by_handle(handle);
    if (!ctx || !key || !value)
        return ESP_ERR_INVALID_ARG;

    mock_entry_t *entry = find_entry(ctx, key, true);
    if (!entry)
        entry = allocate_entry(ctx, key, true);
    if (!entry)
        return ESP_ERR_NO_MEM;

    strncpy(entry->string_value, value, sizeof(entry->string_value) - 1);
    entry->string_value[sizeof(entry->string_value) - 1] = '\0';
    return ESP_OK;
}

esp_err_t nvs_get_u32(nvs_handle_t handle, const char *key, uint32_t *out_value)
{
    mock_context_t *ctx = find_context_by_handle(handle);
    if (!ctx || !key || !out_value)
        return ESP_ERR_INVALID_ARG;

    mock_entry_t *entry = find_entry(ctx, key, false);
    if (!entry)
        return ESP_ERR_NVS_NOT_FOUND;

    *out_value = entry->u32_value;
    return ESP_OK;
}

esp_err_t nvs_set_u32(nvs_handle_t handle, const char *key, uint32_t value)
{
    mock_context_t *ctx = find_context_by_handle(handle);
    if (!ctx || !key)
        return ESP_ERR_INVALID_ARG;

    mock_entry_t *entry = find_entry(ctx, key, false);
    if (!entry)
        entry = allocate_entry(ctx, key, false);
    if (!entry)
        return ESP_ERR_NO_MEM;

    entry->u32_value = value;
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle)
{
    (void)handle;
}
