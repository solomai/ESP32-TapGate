#include "unity.h"
#include "clients/clients.h"
#include "nvs_flash.h"

#include <stdbool.h>
#include <string.h>

static client_t make_client(const char *id, const char *name, const char *pem, nonce_t nonce, uint8_t flags);
static void     copy_string(char *dest, size_t capacity, const char *src);
static void     expect_client_id_equal(const client_uid_t expected, const client_uid_t actual);

void setUp(void)
{
    close_client_db();
    nvs_flash_deinit();
    nvs_flash_erase();
}

void tearDown(void)
{
    close_client_db();
    nvs_flash_deinit();
    nvs_flash_erase();
}

void test_clients_namespace_length(void)
{
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(15, strlen(CLIENTS_DB_NAMESPACE));
}

void test_clients_requires_init(void)
{
    client_t sample = make_client("alpha", "Alpha", "PEM", 1U, 0U);
    TEST_ASSERT_EQUAL_INT(CLIENTS_ERR_NO_INIT, clients_add(&sample));
}

void test_clients_open_close(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());
    close_client_db();
    close_client_db();
}

void test_clients_add_and_get(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    client_t sample = make_client("client1", "Client One", "PEM1", 42U, 3U);
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&sample));

    const client_t *retrieved = NULL;
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_get(sample.client_id, &retrieved));
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_UINT32(sample.allow_flags, retrieved->allow_flags);
    TEST_ASSERT_EQUAL_UINT32(sample.nonce, retrieved->nonce);
    TEST_ASSERT_EQUAL_STRING((const char *)sample.client_id, (const char *)retrieved->client_id);
    TEST_ASSERT_EQUAL_STRING(sample.name, retrieved->name);
    TEST_ASSERT_EQUAL_STRING(sample.pub_pem, retrieved->pub_pem);
}

void test_clients_add_duplicate(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    client_t sample = make_client("dup", "Duplicate", "PEM", 5U, 0U);
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&sample));
    TEST_ASSERT_EQUAL_INT(CLIENTS_ERR_EXISTS, clients_add(&sample));
}

void test_clients_add_invalid_name(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    client_t sample = make_client("badname", "Valid", "PEM", 1U, 0U);
    memset(sample.name, 'A', CLIENTS_NAME_MAX);
    sample.name[CLIENTS_NAME_MAX - 1U] = 'B';

    TEST_ASSERT_EQUAL_INT(CLIENTS_ERR_TOO_LONG, clients_add(&sample));
}

void test_clients_get_unknown(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    client_t sample = make_client("known", "Known", "PEM", 1U, 0U);
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&sample));

    client_uid_t unknown;
    copy_string((char *)unknown, sizeof(unknown), "unknown");

    const client_t *retrieved = NULL;
    TEST_ASSERT_EQUAL_INT(CLIENTS_ERR_NOT_FOUND, clients_get(unknown, &retrieved));
    TEST_ASSERT_NULL(retrieved);
}

void test_clients_get_field_helpers(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    client_t sample = make_client("fields", "Field User", "PEM DATA", 77U, 0x05U);
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&sample));

    char name_buf[CLIENTS_NAME_MAX];
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_get_name(sample.client_id, name_buf, sizeof(name_buf)));
    TEST_ASSERT_EQUAL_STRING(sample.name, name_buf);

    char pem_buf[CLIENTS_PUBPEM_CAP];
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_get_pub_pem(sample.client_id, pem_buf, sizeof(pem_buf)));
    TEST_ASSERT_EQUAL_STRING(sample.pub_pem, pem_buf);

    nonce_t nonce = 0U;
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_get_nonce(sample.client_id, &nonce));
    TEST_ASSERT_EQUAL_UINT32(sample.nonce, nonce);

    uint8_t flags = 0U;
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_get_allow_flags(sample.client_id, &flags));
    TEST_ASSERT_EQUAL_UINT32(sample.allow_flags, flags);

    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_set_nonce(sample.client_id, 1234U));
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_get_nonce(sample.client_id, &nonce));
    TEST_ASSERT_EQUAL_UINT32(1234U, nonce);
}

