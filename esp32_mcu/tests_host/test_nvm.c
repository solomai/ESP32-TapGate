#include "unity.h"

#include "nvm/nvm.h"
#include "nvm/nvm_partition.h"
#include "nvs_flash.h"

#include <stdint.h>
#include <string.h>

#define MAX_CALLS 8

static const char *init_call_labels[MAX_CALLS];
static size_t init_call_count;
static esp_err_t init_results[MAX_CALLS];
static size_t init_result_count;
static size_t init_result_index;

static const char *erase_call_labels[MAX_CALLS];
static size_t erase_call_count;
static esp_err_t erase_results[MAX_CALLS];
static size_t erase_result_count;
static size_t erase_result_index;

static void reset_stubs(void)
{
    memset(init_call_labels, 0, sizeof(init_call_labels));
    init_call_count = 0;
    memset(init_results, 0, sizeof(init_results));
    init_result_count = 0;
    init_result_index = 0;

    memset(erase_call_labels, 0, sizeof(erase_call_labels));
    erase_call_count = 0;
    memset(erase_results, 0, sizeof(erase_results));
    erase_result_count = 0;
    erase_result_index = 0;
}

static void set_init_results(const esp_err_t *values, size_t count)
{
    memset(init_results, 0, sizeof(init_results));
    if (count > MAX_CALLS)
        count = MAX_CALLS;
    memcpy(init_results, values, count * sizeof(values[0]));
    init_result_count = count;
    init_result_index = 0;
}

static void set_erase_results(const esp_err_t *values, size_t count)
{
    memset(erase_results, 0, sizeof(erase_results));
    if (count > MAX_CALLS)
        count = MAX_CALLS;
    memcpy(erase_results, values, count * sizeof(values[0]));
    erase_result_count = count;
    erase_result_index = 0;
}

esp_err_t nvs_flash_init_partition(const char *partition_name)
{
    if (init_call_count < MAX_CALLS)
        init_call_labels[init_call_count] = partition_name;
    init_call_count++;

    if (init_result_index < init_result_count)
        return init_results[init_result_index++];

    return ESP_OK;
}

esp_err_t nvs_flash_erase_partition(const char *partition_name)
{
    if (erase_call_count < MAX_CALLS)
        erase_call_labels[erase_call_count] = partition_name;
    erase_call_count++;

    if (erase_result_index < erase_result_count)
        return erase_results[erase_result_index++];

    return ESP_OK;
}

void setUp(void)
{
    reset_stubs();
}

void tearDown(void) {}

void test_nvm_init_initializes_all_partitions(void)
{
    esp_err_t err = nvm_init();
    TEST_ASSERT_EQUAL_INT(ESP_OK, err);
    TEST_ASSERT_EQUAL_UINT32(3u, (uint32_t)init_call_count);
    TEST_ASSERT_NOT_NULL(init_call_labels[0]);
    TEST_ASSERT_NOT_NULL(init_call_labels[1]);
    TEST_ASSERT_NOT_NULL(init_call_labels[2]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_DEFAULT, init_call_labels[0]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_CLIENTS, init_call_labels[1]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_NONCE, init_call_labels[2]);
    TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)erase_call_count);
}

void test_nvm_init_handles_no_free_pages(void)
{
    const esp_err_t init_sequence[] = {
        ESP_ERR_NVS_NO_FREE_PAGES,
        ESP_OK,
        ESP_OK,
        ESP_OK,
    };
    const esp_err_t erase_sequence[] = {
        ESP_OK,
    };

    set_init_results(init_sequence, sizeof(init_sequence) / sizeof(init_sequence[0]));
    set_erase_results(erase_sequence, sizeof(erase_sequence) / sizeof(erase_sequence[0]));

    esp_err_t err = nvm_init();
    TEST_ASSERT_EQUAL_INT(ESP_OK, err);
    TEST_ASSERT_EQUAL_UINT32(4u, (uint32_t)init_call_count);
    TEST_ASSERT_NOT_NULL(init_call_labels[0]);
    TEST_ASSERT_NOT_NULL(init_call_labels[1]);
    TEST_ASSERT_NOT_NULL(init_call_labels[2]);
    TEST_ASSERT_NOT_NULL(init_call_labels[3]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_DEFAULT, init_call_labels[0]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_DEFAULT, init_call_labels[1]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_CLIENTS, init_call_labels[2]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_NONCE, init_call_labels[3]);
    TEST_ASSERT_EQUAL_UINT32(1u, (uint32_t)erase_call_count);
    TEST_ASSERT_NOT_NULL(erase_call_labels[0]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_DEFAULT, erase_call_labels[0]);
}

