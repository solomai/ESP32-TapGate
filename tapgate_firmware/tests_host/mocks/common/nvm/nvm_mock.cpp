#include "nvm.h"
#include <cstring>

NVMWrapper& NVMWrapper::getInstance() noexcept
{
    static NVMWrapper inst;
    return inst;
}

NVMWrapper& NVM = NVMWrapper::getInstance();

static std::string make_key(const char* partition, const char* ns, const char* key)
{
    std::string s = partition ? partition : "";
    s.push_back('/');
    s += ns ? ns : "";
    s.push_back('/');
    s += key ? key : "";
    return s;
}

esp_err_t NVMWrapper::ReadString(const char* partition,
                                 const char* namespace_name,
                                 const char* key,
                                 char* buffer,
                                 size_t size)
{
    if (!buffer || size == 0)
        return ESP_ERR_INVALID_ARG;

    buffer[0] = '\0';
    auto it = storage_.find(make_key(partition, namespace_name, key));
    if (it == storage_.end()) {
        if (not_found_err_ != ESP_OK)
            return not_found_err_;
        return ESP_OK; // empty buffer signals "key absent" to the caller
    }

    const size_t copy_len = std::min(it->second.size(), size - 1);
    std::memcpy(buffer, it->second.data(), copy_len);
    buffer[copy_len] = '\0';
    return ESP_OK;
}

esp_err_t NVMWrapper::WriteString(const char* partition,
                                  const char* namespace_name,
                                  const char* key,
                                  const char* value)
{
    storage_[make_key(partition, namespace_name, key)] = value ? value : std::string();
    return ESP_OK;
}
