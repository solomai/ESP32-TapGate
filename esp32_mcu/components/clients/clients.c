#include "clients.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#define CLIENTS_STORAGE_KEY "table"

static const char *TAG = "clients";

typedef struct
{
    uint32_t count;
    client_t records[CLIENTS_DB_MAX_RECORDS];
} clients_storage_t;

typedef struct
{
    bool             initialized;
    nvs_handle_t     handle;
    clients_storage_t storage;
} clients_db_t;

static clients_db_t g_db;

static clients_status_t ensure_initialized(void);
static clients_status_t load_storage(void);
static clients_status_t persist_storage(void);
static int              find_client_index(const client_uid_t client_id);
static bool             client_id_is_empty(const client_uid_t client_id);
static bool             string_has_terminator(const char *str, size_t capacity);
static clients_status_t validate_client(const client_t *client);
static void             copy_client_record(client_t *dst, const client_t *src);
static bool             get_bit(uint8_t flags, uint8_t bit_num);
static uint8_t          set_bit(uint8_t flags, uint8_t bit_num, bool value);
static clients_status_t update_allow_bit(const client_uid_t client_id, uint8_t bit_num, bool value);

clients_status_t open_client_db(void)
{
    if (g_db.initialized)
        return CLIENTS_OK;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        if (nvs_flash_erase() != ESP_OK)
        {
            ESP_LOGE(TAG, "%s", "failed to erase NVS flash");
            return CLIENTS_ERR_STORAGE;
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return CLIENTS_ERR_STORAGE;
    }

    err = nvs_open(CLIENTS_DB_NAMESPACE, NVS_READWRITE, &g_db.handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return CLIENTS_ERR_STORAGE;
    }

    clients_status_t status = load_storage();
    if (status != CLIENTS_OK)
    {
        nvs_close(g_db.handle);
        g_db.handle      = NULL;
        g_db.initialized = false;
        return status;
    }

    g_db.initialized = true;
    return CLIENTS_OK;
}

void close_client_db(void)
{
    if (!g_db.initialized)
        return;

    nvs_close(g_db.handle);
    g_db.handle      = NULL;
    g_db.initialized = false;
    memset(&g_db.storage, 0, sizeof(g_db.storage));
}

clients_status_t clients_add(const client_t *client)
{
    clients_status_t status = ensure_initialized();
    if (status != CLIENTS_OK)
        return status;
    status = validate_client(client);
    if (status != CLIENTS_OK)
        return status;

    if (g_db.storage.count >= CLIENTS_DB_MAX_RECORDS)
        return CLIENTS_ERR_FULL;

    if (find_client_index(client->client_id) >= 0)
        return CLIENTS_ERR_EXISTS;

    client_t  record;
    uint32_t  index = g_db.storage.count;

    copy_client_record(&record, client);
    g_db.storage.records[index] = record;
    g_db.storage.count++;

    status = persist_storage();
    if (status != CLIENTS_OK)
    {
        g_db.storage.count--;
        memset(&g_db.storage.records[index], 0, sizeof(client_t));
    }

    return status;
}

clients_status_t clients_get(const client_uid_t client_id, const client_t **out_client)
{
    if (NULL == out_client)
        return CLIENTS_ERR_INVALID_ARG;

    clients_status_t status = ensure_initialized();
    if (status != CLIENTS_OK)
        return status;

    if (NULL == client_id)
        return CLIENTS_ERR_INVALID_ARG;

    int index = find_client_index(client_id);
    if (index < 0)
        return CLIENTS_ERR_NOT_FOUND;

    *out_client = &g_db.storage.records[index];
    return CLIENTS_OK;
}

clients_status_t clients_get_name(const client_uid_t client_id, char *buffer, size_t buffer_size)
{
    if ((NULL == buffer) || (0U == buffer_size))
        return CLIENTS_ERR_INVALID_ARG;

    const client_t *client = NULL;
    clients_status_t status = clients_get(client_id, &client);
    if (status != CLIENTS_OK)
        return status;

    size_t length = 0;
    while ((length < CLIENTS_NAME_MAX) && (client->name[length] != '\0'))
        ++length;

    if (length >= buffer_size)
        return CLIENTS_ERR_TOO_LONG;

    memcpy(buffer, client->name, length);
    buffer[length] = '\0';
    return CLIENTS_OK;
}

clients_status_t clients_get_pub_pem(const client_uid_t client_id, char *buffer, size_t buffer_size)
{
    if ((NULL == buffer) || (0U == buffer_size))
        return CLIENTS_ERR_INVALID_ARG;

    const client_t *client = NULL;
    clients_status_t status = clients_get(client_id, &client);
    if (status != CLIENTS_OK)
        return status;

    size_t length = 0;
    while ((length < CLIENTS_PUBPEM_CAP) && (client->pub_pem[length] != '\0'))
        ++length;

    if (length >= buffer_size)
        return CLIENTS_ERR_TOO_LONG;

    memcpy(buffer, client->pub_pem, length);
    buffer[length] = '\0';
    return CLIENTS_OK;
}

