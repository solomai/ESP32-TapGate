/**
 * NVM (Non-Volatile Memory) context singleton
 * 
 */
#pragma once
#include "esp_err.h"

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

protected:
    esp_err_t EnsurePartitionReady(const char *partition_label);

private:
    NVM() = default;
    ~NVM() = default;
};
