#pragma once

#include <string>
#include <unordered_map>
#include "device_err.h" // uses esp_err_t

class NVM
{
public:
    static NVM& getInstance() noexcept;

    esp_err_t Init() noexcept { return ESP_OK; }

    esp_err_t ReadString(const char *partition,
                         const char *namespace_name,
                         const char *key,
                         char *buffer,
                         size_t size);

    esp_err_t WriteString(const char *partition,
                          const char *namespace_name,
                          const char *key,
                          const char *value);

private:
    NVM() = default;
    ~NVM() = default;

    std::unordered_map<std::string, std::string> storage_;
};
