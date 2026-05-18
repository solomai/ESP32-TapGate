#pragma once

#include <cstdint>
#include <mutex>
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

    esp_err_t ReadBlob(const char* partition,
                       const char* namespace_name,
                       const char* key,
                       void* buffer,
                       size_t size);

    esp_err_t WriteBlob(const char* partition,
                        const char* namespace_name,
                        const char* key,
                        const void* value,
                        size_t size);

    esp_err_t ReadU32(const char* partition,
                      const char* namespace_name,
                      const char* key,
                      uint32_t* value);

    esp_err_t WriteU32(const char* partition,
                       const char* namespace_name,
                       const char* key,
                       uint32_t value);

    // Reset all stored values and injected errors — call between tests to ensure isolation
    void reset() noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        storage_.clear();
        not_found_err_ = ESP_OK;
        read_err_      = ESP_OK;
    }

    // Set error returned for missing keys (default: ESP_OK for strings, NOT_FOUND for blobs/u32).
    // Pass ESP_ERR_NVS_NOT_FOUND to simulate real NVS namespace-absent (first boot) behaviour.
    void set_not_found_err(esp_err_t err) noexcept { not_found_err_ = err; }

    // Inject a hard read error returned by all Read* calls regardless of key presence.
    // Use to test NVM failure propagation paths (e.g. ESP_ERR_NO_MEM).
    void set_read_err(esp_err_t err) noexcept { read_err_ = err; }

private:
    NVMWrapper() = default;
    ~NVMWrapper() = default;

    mutable std::mutex mutex_;
    // Stores all value types as raw byte strings, keyed by "partition/namespace/key"
    std::unordered_map<std::string, std::string> storage_;
    esp_err_t not_found_err_ = ESP_OK;
    esp_err_t read_err_      = ESP_OK;
};

// Global instance of NVMWrapper
extern NVMWrapper& NVM;
