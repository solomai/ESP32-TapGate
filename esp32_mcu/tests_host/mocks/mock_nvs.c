#include "nvs.h"
#include "nvs_flash.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MOCK_NVS_MAX_NAMESPACES      8
#define MOCK_NVS_MAX_KEYS            128
#define MOCK_NVS_MAX_BLOB_SIZE       32768
#define MOCK_NVS_MAX_KEY_LENGTH      15
#define MOCK_NVS_MAX_NAMESPACE_LENGTH 15

typedef struct
{
    bool    used;
    char    key[MOCK_NVS_MAX_KEY_LENGTH + 1];
    size_t  length;
    uint8_t value[MOCK_NVS_MAX_BLOB_SIZE];
} mock_nvs_entry_t;

typedef struct
{
    bool             used;
    char             name[MOCK_NVS_MAX_NAMESPACE_LENGTH + 1];
    mock_nvs_entry_t entries[MOCK_NVS_MAX_KEYS];
} mock_nvs_namespace_t;

static mock_nvs_namespace_t g_namespaces[MOCK_NVS_MAX_NAMESPACES];
static bool                 g_initialized;

static mock_nvs_namespace_t *find_namespace(const char *name, bool create);
static mock_nvs_entry_t     *find_entry(mock_nvs_namespace_t *ns, const char *key, bool create);
static size_t                local_strnlen(const char *str, size_t max_len);

esp_err_t nvs_flash_init(void)
{
    g_initialized = true;
    return ESP_OK;
}

esp_err_t nvs_flash_deinit(void)
{
    g_initialized = false;
    return ESP_OK;
}

esp_err_t nvs_flash_erase(void)
{
    memset(g_namespaces, 0, sizeof(g_namespaces));
    return ESP_OK;
}

esp_err_t nvs_open(const char *name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle)
{
    (void)open_mode;

    if ((NULL == name) || (NULL == out_handle))
        return ESP_ERR_INVALID_ARG;

    size_t len = local_strnlen(name, MOCK_NVS_MAX_NAMESPACE_LENGTH + 1);
    if ((len == 0U) || (len > MOCK_NVS_MAX_NAMESPACE_LENGTH))
        return ESP_ERR_INVALID_ARG;

    if (!g_initialized)
        return ESP_ERR_INVALID_ARG;

    mock_nvs_namespace_t *ns = find_namespace(name, true);
    if (NULL == ns)
        return ESP_ERR_NO_MEM;

    *out_handle = (nvs_handle_t)ns;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle)
{
    (void)handle;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char *key, const void *value, size_t length)
{
    if ((NULL == handle) || (NULL == key))
        return ESP_ERR_INVALID_ARG;

    if ((length > 0U) && (NULL == value))
        return ESP_ERR_INVALID_ARG;

    size_t key_len = local_strnlen(key, MOCK_NVS_MAX_KEY_LENGTH + 1);
    if ((key_len == 0U) || (key_len > MOCK_NVS_MAX_KEY_LENGTH))
        return ESP_ERR_INVALID_ARG;

    if (length > MOCK_NVS_MAX_BLOB_SIZE)
        return ESP_ERR_NVS_INVALID_LENGTH;

    mock_nvs_entry_t *entry = find_entry((mock_nvs_namespace_t *)handle, key, true);
    if (NULL == entry)
        return ESP_ERR_NO_MEM;

    entry->length = length;
    if (length > 0U)
        memcpy(entry->value, value, length);

    return ESP_OK;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char *key, void *out_value, size_t *length)
{
    if ((NULL == handle) || (NULL == key) || (NULL == length))
        return ESP_ERR_INVALID_ARG;

    size_t key_len = local_strnlen(key, MOCK_NVS_MAX_KEY_LENGTH + 1);
    if ((key_len == 0U) || (key_len > MOCK_NVS_MAX_KEY_LENGTH))
        return ESP_ERR_INVALID_ARG;

    mock_nvs_entry_t *entry = find_entry((mock_nvs_namespace_t *)handle, key, false);
    if (NULL == entry)
        return ESP_ERR_NVS_NOT_FOUND;

    if (NULL == out_value)
    {
        *length = entry->length;
        return ESP_OK;
    }

    if (*length < entry->length)
    {
        *length = entry->length;
        return ESP_ERR_NVS_INVALID_LENGTH;
    }

    if (entry->length > 0U)
        memcpy(out_value, entry->value, entry->length);
    *length = entry->length;

    return ESP_OK;
}

esp_err_t nvs_erase_key(nvs_handle_t handle, const char *key)
{
    if ((NULL == handle) || (NULL == key))
        return ESP_ERR_INVALID_ARG;

    mock_nvs_entry_t *entry = find_entry((mock_nvs_namespace_t *)handle, key, false);
    if (NULL == entry)
        return ESP_ERR_NVS_NOT_FOUND;

    memset(entry, 0, sizeof(*entry));
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}

static size_t local_strnlen(const char *str, size_t max_len)
{
    size_t len = 0U;
    if (NULL == str)
        return 0U;

    while ((len < max_len) && (str[len] != '\0'))
        ++len;
    return len;
}

static mock_nvs_namespace_t *find_namespace(const char *name, bool create)
{
    for (size_t i = 0; i < MOCK_NVS_MAX_NAMESPACES; ++i)
    {
        mock_nvs_namespace_t *ns = &g_namespaces[i];
        if (ns->used && (strncmp(ns->name, name, MOCK_NVS_MAX_NAMESPACE_LENGTH) == 0))
            return ns;
    }

    if (!create)
        return NULL;

    for (size_t i = 0; i < MOCK_NVS_MAX_NAMESPACES; ++i)
    {
        mock_nvs_namespace_t *ns = &g_namespaces[i];
        if (!ns->used)
        {
            memset(ns, 0, sizeof(*ns));
            ns->used = true;
            strncpy(ns->name, name, MOCK_NVS_MAX_NAMESPACE_LENGTH);
            ns->name[MOCK_NVS_MAX_NAMESPACE_LENGTH] = '\0';
            return ns;
        }
    }

    return NULL;
}

static mock_nvs_entry_t *find_entry(mock_nvs_namespace_t *ns, const char *key, bool create)
{
    if (NULL == ns)
        return NULL;

    for (size_t i = 0; i < MOCK_NVS_MAX_KEYS; ++i)
    {
        mock_nvs_entry_t *entry = &ns->entries[i];
        if (entry->used && (strncmp(entry->key, key, MOCK_NVS_MAX_KEY_LENGTH) == 0))
            return entry;
    }

    if (!create)
        return NULL;

    for (size_t i = 0; i < MOCK_NVS_MAX_KEYS; ++i)
    {
        mock_nvs_entry_t *entry = &ns->entries[i];
        if (!entry->used)
        {
            memset(entry, 0, sizeof(*entry));
            entry->used = true;
            strncpy(entry->key, key, MOCK_NVS_MAX_KEY_LENGTH);
            entry->key[MOCK_NVS_MAX_KEY_LENGTH] = '\0';
            return entry;
        }
    }

    return NULL;
}
