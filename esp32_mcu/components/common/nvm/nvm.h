#pragma once

#include "esp_err.h"

#include <stddef.h>
#include <stdint.h>

esp_err_t nvm_init(void);

esp_err_t nvm_read_string(const char *partition,
                          const char *namespace_name,
                          const char *key,
                          char *buffer,
                          size_t size);

esp_err_t nvm_write_string(const char *partition,
                           const char *namespace_name,
                           const char *key,
                           const char *value);

esp_err_t nvm_read_u32(const char *partition,
                       const char *namespace_name,
                       const char *key,
                       uint32_t *value);

esp_err_t nvm_write_u32(const char *partition,
                        const char *namespace_name,
                        const char *key,
                        uint32_t value);

esp_err_t nvm_read_u8(const char *partition,
                      const char *namespace_name,
                      const char *key,
                      uint8_t *value);
     
esp_err_t nvm_write_u8(const char *partition,
                       const char *namespace_name,
                       const char *key,
                       uint8_t value);
     