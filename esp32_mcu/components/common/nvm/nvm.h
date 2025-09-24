#pragma once

#include "esp_err.h"
#include "nvs.h"
#include <stddef.h>

esp_err_t nvm_init(void);

esp_err_t nvm_open_partition_handle(const char *partition_label,
                                    const char *name_space,
                                    nvs_open_mode mode,
                                    nvs_handle_t *handle);

esp_err_t nvm_read_string_from_partition(const char *partition_label,
                                         const char *name_space,
                                         const char *key,
                                         char *buffer,
                                         size_t size);

esp_err_t nvm_write_string_to_partition(const char *partition_label,
                                        const char *name_space,
                                        const char *key,
                                        const char *value);

esp_err_t nvm_read_u32_from_partition(const char *partition_label,
                                      const char *name_space,
                                      const char *key,
                                      uint32_t *value);

esp_err_t nvm_write_u32_to_partition(const char *partition_label,
                                     const char *name_space,
                                     const char *key,
                                     uint32_t value);
