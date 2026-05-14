/**
 * NVM (Non-Volatile Memory) context singleton
 * 
 */
#pragma once
#include "device_err.h"
#include "nvm_partition.h"

class NVM
{
public:
    static NVM& getInstance() noexcept;

    // Delete copy constructor and assignment operator
    NVM(const NVM&) = delete;
    NVM& operator=(const NVM&) = delete;
    // Delete move constructor and assignment operator
    NVM(NVM&&) = delete;
    NVM& operator=(NVM&&) = delete;

public:
    esp_err_t Init() noexcept;

    esp_err_t ReadString(const char *partition,
                         const char *namespace_name,
                         const char *key,
                         char *buffer,
                         size_t size);
    esp_err_t WriteString(const char *partition,
                          const char *namespace_name,
                          const char *key,
                          const char *value);
    esp_err_t StringSize(const char *partition,
                         const char *namespace_name,
                         const char *key,
                         size_t &size);

    esp_err_t ReadU32(const char *partition,
                       const char *namespace_name,
                       const char *key,
                       uint32_t *value);
    esp_err_t WriteU32(const char *partition,
                        const char *namespace_name,
                        const char *key,
                        uint32_t value);

    esp_err_t ReadU8(const char *partition,
                      const char *namespace_name,
                      const char *key,
                      uint8_t *value);
    esp_err_t WriteU8(const char *partition,
                       const char *namespace_name,
                       const char *key,
                       uint8_t value);

protected:
    esp_err_t EnsurePartitionReady(const char *partition_label);

private:
    NVM() = default;
    ~NVM() = default;
};