clients_status_t clients_get_nonce(const client_uid_t client_id, nonce_t *nonce)
{
    if (NULL == nonce)
        return CLIENTS_ERR_INVALID_ARG;

    const client_t *client = NULL;
    clients_status_t status = clients_get(client_id, &client);
    if (status != CLIENTS_OK)
        return status;

    *nonce = client->nonce;
    return CLIENTS_OK;
}

clients_status_t clients_set_nonce(const client_uid_t client_id, nonce_t nonce)
{
    clients_status_t status = ensure_initialized();
    if (status != CLIENTS_OK)
        return status;

    if (NULL == client_id)
        return CLIENTS_ERR_INVALID_ARG;

    int index = find_client_index(client_id);
    if (index < 0)
        return CLIENTS_ERR_NOT_FOUND;

    nonce_t old_value = g_db.storage.records[index].nonce;
    g_db.storage.records[index].nonce = nonce;

    status = persist_storage();
    if (status != CLIENTS_OK)
        g_db.storage.records[index].nonce = old_value;

    return status;
}

clients_status_t clients_get_allow_flags(const client_uid_t client_id, uint8_t *flags)
{
    if (NULL == flags)
        return CLIENTS_ERR_INVALID_ARG;

    const client_t *client = NULL;
    clients_status_t status = clients_get(client_id, &client);
    if (status != CLIENTS_OK)
        return status;

    *flags = client->allow_flags;
    return CLIENTS_OK;
}

clients_status_t clients_set_allow_flags(const client_uid_t client_id, uint8_t flags)
{
    clients_status_t status = ensure_initialized();
    if (status != CLIENTS_OK)
        return status;

    if (NULL == client_id)
        return CLIENTS_ERR_INVALID_ARG;

    int index = find_client_index(client_id);
    if (index < 0)
        return CLIENTS_ERR_NOT_FOUND;

    uint8_t old_flags = g_db.storage.records[index].allow_flags;
    g_db.storage.records[index].allow_flags = flags;

    status = persist_storage();
    if (status != CLIENTS_OK)
        g_db.storage.records[index].allow_flags = old_flags;

    return status;
}

clients_status_t clients_allow_bluetooth_get(const client_uid_t client_id, bool *allowed)
{
    if (NULL == allowed)
        return CLIENTS_ERR_INVALID_ARG;

    const client_t *client = NULL;
    clients_status_t status = clients_get(client_id, &client);
    if (status != CLIENTS_OK)
        return status;

    *allowed = get_bit(client->allow_flags, CLIENTS_ALLOW_FLAG_BLUETOOTH);
    return CLIENTS_OK;
}

clients_status_t clients_allow_bluetooth_set(const client_uid_t client_id, bool allowed)
{
    return update_allow_bit(client_id, CLIENTS_ALLOW_FLAG_BLUETOOTH, allowed);
}

clients_status_t clients_allow_public_mqtt_get(const client_uid_t client_id, bool *allowed)
{
    if (NULL == allowed)
        return CLIENTS_ERR_INVALID_ARG;

    const client_t *client = NULL;
    clients_status_t status = clients_get(client_id, &client);
    if (status != CLIENTS_OK)
        return status;

    *allowed = get_bit(client->allow_flags, CLIENTS_ALLOW_FLAG_PUBLIC_MQTT);
    return CLIENTS_OK;
}

clients_status_t clients_allow_public_mqtt_set(const client_uid_t client_id, bool allowed)
{
    return update_allow_bit(client_id, CLIENTS_ALLOW_FLAG_PUBLIC_MQTT, allowed);
}

clients_status_t clients_get_ids(client_uid_t *out_ids, size_t max_ids, size_t *out_count)
{
    clients_status_t status = ensure_initialized();
    if (status != CLIENTS_OK)
        return status;

    if (NULL == out_count)
        return CLIENTS_ERR_INVALID_ARG;

    *out_count = g_db.storage.count;

    if (0U == g_db.storage.count)
        return CLIENTS_OK;

    if ((NULL == out_ids) || (max_ids < g_db.storage.count))
        return CLIENTS_ERR_TOO_LONG;

    for (uint32_t i = 0; i < g_db.storage.count; ++i)
    {
        memcpy(out_ids[i], g_db.storage.records[i].client_id, sizeof(client_uid_t));
    }

    return CLIENTS_OK;
}

size_t clients_size(void)
{
    if (!g_db.initialized)
        return 0U;

    return (size_t)g_db.storage.count;
}

static clients_status_t ensure_initialized(void)
{
    if (!g_db.initialized)
        return CLIENTS_ERR_NO_INIT;
    return CLIENTS_OK;
}

