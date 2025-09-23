#include "http_service_logic.h"

#include <ctype.h>
#include <string.h>

static void write_message(char *buffer, size_t buffer_size, const char *message)
{
    if (!buffer || buffer_size == 0) {
        return;
    }
    if (!message) {
        buffer[0] = '\0';
        return;
    }
    size_t len = strlen(message);
    if (len >= buffer_size) {
        len = buffer_size - 1;
    }
    memcpy(buffer, message, len);
    buffer[len] = '\0';
}

static void trim_string(char *value, size_t capacity)
{
    if (!value || capacity == 0) {
        return;
    }

    char *start = value;
    while (*start && isspace((unsigned char)*start)) {
        ++start;
    }

    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) {
        --end;
    }

    size_t length = (size_t)(end - start);
    if (start != value && length > 0) {
        memmove(value, start, length);
    } else if (length == 0) {
        value[0] = '\0';
        return;
    }

    value[length] = '\0';
}

static bool contains_only_printable(const char *value)
{
    if (!value) {
        return false;
    }

    for (const unsigned char *ptr = (const unsigned char *)value; *ptr; ++ptr) {
        if (!isprint(*ptr)) {
            return false;
        }
    }
    return true;
}

esp_err_t http_service_logic_validate_credentials(const http_service_credentials_t *credentials,
                                                  char *message,
                                                  size_t message_size)
{
    if (!credentials) {
        write_message(message, message_size, "Internal error");
        return ESP_ERR_INVALID_ARG;
    }

    http_service_credentials_t temp = *credentials;
    http_service_logic_trim_credentials(&temp);

    size_t ssid_len = strlen(temp.ssid);
    if (ssid_len == 0) {
        write_message(message, message_size, "SSID is required");
        return ESP_ERR_INVALID_ARG;
    }
    if (ssid_len >= MAX_SSID_SIZE) {
        write_message(message, message_size, "SSID is too long");
        return ESP_ERR_INVALID_SIZE;
    }
    if (!contains_only_printable(temp.ssid)) {
        write_message(message, message_size, "SSID contains invalid characters");
        return ESP_ERR_INVALID_STATE;
    }

    size_t password_len = strlen(temp.password);
    if (password_len < WPA2_MINIMUM_PASSWORD_LENGTH) {
        write_message(message, message_size, "Password must be at least 8 characters");
        return ESP_ERR_INVALID_SIZE;
    }
    if (password_len >= MAX_PASSWORD_SIZE) {
        write_message(message, message_size, "Password is too long");
        return ESP_ERR_INVALID_SIZE;
    }
    if (!contains_only_printable(temp.password)) {
        write_message(message, message_size, "Password contains invalid characters");
        return ESP_ERR_INVALID_STATE;
    }

    write_message(message, message_size, "");
    return ESP_OK;
}

bool http_service_logic_password_is_valid(const char *password)
{
    size_t length = password ? strlen(password) : 0;
    return length >= WPA2_MINIMUM_PASSWORD_LENGTH;
}

const char *http_service_logic_select_initial_uri(const http_service_credentials_t *credentials)
{
    if (!credentials) {
        return HTTP_SERVICE_URI_AP_PAGE;
    }
    return http_service_logic_password_is_valid(credentials->password)
               ? HTTP_SERVICE_URI_MAIN_PAGE
               : HTTP_SERVICE_URI_AP_PAGE;
}

void http_service_logic_prepare_ap_state(const http_service_credentials_t *credentials,
                                         http_service_ap_state_t *state)
{
    if (!state) {
        return;
    }

    if (credentials) {
        state->credentials = *credentials;
    } else {
        memset(&state->credentials, 0, sizeof(state->credentials));
    }
    state->cancel_enabled = http_service_logic_password_is_valid(state->credentials.password);
}

void http_service_logic_trim_credentials(http_service_credentials_t *credentials)
{
    if (!credentials) {
        return;
    }

    trim_string(credentials->ssid, sizeof(credentials->ssid));
    trim_string(credentials->password, sizeof(credentials->password));
}
