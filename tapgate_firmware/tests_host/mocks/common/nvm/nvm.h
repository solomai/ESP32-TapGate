#pragma once

#include <string>
#include <unordered_map>
#include "device_err.h"

class NVMWrapper
{
public:
    static NVMWrapper& getInstance() noexcept;

    NVMWrapper(const NVMWrapper&)            = delete;
    NVMWrapper& operator=(const NVMWrapper&) = delete;
    NVMWrapper(NVMWrapper&&)                  = delete;
    NVMWrapper& operator=(NVMWrapper&&)       = delete;

    esp_err_t Init() noexcept { return ESP_OK; }

    esp_err_t ReadString(const char* partition,
                         const char* namespace_name,
                         const char* key,
                         char* buffer,
                         size_t size);

    esp_err_t WriteString(const char* partition,
                          const char* namespace_name,
                          const char* key,
                          const char* value);

    // Reset all stored values — call between tests to ensure isolation
    void reset() noexcept { storage_.clear(); not_found_err_ = ESP_OK; }

    // Set error code returned for missing keys (default: ESP_OK + empty buffer).
    // Use ESP_ERR_NVS_NOT_FOUND to simulate real NVS behaviour on first boot.
    void set_not_found_err(esp_err_t err) noexcept { not_found_err_ = err; }

private:
    NVMWrapper() = default;
    ~NVMWrapper() = default;

    std::unordered_map<std::string, std::string> storage_;
    esp_err_t not_found_err_ = ESP_OK;
};

// Global instance of NVMWrapper
extern NVMWrapper& NVM;
