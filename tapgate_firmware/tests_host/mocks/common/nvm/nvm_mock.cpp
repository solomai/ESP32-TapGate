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

// ---------------------------------------------------------------------------
// String
// ---------------------------------------------------------------------------

esp_err_t NVMWrapper::ReadString(const char* partition,
                                 const char* namespace_name,
                                 const char* key,
                                 char* buffer,
                                 size_t size)
{
    if (!buffer || size == 0)
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(mutex_);
    if (read_err_ != ESP_OK) return read_err_;
    buffer[0] = '\0';
    auto it = storage_.find(make_key(partition, namespace_name, key));
    if (it == storage_.end()) {
        if (not_found_err_ != ESP_OK)
            return not_found_err_;
        return ESP_OK;
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
    std::lock_guard<std::mutex> lock(mutex_);
    storage_[make_key(partition, namespace_name, key)] = value ? value : std::string();
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Blob — raw bytes stored as std::string
// ---------------------------------------------------------------------------

esp_err_t NVMWrapper::ReadBlob(const char* partition,
                               const char* namespace_name,
                               const char* key,
                               void* buffer,
                               size_t size)
{
    if (!buffer || size == 0)
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(mutex_);
    if (read_err_ != ESP_OK) return read_err_;
    auto it = storage_.find(make_key(partition, namespace_name, key));
    if (it == storage_.end())
        return not_found_err_ != ESP_OK ? not_found_err_ : ESP_ERR_NVS_NOT_FOUND;

    if (it->second.size() != size)
        return ESP_ERR_INVALID_SIZE;

    std::memcpy(buffer, it->second.data(), size);
    return ESP_OK;
}

esp_err_t NVMWrapper::WriteBlob(const char* partition,
                                const char* namespace_name,
                                const char* key,
                                const void* value,
                                size_t size)
{
    if (!value || size == 0)
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(mutex_);
    storage_[make_key(partition, namespace_name, key)] =
        std::string(static_cast<const char*>(value), size);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// U32 — serialised as 4 bytes (little-endian host order)
// ---------------------------------------------------------------------------

esp_err_t NVMWrapper::ReadU32(const char* partition,
                              const char* namespace_name,
                              const char* key,
                              uint32_t* value)
{
    if (!value)
        return ESP_ERR_INVALID_ARG;

    std::lock_guard<std::mutex> lock(mutex_);
    if (read_err_ != ESP_OK) return read_err_;
    auto it = storage_.find(make_key(partition, namespace_name, key));
    if (it == storage_.end())
        return not_found_err_ != ESP_OK ? not_found_err_ : ESP_ERR_NVS_NOT_FOUND;

    if (it->second.size() != sizeof(uint32_t))
        return ESP_ERR_INVALID_SIZE;

    std::memcpy(value, it->second.data(), sizeof(uint32_t));
    return ESP_OK;
}

esp_err_t NVMWrapper::WriteU32(const char* partition,
                               const char* namespace_name,
                               const char* key,
                               uint32_t value)
{
    std::string bytes(sizeof(uint32_t), '\0');
    std::memcpy(bytes.data(), &value, sizeof(uint32_t));

    std::lock_guard<std::mutex> lock(mutex_);
    storage_[make_key(partition, namespace_name, key)] = std::move(bytes);
    return ESP_OK;
}
