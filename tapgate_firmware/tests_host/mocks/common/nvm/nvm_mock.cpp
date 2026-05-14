#include "nvm.h"
#include <cstring>
#include <string>

NVM& NVM::getInstance() noexcept {
    static NVM inst;
    return inst;
}

static std::string make_key(const char *partition, const char *ns, const char *key) {
    std::string s = partition ? partition : "";
    s.push_back('/');
    s += ns ? ns : "";
    s.push_back('/');
    s += key ? key : "";
    return s;
}

esp_err_t NVM::ReadString(const char *partition,
                          const char *namespace_name,
                          const char *key,
                          char *buffer,
                          size_t size) {
    auto k = make_key(partition, namespace_name, key);
    auto it = storage_.find(k);
    if (it == storage_.end()) {
        if (size > 0) buffer[0] = '\0';
        return ESP_FAIL; // not found
    }
    const std::string &val = it->second;
    const size_t copy_len = std::min(val.size(), (size > 0 ? size - 1 : 0));
    if (size > 0 && copy_len > 0) std::memcpy(buffer, val.data(), copy_len);
    if (size > 0) buffer[copy_len] = '\0';
    return ESP_OK;
}

esp_err_t NVM::WriteString(const char *partition,
                           const char *namespace_name,
                           const char *key,
                           const char *value) {
    auto k = make_key(partition, namespace_name, key);
    storage_[k] = value ? value : std::string();
    return ESP_OK;
}