void test_nvm_init_stops_when_erase_fails(void)
{
    const esp_err_t init_sequence[] = {
        ESP_ERR_NVS_NO_FREE_PAGES,
    };
    const esp_err_t erase_sequence[] = {
        ESP_FAIL,
    };

    set_init_results(init_sequence, sizeof(init_sequence) / sizeof(init_sequence[0]));
    set_erase_results(erase_sequence, sizeof(erase_sequence) / sizeof(erase_sequence[0]));

    esp_err_t err = nvm_init();
    TEST_ASSERT_EQUAL_INT(ESP_FAIL, err);
    TEST_ASSERT_EQUAL_UINT32(1u, (uint32_t)init_call_count);
    TEST_ASSERT_EQUAL_UINT32(1u, (uint32_t)erase_call_count);
    TEST_ASSERT_NOT_NULL(erase_call_labels[0]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_DEFAULT, erase_call_labels[0]);
}

void test_nvm_init_propagates_init_error(void)
{
    const esp_err_t init_sequence[] = {
        ESP_ERR_NOT_FOUND,
    };

    set_init_results(init_sequence, sizeof(init_sequence) / sizeof(init_sequence[0]));

    esp_err_t err = nvm_init();
    TEST_ASSERT_EQUAL_INT(ESP_ERR_NOT_FOUND, err);
    TEST_ASSERT_EQUAL_UINT32(1u, (uint32_t)init_call_count);
    TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)erase_call_count);
}

void test_nvm_init_handles_new_version_found(void)
{
    const esp_err_t init_sequence[] = {
        ESP_ERR_NVS_NEW_VERSION_FOUND,
        ESP_OK,
        ESP_OK,
        ESP_OK,
    };
    const esp_err_t erase_sequence[] = {
        ESP_OK,
    };

    set_init_results(init_sequence, sizeof(init_sequence) / sizeof(init_sequence[0]));
    set_erase_results(erase_sequence, sizeof(erase_sequence) / sizeof(erase_sequence[0]));

    esp_err_t err = nvm_init();
    TEST_ASSERT_EQUAL_INT(ESP_OK, err);
    TEST_ASSERT_EQUAL_UINT32(4u, (uint32_t)init_call_count);
    TEST_ASSERT_NOT_NULL(init_call_labels[0]);
    TEST_ASSERT_NOT_NULL(init_call_labels[1]);
    TEST_ASSERT_NOT_NULL(init_call_labels[2]);
    TEST_ASSERT_NOT_NULL(init_call_labels[3]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_DEFAULT, init_call_labels[0]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_DEFAULT, init_call_labels[1]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_CLIENTS, init_call_labels[2]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_NONCE, init_call_labels[3]);
    TEST_ASSERT_EQUAL_UINT32(1u, (uint32_t)erase_call_count);
    TEST_ASSERT_NOT_NULL(erase_call_labels[0]);
    TEST_ASSERT_EQUAL_STRING(NVM_PARTITION_DEFAULT, erase_call_labels[0]);
}

int main(int argc, char **argv)
{
    UnityConfigureFromArgs(argc, (const char **)argv);
    UNITY_BEGIN();
    RUN_TEST(test_nvm_init_initializes_all_partitions);
    RUN_TEST(test_nvm_init_handles_no_free_pages);
    RUN_TEST(test_nvm_init_stops_when_erase_fails);
    RUN_TEST(test_nvm_init_propagates_init_error);
    RUN_TEST(test_nvm_init_handles_new_version_found);
    return UNITY_END();
}