static clients_status_t load_storage(void)
{
    memset(&g_db.storage, 0, sizeof(g_db.storage));

    size_t required_size = 0;
    esp_err_t err = nvs_get_blob(g_db.handle, CLIENTS_STORAGE_KEY, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        return persist_storage();
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_get_blob size failed: %s", esp_err_to_name(err));
        return CLIENTS_ERR_STORAGE;
    }
    if (required_size < sizeof(uint32_t))
    {
        ESP_LOGE(TAG, "%s", "stored clients blob too small");
        return CLIENTS_ERR_STORAGE;
    }
    if (required_size > sizeof(g_db.storage))
    {
        ESP_LOGE(TAG, "stored clients blob too large (%zu)", required_size);
        return CLIENTS_ERR_STORAGE;
    }

    size_t copy_size = sizeof(g_db.storage);
    err              = nvs_get_blob(g_db.handle, CLIENTS_STORAGE_KEY, &g_db.storage, &copy_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_get_blob data failed: %s", esp_err_to_name(err));
        return CLIENTS_ERR_STORAGE;
    }

    if (g_db.storage.count > CLIENTS_DB_MAX_RECORDS)
        g_db.storage.count = CLIENTS_DB_MAX_RECORDS;

    return CLIENTS_OK;
}

static clients_status_t persist_storage(void)
{
    size_t stored_size = sizeof(g_db.storage.count) + (size_t)g_db.storage.count * sizeof(client_t);
    if (stored_size < sizeof(uint32_t))
        stored_size = sizeof(uint32_t);

    esp_err_t err = nvs_set_blob(g_db.handle, CLIENTS_STORAGE_KEY, &g_db.storage, stored_size);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_set_blob failed: %s", esp_err_to_name(err));
        return CLIENTS_ERR_STORAGE;
    }

    err = nvs_commit(g_db.handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
        return CLIENTS_ERR_STORAGE;
    }

    return CLIENTS_OK;
}

static int find_client_index(const client_uid_t client_id)
{
    if (NULL == client_id)
        return -1;

    for (uint32_t i = 0; i < g_db.storage.count; ++i)
    {
        if (0 == memcmp(g_db.storage.records[i].client_id, client_id, sizeof(client_uid_t)))
            return (int)i;
    }

    return -1;
}

static bool client_id_is_empty(const client_uid_t client_id)
{
    for (size_t i = 0; i < sizeof(client_uid_t); ++i)
    {
        if (client_id[i] != 0U)
            return false;
    }
    return true;
}

static bool string_has_terminator(const char *str, size_t capacity)
{
    for (size_t i = 0; i < capacity; ++i)
    {
        if ('\0' == str[i])
            return true;
    }
    return false;
}

static clients_status_t validate_client(const client_t *client)
{
    if (NULL == client)
        return CLIENTS_ERR_INVALID_ARG;

    if (client_id_is_empty(client->client_id))
        return CLIENTS_ERR_INVALID_ARG;

    if (!string_has_terminator(client->name, CLIENTS_NAME_MAX))
        return CLIENTS_ERR_TOO_LONG;

    if (!string_has_terminator(client->pub_pem, CLIENTS_PUBPEM_CAP))
        return CLIENTS_ERR_TOO_LONG;

    return CLIENTS_OK;
}

static void copy_client_record(client_t *dst, const client_t *src)
{
    memset(dst, 0, sizeof(*dst));
    dst->allow_flags = src->allow_flags;
    dst->nonce       = src->nonce;
    memcpy(dst->client_id, src->client_id, sizeof(client_uid_t));

    size_t name_len = 0;
    while ((name_len < (CLIENTS_NAME_MAX - 1U)) && (src->name[name_len] != '\0'))
        ++name_len;
    if (name_len > 0U)
        memcpy(dst->name, src->name, name_len);
    dst->name[name_len] = '\0';

    size_t pem_len = 0;
    while ((pem_len < (CLIENTS_PUBPEM_CAP - 1U)) && (src->pub_pem[pem_len] != '\0'))
        ++pem_len;
    if (pem_len > 0U)
        memcpy(dst->pub_pem, src->pub_pem, pem_len);
    dst->pub_pem[pem_len] = '\0';
}

static bool get_bit(uint8_t flags, uint8_t bit_num)
{
    if (bit_num >= 8U)
        return false;

    return ((flags >> bit_num) & 0x01U) != 0U;
}

static uint8_t set_bit(uint8_t flags, uint8_t bit_num, bool value)
{
    if (bit_num >= 8U)
        return flags;

    uint8_t mask = (uint8_t)(1U << bit_num);
    if (value)
        return (uint8_t)(flags | mask);
    return (uint8_t)(flags & (uint8_t)~mask);
}

static clients_status_t update_allow_bit(const client_uid_t client_id, uint8_t bit_num, bool value)
{
    clients_status_t status = ensure_initialized();
    if (status != CLIENTS_OK)
        return status;

    if (NULL == client_id)
        return CLIENTS_ERR_INVALID_ARG;

    int index = find_client_index(client_id);
    if (index < 0)
        return CLIENTS_ERR_NOT_FOUND;

    client_t *record   = &g_db.storage.records[index];
    uint8_t  new_flags = set_bit(record->allow_flags, bit_num, value);
    if (new_flags == record->allow_flags)
        return CLIENTS_OK;

    uint8_t old_flags = record->allow_flags;
    record->allow_flags = new_flags;

    status = persist_storage();
    if (status != CLIENTS_OK)
        record->allow_flags = old_flags;

    return status;
}
