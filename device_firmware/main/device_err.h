#pragma once
#include "esp_err.h"
#ifdef __cplusplus
#include <string_view>
#include <utility>
#endif

// Add new device error codes in MAIN_ERR_LIST below

// Standard format for printing error name and hex value in logs.
// Usage example:
//   ESP_LOGW(TAG, "Failed: " ERR_FORMAT, esp_err_to_str(err), err);
// Note: keep it a macro so it works in both C and C++ format strings.
#define ERR_FORMAT "\"%s\" (0x%x)"

// Macro to log a standardized "not implemented" warning for the current function.
// Expands to: ESP_LOGW(tag, "Function %s is not implemented", __FUNCTION__|__func__)
#if defined(__cplusplus)
#define ESP_LOG_NOTIMPLEMENTED(tag) ESP_LOGW(tag, "Function %s is not implemented", __FUNCTION__)
#else
#define ESP_LOG_NOTIMPLEMENTED(tag) ESP_LOGW(tag, "Function %s is not implemented", __func__)
#endif // defined(__cplusplus)

// Definitions for device error constants.
// Keep in sync with esp_err.h
#define ESP_ERR_DEV_BASE 0xF0000

// Define the list of device errors here using the X macro helper.
// Each entry is: X(NAME, assignment-token-for-first, "human-readable string")
// For the first entry use `= ESP_ERR_DEV_BASE + 1`, subsequent entries leave the second token empty.
// To add a new error, add a new line here and a corresponding human-readable string will be generated
// automatically into the table.
#define MAIN_ERR_LIST \
    X(ESP_ERR_DEV_NOT_IMPLEMENTED, = ESP_ERR_DEV_BASE + 1, "Feature or functionality not implemented") \
    X(ESP_ERR_DEV_INIT, , "Device initialization failed")

    // Add new device error codes here

// Generate an enum with auto-incremented values starting from ESP_ERR_DEV_BASE + 1
enum {
#define X(name, assign, desc) name assign,
    MAIN_ERR_LIST
#undef X
};

#ifdef __cplusplus
namespace main_errors {
    using entry_t = std::pair<esp_err_t, std::string_view>;

#ifdef CONFIG_ESP_ERR_TO_NAME_LOOKUP
    // Table of (code, name) entries generated from the same list; this is evaluated
    // at compile time and can be used by the wrapper function.
    constexpr entry_t k_table[] = {
#define X(name, assign, desc) { name, desc },
        MAIN_ERR_LIST
#undef X
    };

    inline const char *impl_cpp_esp_err_to_str(esp_err_t code) noexcept {
        for (auto const &e : k_table) {
            if (e.first == code) {
                return e.second.data();
            }
        }
        return esp_err_to_name(code);
    }
#else
    inline const char *impl_cpp_esp_err_to_str(esp_err_t code) noexcept { return esp_err_to_name(code); }
#endif
} // namespace main_errors

// Expose a C-linkage inline wrapper so C code can call esp_err_to_str() without
// depending on C++ headers.
extern "C" inline const char *esp_err_to_str(esp_err_t code) noexcept {
    return main_errors::impl_cpp_esp_err_to_str(code);
}

#else // not __cplusplus

// In pure C compiles we just forward directly to esp_err_to_name(), avoiding
// any C++ headers or types.
static inline const char *esp_err_to_str(esp_err_t code) { return esp_err_to_name(code); }

#endif // __cplusplus