void test_clients_allow_flags_helpers(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    client_t sample = make_client("allow", "Allow", "PEM", 1U, 0U);
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&sample));

    bool allowed = true;
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_allow_bluetooth_get(sample.client_id, &allowed));
    TEST_ASSERT_FALSE(allowed);

    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_allow_bluetooth_set(sample.client_id, true));
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_allow_bluetooth_get(sample.client_id, &allowed));
    TEST_ASSERT_TRUE(allowed);

    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_allow_public_mqtt_set(sample.client_id, true));
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_allow_public_mqtt_get(sample.client_id, &allowed));
    TEST_ASSERT_TRUE(allowed);

    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_allow_public_mqtt_set(sample.client_id, false));
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_allow_public_mqtt_get(sample.client_id, &allowed));
    TEST_ASSERT_FALSE(allowed);
}

void test_clients_get_ids(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    client_t first  = make_client("id1", "One", "PEM1", 1U, 0U);
    client_t second = make_client("id2", "Two", "PEM2", 2U, 1U);
    client_t third  = make_client("id3", "Three", "PEM3", 3U, 2U);

    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&first));
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&second));
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&third));

    client_uid_t ids[3];
    size_t       count = 0U;
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_get_ids(ids, 3U, &count));
    TEST_ASSERT_EQUAL_UINT32(3U, count);
    expect_client_id_equal(first.client_id, ids[0]);
    expect_client_id_equal(second.client_id, ids[1]);
    expect_client_id_equal(third.client_id, ids[2]);
}

void test_clients_get_ids_buffer_too_small(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    client_t first  = make_client("id1", "One", "PEM1", 1U, 0U);
    client_t second = make_client("id2", "Two", "PEM2", 2U, 1U);

    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&first));
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&second));

    client_uid_t ids[1];
    size_t       count = 0U;
    TEST_ASSERT_EQUAL_INT(CLIENTS_ERR_TOO_LONG, clients_get_ids(ids, 1U, &count));
    TEST_ASSERT_EQUAL_UINT32(2U, count);
}

void test_clients_size(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    TEST_ASSERT_EQUAL_UINT32(0U, clients_size());

    client_t first  = make_client("id1", "One", "PEM1", 1U, 0U);
    client_t second = make_client("id2", "Two", "PEM2", 2U, 1U);

    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&first));
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&second));

    TEST_ASSERT_EQUAL_UINT32(2U, clients_size());
}

void test_clients_persistence(void)
{
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    client_t sample = make_client("persist", "Persist", "PEM", 11U, 0U);
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_add(&sample));

    close_client_db();

    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, open_client_db());

    const client_t *retrieved = NULL;
    TEST_ASSERT_EQUAL_INT(CLIENTS_OK, clients_get(sample.client_id, &retrieved));
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING(sample.name, retrieved->name);
}

static client_t make_client(const char *id, const char *name, const char *pem, nonce_t nonce, uint8_t flags)
{
    client_t client;
    memset(&client, 0, sizeof(client));
    client.allow_flags = flags;
    client.nonce       = nonce;
    copy_string((char *)client.client_id, sizeof(client.client_id), id);
    copy_string(client.name, sizeof(client.name), name);
    copy_string(client.pub_pem, sizeof(client.pub_pem), pem);
    return client;
}

static void copy_string(char *dest, size_t capacity, const char *src)
{
    if ((NULL == dest) || (0U == capacity))
        return;

    memset(dest, 0, capacity);

    if (NULL == src)
        return;

    size_t index = 0U;
    while ((index + 1U < capacity) && (src[index] != '\0'))
    {
        dest[index] = src[index];
        ++index;
    }

    dest[index < capacity ? index : (capacity - 1U)] = '\0';
}

static void expect_client_id_equal(const client_uid_t expected, const client_uid_t actual)
{
    TEST_ASSERT_EQUAL_INT(0, memcmp(expected, actual, sizeof(client_uid_t)));
}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    RUN_TEST(test_clients_namespace_length);
    RUN_TEST(test_clients_requires_init);
    RUN_TEST(test_clients_open_close);
    RUN_TEST(test_clients_add_and_get);
    RUN_TEST(test_clients_add_duplicate);
    RUN_TEST(test_clients_add_invalid_name);
    RUN_TEST(test_clients_get_unknown);
    RUN_TEST(test_clients_get_field_helpers);
    RUN_TEST(test_clients_allow_flags_helpers);
    RUN_TEST(test_clients_get_ids);
    RUN_TEST(test_clients_get_ids_buffer_too_small);
    RUN_TEST(test_clients_size);
    RUN_TEST(test_clients_persistence);
    return UNITY_END();
}
