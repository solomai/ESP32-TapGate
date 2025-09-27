#include "http_service.h"

#include "admin_portal_state.h"
#include "admin_portal_device.h"
#include "logs.h"
#include "wifi_manager.h"
#include "nvm/nvm.h"

#include "esp_random.h"
#include "esp_timer.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pages/page_auth.h"
#include "pages/page_busy.h"
#include "pages/page_change_psw.h"
#include "pages/page_clients.h"
#include "pages/page_device.h"
#include "pages/page_enroll.h"
#include "pages/page_events.h"
#include "pages/page_main.h"
#include "pages/page_off.h"
#include "pages/page_wifi.h"

static const char *TAG = "AdminPortal";

#define ADMIN_PORTAL_NAMESPACE       "admin_portal"
#define ADMIN_PORTAL_KEY_PSW         "AP_PSW"
#define ADMIN_PORTAL_KEY_SSID        "AP_SSID"
#define ADMIN_PORTAL_KEY_IDLE_MIN    "AP_IDLE_TIMEOUT"
#define ADMIN_PORTAL_COOKIE_NAME     "tg_session"
#define DEFAULT_IDLE_TIMEOUT_MINUTES 15U
#define DEFAULT_SESSION_MAX_AGE 900U  // 15 minutes in seconds

typedef struct {
    const char *uri;
    const uint8_t *start;
    const uint8_t *end;
    const char *content_type;
    bool compressed;
} admin_portal_asset_t;

static httpd_handle_t g_server = NULL;
static admin_portal_state_t g_state;

static const admin_portal_page_descriptor_t *const g_pages[] = {
    &admin_portal_page_enroll,
    &admin_portal_page_auth,
    &admin_portal_page_change_psw,
    &admin_portal_page_device,
    &admin_portal_page_wifi,
    &admin_portal_page_clients,
    &admin_portal_page_events,
    &admin_portal_page_main,
    &admin_portal_page_busy,
    &admin_portal_page_off,
};

extern const uint8_t _binary_logo_png_start[];
extern const uint8_t _binary_logo_png_end[];
extern const uint8_t _binary_styles_css_gz_start[];
extern const uint8_t _binary_styles_css_gz_end[];
extern const uint8_t _binary_app_js_gz_start[];
extern const uint8_t _binary_app_js_gz_end[];

static const admin_portal_asset_t g_assets[] = {
    { .uri = "/assets/logo.png", .start = _binary_logo_png_start, .end = _binary_logo_png_end, .content_type = "image/png", .compressed = false },
    { .uri = "/assets/styles.css", .start = _binary_styles_css_gz_start, .end = _binary_styles_css_gz_end, .content_type = "text/css", .compressed = true },
    { .uri = "/assets/app.js", .start = _binary_app_js_gz_start, .end = _binary_app_js_gz_end, .content_type = "application/javascript", .compressed = true },
};

static const char *session_status_name(admin_portal_session_status_t status)
{
    switch (status)
    {
        case ADMIN_PORTAL_SESSION_NONE:
            return "none";
        case ADMIN_PORTAL_SESSION_MATCH:
            return "match";
        case ADMIN_PORTAL_SESSION_EXPIRED:
            return "expired";
        case ADMIN_PORTAL_SESSION_BUSY:
            return "busy";
        default:
            return "unknown";
    }
}

static const char *page_name(admin_portal_page_t page)
{
    switch (page)
    {
        case ADMIN_PORTAL_PAGE_ENROLL:
            return "enroll";
        case ADMIN_PORTAL_PAGE_AUTH:
            return "auth";
        case ADMIN_PORTAL_PAGE_CHANGE_PASSWORD:
            return "change_password";
        case ADMIN_PORTAL_PAGE_DEVICE:
            return "device";
        case ADMIN_PORTAL_PAGE_WIFI:
            return "wifi";
        case ADMIN_PORTAL_PAGE_CLIENTS:
            return "clients";
        case ADMIN_PORTAL_PAGE_EVENTS:
            return "events";
        case ADMIN_PORTAL_PAGE_MAIN:
            return "main";
        case ADMIN_PORTAL_PAGE_BUSY:
            return "busy";
        case ADMIN_PORTAL_PAGE_OFF:
            return "off";
        default:
            return "unknown";
    }
}

static uint64_t get_now_ms(void)
{
    return esp_timer_get_time() / 1000ULL;
}

static uint64_t minutes_to_ms(uint32_t minutes)
{
    return (uint64_t)minutes * 60ULL * 1000ULL;
}

static void generate_session_token(char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1])
{
    uint8_t buffer[ADMIN_PORTAL_TOKEN_MAX_LEN / 2];
    esp_fill_random(buffer, sizeof(buffer));
    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < sizeof(buffer); ++i)
    {
        token[i * 2] = hex[(buffer[i] >> 4) & 0x0F];
        token[i * 2 + 1] = hex[buffer[i] & 0x0F];
    }
    token[ADMIN_PORTAL_TOKEN_MAX_LEN] = '\0';
}

static void set_cache_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
}


static size_t escape_js_string(const char *src, char *dest, size_t dest_size)
{
    if (!dest || dest_size == 0)
        return 0;

    size_t di = 0;
    const unsigned char *input = (const unsigned char *)(src ? src : "");
    while (*input && di + 1 < dest_size)
    {
        const char *replacement = NULL;
        char buffer[8];

        switch (*input)
        {
            case '\\':
                buffer[0] = '\\';
                buffer[1] = '\\';
                buffer[2] = '\0';
                replacement = buffer;
                break;
            case '"':
                replacement = "\\\"";
                break;
            case '\b':
                replacement = "\b";
                break;
            case '\f':
                replacement = "\f";
                break;
            case '\n':
                replacement = "\n";
                break;
            case '\r':
                replacement = "\r";
                break;
            case '\t':
                replacement = "\t";
                break;
            case '<':
                replacement = "\\u003C";
                break;
            case '>':
                replacement = "\\u003E";
                break;
            case '&':
                replacement = "\\u0026";
                break;
            case 0x27:
                replacement = "\\u0027";
                break;
            default:
                if (*input < 0x20)
                {
                    snprintf(buffer, sizeof(buffer), "\\u%04X", (unsigned)*input);
                    replacement = buffer;
                }
                break;
        }

        if (replacement)
        {
            size_t len = strlen(replacement);
            if (di + len >= dest_size)
                break;
            memcpy(dest + di, replacement, len);
            di += len;
        }
        else
        {
            dest[di++] = (char)*input;
        }

        ++input;
    }

    dest[di] = '\0';
    return di;
}

static void set_session_cookie(httpd_req_t *req, const char *token, uint32_t max_age_seconds)
{
    char header[128];
    if (token && token[0])
    {
        snprintf(header, sizeof(header), ADMIN_PORTAL_COOKIE_NAME "=%s; Max-Age=%" PRIu32 "; Path=/; HttpOnly; SameSite=Lax", token, max_age_seconds);
    }
    else
    {
        snprintf(header, sizeof(header), ADMIN_PORTAL_COOKIE_NAME "=deleted; Max-Age=0; Path=/; HttpOnly; SameSite=Lax");
    }
    httpd_resp_set_hdr(req, "Set-Cookie", header);
}

static bool parse_cookie_value(const char *cookies, const char *name, char *value, size_t value_size)
{
    if (!cookies || !name || !value || value_size == 0)
        return false;

    size_t name_len = strlen(name);
    const char *ptr = cookies;
    while (ptr && *ptr)
    {
        while (*ptr == ' ')
            ++ptr;

        if (strncmp(ptr, name, name_len) == 0 && ptr[name_len] == '=')
        {
            const char *start = ptr + name_len + 1;
            const char *end = strchr(start, ';');
            if (!end)
                end = start + strlen(start);
            size_t length = (size_t)(end - start);
            if (length >= value_size)
                length = value_size - 1;
            memcpy(value, start, length);
            value[length] = '\0';
            return true;
        }

        ptr = strchr(ptr, ';');
        if (ptr)
            ++ptr;
    }
    return false;
}

static bool get_session_token(httpd_req_t *req, char *token, size_t token_size)
{
    if (!req || !token || token_size == 0)
        return false;

    size_t length = httpd_req_get_hdr_value_len(req, "Cookie");
    if (length == 0)
        return false;

    char *buffer = (char *)malloc(length + 1);
    if (!buffer)
        return false;

    if (httpd_req_get_hdr_value_str(req, "Cookie", buffer, length + 1) != ESP_OK)
    {
        free(buffer);
        return false;
    }

    bool found = parse_cookie_value(buffer, ADMIN_PORTAL_COOKIE_NAME, token, token_size);
    free(buffer);
    return found;
}

static void load_initial_state(void)
{
    uint32_t stored_minutes = DEFAULT_IDLE_TIMEOUT_MINUTES;
    if (nvm_read_u32(NVM_WIFI_PARTITION, ADMIN_PORTAL_NAMESPACE, ADMIN_PORTAL_KEY_IDLE_MIN, &stored_minutes) != ESP_OK ||
        stored_minutes == 0)
    {
        stored_minutes = DEFAULT_IDLE_TIMEOUT_MINUTES;
    }

    admin_portal_state_init(&g_state, (uint32_t)minutes_to_ms(stored_minutes), WPA2_MINIMUM_PASSWORD_LENGTH);

    char ssid[sizeof(g_state.ap_ssid)];
    if (nvm_read_string(NVM_WIFI_PARTITION, ADMIN_PORTAL_NAMESPACE, ADMIN_PORTAL_KEY_SSID, ssid, sizeof(ssid)) != ESP_OK ||
        ssid[0] == '\0')
    {
        admin_portal_device_get_ap_ssid(ssid, sizeof(ssid));
    }
    admin_portal_state_set_ssid(&g_state, ssid);

    char password[sizeof(g_state.ap_password)];
    if (nvm_read_string(NVM_WIFI_PARTITION, ADMIN_PORTAL_NAMESPACE, ADMIN_PORTAL_KEY_PSW, password, sizeof(password)) !=
        ESP_OK)
    {
        password[0] = '\0';
    }
    admin_portal_state_set_password(&g_state, password);
    if (admin_portal_state_has_password(&g_state))
        admin_portal_device_set_ap_password(password);

    LOGI(TAG,
         "Initial state loaded: SSID=\"%s\", AP PSW=%s, idle timeout=%" PRIu32 " ms",
         admin_portal_state_get_ssid(&g_state),
         admin_portal_state_has_password(&g_state) ? "loaded" : "empty",
         g_state.inactivity_timeout_ms);
#ifdef DIAGNOSTIC_VERSION
    LOGI(TAG, "AP PASSWORD: \"%s\"", password);
#endif
}

static esp_err_t send_asset(httpd_req_t *req, const admin_portal_asset_t *asset)
{
    if (!req || !asset)
        return ESP_ERR_INVALID_ARG;

    set_cache_headers(req);
    httpd_resp_set_type(req, asset->content_type);
    if (asset->compressed)
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");

    size_t length = (size_t)(asset->end - asset->start);
    return httpd_resp_send(req, (const char *)asset->start, length);
}


static esp_err_t send_initial_data_script(httpd_req_t *req)
{
    const char *ssid = admin_portal_state_get_ssid(&g_state);
    bool has_password = admin_portal_state_has_password(&g_state);
    bool authorized = admin_portal_state_session_authorized(&g_state);

    char escaped_ssid[sizeof(g_state.ap_ssid) * 6 + 1];
    escape_js_string(ssid, escaped_ssid, sizeof(escaped_ssid));

    char script[512];
    snprintf(script,
             sizeof(script),
             "window.TAPGATE_INITIAL_DATA={ap_ssid:\"%s\",has_password:%s,authorized:%s};",
             escaped_ssid,
             has_password ? "true" : "false",
             authorized ? "true" : "false");

    httpd_resp_set_type(req, "application/javascript");
    set_cache_headers(req);
    return httpd_resp_send(req, script, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t handle_initial_data(httpd_req_t *req)
{
    return send_initial_data_script(req);
}

static esp_err_t send_page_content(httpd_req_t *req, const admin_portal_page_descriptor_t *desc)
{
    if (!req || !desc)
        return ESP_ERR_INVALID_ARG;

    set_cache_headers(req);
    httpd_resp_set_type(req, desc->content_type);
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    size_t length = (size_t)(desc->asset_end - desc->asset_start);
    return httpd_resp_send(req, (const char *)desc->asset_start, length);
}

static esp_err_t send_redirect(httpd_req_t *req, admin_portal_page_t page)
{
    const char *location = admin_portal_state_page_route(page);
    if (!location)
        location = "/";
    LOGI(TAG,
         "Redirecting request for %s to %s (page=%s)",
         req ? req->uri : "<null>",
         location,
         page_name(page));
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", location);
    set_cache_headers(req);
    return httpd_resp_send(req, "", 0);
}

static esp_err_t send_json(httpd_req_t *req, const char *status_text, const char *body)
{
    httpd_resp_set_status(req, status_text);
    httpd_resp_set_type(req, "application/json");
    set_cache_headers(req);
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

static admin_portal_session_status_t evaluate_session(httpd_req_t *req,
                                                      char *token_buffer,
                                                      size_t token_size)
{
    bool has_token = get_session_token(req, token_buffer, token_size);
    uint64_t now = get_now_ms();
    admin_portal_session_status_t status = admin_portal_state_check_session(&g_state, has_token ? token_buffer : NULL, now);
    if (status == ADMIN_PORTAL_SESSION_MATCH)
    {
        admin_portal_state_update_session(&g_state, now);
    }
    return status;
}

static esp_err_t ensure_session_claim(httpd_req_t *req,
                                      admin_portal_session_status_t *status,
                                      char *token_buffer,
                                      size_t token_size)
{
    if (!status || !token_buffer)
        return ESP_ERR_INVALID_ARG;

    if (*status != ADMIN_PORTAL_SESSION_NONE)
        return ESP_OK;

    char new_token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1];
    generate_session_token(new_token);
    admin_portal_state_start_session(&g_state, new_token, get_now_ms(), false);
    uint32_t max_age = g_state.inactivity_timeout_ms ? (uint32_t)(g_state.inactivity_timeout_ms / 1000UL) : 60U;
    LOGI(TAG,
         "Session started for URI %s (max_age=%" PRIu32 " s)",
         req ? req->uri : "<null>",
         max_age);
    set_session_cookie(req, new_token, max_age);
    strncpy(token_buffer, new_token, token_size - 1);
    token_buffer[token_size - 1] = '\0';
    *status = ADMIN_PORTAL_SESSION_MATCH;
    return ESP_OK;
}

static size_t url_decode(char *dest, size_t dest_size, const char *src, size_t src_len)
{
    if (!dest || dest_size == 0)
        return 0;

    size_t di = 0;
    for (size_t si = 0; si < src_len && di + 1 < dest_size; ++si)
    {
        char ch = src[si];
        if (ch == '+')
        {
            dest[di++] = ' ';
        }
        else if (ch == '%' && si + 2 < src_len)
        {
            char hi = src[si + 1];
            char lo = src[si + 2];
            int value = 0;
            if (hi >= '0' && hi <= '9')
                value = (hi - '0') << 4;
            else if (hi >= 'a' && hi <= 'f')
                value = (hi - 'a' + 10) << 4;
            else if (hi >= 'A' && hi <= 'F')
                value = (hi - 'A' + 10) << 4;
            else
                value = -1;

            if (lo >= '0' && lo <= '9')
                value |= (lo - '0');
            else if (lo >= 'a' && lo <= 'f')
                value |= (lo - 'a' + 10);
            else if (lo >= 'A' && lo <= 'F')
                value |= (lo - 'A' + 10);
            else
                value = -1;

            if (value >= 0)
            {
                dest[di++] = (char)value;
                si += 2;
            }
        }
        else
        {
            dest[di++] = ch;
        }
    }
    dest[di] = '\0';
    return di;
}

static bool form_get_field(const char *body, const char *key, char *value, size_t value_size)
{
    if (!body || !key || !value || value_size == 0)
        return false;

    size_t key_len = strlen(key);
    const char *ptr = body;
    while (*ptr)
    {
        while (*ptr == '&')
            ++ptr;

        const char *eq = strchr(ptr, '=');
        if (!eq)
            break;

        size_t field_len = (size_t)(eq - ptr);
        const char *value_start = eq + 1;
        const char *next = strchr(value_start, '&');
        size_t value_len = next ? (size_t)(next - value_start) : strlen(value_start);

        if (field_len == key_len && strncmp(ptr, key, key_len) == 0)
        {
            url_decode(value, value_size, value_start, value_len);
            return true;
        }

        if (!next)
            break;
        ptr = next + 1;
    }
    return false;
}

static char *read_body(httpd_req_t *req)
{
    if (!req)
        return NULL;

    size_t total = req->content_len;
    char *buffer = (char *)malloc(total + 1);
    if (!buffer)
        return NULL;

    size_t received = 0;
    while (received < total)
    {
        int len = httpd_req_recv(req, buffer + received, total - received);
        if (len <= 0)
        {
            free(buffer);
            return NULL;
        }
        received += (size_t)len;
    }
    buffer[received] = '\0';
    return buffer;
}

static esp_err_t handle_session_info(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));

    LOGI(TAG,
         "handle_session_info: API /api/session: status=%s authorized=%s claimed=%s",
         session_status_name(status),
         admin_portal_state_session_authorized(&g_state) ? "true" : "false",
         g_state.session.claimed ? "true" : "false");

    if (status == ADMIN_PORTAL_SESSION_BUSY)
    {
        LOGI(TAG, "handle_session_info: API /api/session response: busy");
        return send_json(req, "409 Conflict", "{\"status\":\"busy\"}");
    }

    if (status == ADMIN_PORTAL_SESSION_EXPIRED)
    {
        admin_portal_state_clear_session(&g_state);
        set_session_cookie(req, NULL, 0);
        LOGI(TAG, "handle_session_info: Session expired, redirecting to off page");
        return send_json(req, "200 OK", "{\"status\":\"expired\",\"redirect\":\"/off/\"}");
    }

    if (status == ADMIN_PORTAL_SESSION_NONE)
    {
        ensure_session_claim(req, &status, token, sizeof(token));
        LOGI(TAG, "handle_session_info: API /api/session issued new token after claim");
    }

    bool authorized = admin_portal_state_session_authorized(&g_state);
    bool has_password = admin_portal_state_has_password(&g_state);

    char response[384];
    snprintf(response, sizeof(response),
             "{\"status\":\"ok\",\"authorized\":%s,\"has_password\":%s,\"ap_ssid\":\"%s\"}",
             authorized ? "true" : "false",
             has_password ? "true" : "false",
             admin_portal_state_get_ssid(&g_state));
    LOGI(TAG, "handle_session_info: API /api/session response: authorized=%s, has_password=%s, AP SSID=%s",
                authorized ? "true" : "false",
                has_password ? "true" : "false",
                admin_portal_state_get_ssid(&g_state));
    return send_json(req, "200 OK", response);
}

static esp_err_t handle_enroll(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));

    LOGI(TAG,
         "API /api/enroll: status=%s has_password=%s",
         session_status_name(status),
         admin_portal_state_has_password(&g_state) ? "true" : "false");

    if (status != ADMIN_PORTAL_SESSION_MATCH)
    {
        if (status == ADMIN_PORTAL_SESSION_NONE)
            ensure_session_claim(req, &status, token, sizeof(token));
        if (status == ADMIN_PORTAL_SESSION_BUSY)
        {
            LOGI(TAG, "Enrollment denied: portal busy");
            return send_json(req, "409 Conflict", "{\"status\":\"busy\"}");
        }
    }

    if (admin_portal_state_has_password(&g_state))
    {
        LOGI(TAG, "Enrollment redirected to auth because password already set");
        return send_json(req, "200 OK", "{\"status\":\"redirect\",\"redirect\":\"/auth/\"}");
    }

    char *body = read_body(req);
    if (!body)
    {
        LOGI(TAG, "Enrollment failed: request body missing");
        return send_json(req, "400 Bad Request", "{\"status\":\"error\",\"code\":\"invalid_request\"}");
    }

    char portal_name[sizeof(g_state.ap_ssid)];
    char password[sizeof(g_state.ap_password)];
    bool has_portal_name = form_get_field(body, "portal", portal_name, sizeof(portal_name));
    bool has_password = form_get_field(body, "password", password, sizeof(password));
    free(body);

    if (!has_portal_name || portal_name[0] == '\0')
    {
        LOGI(TAG, "Enrollment failed: portal name missing");
        return send_json(req, "200 OK", "{\"status\":\"error\",\"code\":\"invalid_ssid\"}");
    }

    if (!has_password || !admin_portal_state_password_valid(&g_state, password))
    {
        LOGI(TAG,
             "Enrollment failed: %s",
             has_password ? "password does not meet requirements" : "password missing");
        return send_json(req, "200 OK", "{\"status\":\"error\",\"code\":\"invalid_password\"}");
    }

    esp_err_t err = nvm_write_string(NVM_WIFI_PARTITION, ADMIN_PORTAL_NAMESPACE, ADMIN_PORTAL_KEY_SSID, portal_name);
    if (err != ESP_OK)
    {
        LOGI(TAG, "Enrollment failed: unable to store AP SSID: %s", esp_err_to_name(err));
        return send_json(req, "500 Internal Server Error", "{\"status\":\"error\",\"code\":\"storage_failed\"}");
    }

    err = nvm_write_string(NVM_WIFI_PARTITION, ADMIN_PORTAL_NAMESPACE, ADMIN_PORTAL_KEY_PSW, password);
    if (err != ESP_OK)
    {
        LOGI(TAG, "Enrollment failed: unable to store password: %s", esp_err_to_name(err));
        return send_json(req, "500 Internal Server Error", "{\"status\":\"error\",\"code\":\"storage_failed\"}");
    }

    admin_portal_state_set_ssid(&g_state, portal_name);
    admin_portal_state_set_password(&g_state, password);
    admin_portal_state_authorize_session(&g_state);
    admin_portal_device_set_ap_password(password);

    LOGI(TAG, "Enrollment successful, redirecting to main page (AP SSID=\"%s\")", portal_name);
    
    // Ensure we have a valid session token for enrollment
    ensure_session_claim(req, &status, token, sizeof(token));
    
    // Set the session cookie
    set_session_cookie(req, token, DEFAULT_SESSION_MAX_AGE);
    
    // Then send HTTP redirect
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/main/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t send_success_with_session(httpd_req_t *req, const char* token, const char* redirect_to) {
    set_session_cookie(req, token, DEFAULT_SESSION_MAX_AGE);
    char response[256];
    snprintf(response, sizeof(response), "{\"status\":\"ok\",\"redirect\":\"%s\"}", redirect_to);
    return send_json(req, "200 OK", response);
}

static esp_err_t handle_login(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));

    LOGI(TAG,
         "API /api/login: status=%s has_password=%s authorized=%s",
         session_status_name(status),
         admin_portal_state_has_password(&g_state) ? "true" : "false",
         admin_portal_state_session_authorized(&g_state) ? "true" : "false");

    if (status == ADMIN_PORTAL_SESSION_BUSY)
    {
        LOGI(TAG, "Login denied: portal busy");
        return send_json(req, "409 Conflict", "{\"status\":\"busy\"}");
    }

    if (status == ADMIN_PORTAL_SESSION_NONE)
    {
        ensure_session_claim(req, &status, token, sizeof(token));
        LOGI(TAG, "Login request claimed new session");
    }

    if (!admin_portal_state_has_password(&g_state))
    {
        LOGI(TAG, "Login redirected to enroll because password missing");
        return send_json(req, "200 OK", "{\"status\":\"redirect\",\"redirect\":\"/enroll/\"}");
    }

    char *body = read_body(req);
    if (!body)
    {
        LOGI(TAG, "Login failed: request body missing");
        return send_json(req, "400 Bad Request", "{\"status\":\"error\",\"code\":\"invalid_request\"}");
    }

    char password[sizeof(g_state.ap_password)];
    bool has_password = form_get_field(body, "password", password, sizeof(password));
    free(body);

    if (!has_password || !admin_portal_state_password_matches(&g_state, password))
    {
        LOGI(TAG, "Login failed: wrong password");
        return send_json(req, "200 OK", "{\"status\":\"error\",\"code\":\"wrong_password\"}");
    }

    admin_portal_state_authorize_session(&g_state);
    admin_portal_state_update_session(&g_state, get_now_ms());
    LOGI(TAG, "Login successful, sending response with redirect to main page");
    return send_success_with_session(req, token, "/main/");
    ensure_session_claim(req, &status, token, sizeof(token));
    
    admin_portal_state_authorize_session(&g_state);
    LOGI(TAG, "Login successful, redirecting to main page");
    
    // Set the session cookie
    set_session_cookie(req, token, DEFAULT_SESSION_MAX_AGE);
    
    // Then send HTTP redirect
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/main/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t handle_change_password(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));
    LOGI(TAG,
         "API /api/change-password: status=%s authorized=%s",
         session_status_name(status),
         admin_portal_state_session_authorized(&g_state) ? "true" : "false");

    if (status == ADMIN_PORTAL_SESSION_BUSY)
    {
        LOGI(TAG, "Change password denied: portal busy");
        return send_json(req, "409 Conflict", "{\"status\":\"busy\"}");
    }
    if (status != ADMIN_PORTAL_SESSION_MATCH || !admin_portal_state_session_authorized(&g_state))
    {
        LOGI(TAG, "Change password denied: unauthorized session");
        return send_json(req, "403 Forbidden", "{\"status\":\"error\",\"code\":\"unauthorized\"}");
    }

    char *body = read_body(req);
    if (!body)
    {
        LOGI(TAG, "Change password failed: request body missing");
        return send_json(req, "400 Bad Request", "{\"status\":\"error\",\"code\":\"invalid_request\"}");
    }

    char current[sizeof(g_state.ap_password)];
    char next[sizeof(g_state.ap_password)];
    bool has_current = form_get_field(body, "current", current, sizeof(current));
    bool has_next = form_get_field(body, "next", next, sizeof(next));
    free(body);

    if (!has_current || !admin_portal_state_password_matches(&g_state, current))
    {
        LOGI(TAG, "Change password failed: wrong current password");
        return send_json(req, "200 OK", "{\"status\":\"error\",\"code\":\"wrong_password\"}");
    }

    if (!has_next || !admin_portal_state_password_valid(&g_state, next))
    {
        LOGI(TAG,
             "Change password failed: %s",
             has_next ? "new password does not meet requirements" : "new password missing");
        return send_json(req, "200 OK", "{\"status\":\"error\",\"code\":\"invalid_new_password\"}");
    }

    esp_err_t err = nvm_write_string(NVM_WIFI_PARTITION, ADMIN_PORTAL_NAMESPACE, ADMIN_PORTAL_KEY_PSW, next);
    if (err != ESP_OK)
    {
        LOGI(TAG, "Change password failed: unable to store password (err=0x%x)", (unsigned)err);
        return send_json(req, "500 Internal Server Error", "{\"status\":\"error\",\"code\":\"storage_failed\"}");
    }

    admin_portal_state_set_password(&g_state, next);
    admin_portal_state_authorize_session(&g_state);
    admin_portal_device_set_ap_password(next);

    LOGI(TAG, "Change password successful, redirecting to device page");
    return send_json(req, "200 OK", "{\"status\":\"ok\",\"redirect\":\"/device/\"}");
}

static esp_err_t handle_update_device(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));
    LOGI(TAG,
         "API /api/device: status=%s authorized=%s",
         session_status_name(status),
         admin_portal_state_session_authorized(&g_state) ? "true" : "false");

    if (status == ADMIN_PORTAL_SESSION_BUSY)
    {
        LOGI(TAG, "Device update denied: portal busy");
        return send_json(req, "409 Conflict", "{\"status\":\"busy\"}");
    }
    if (status != ADMIN_PORTAL_SESSION_MATCH || !admin_portal_state_session_authorized(&g_state))
    {
        LOGI(TAG, "Device update denied: unauthorized session");
        return send_json(req, "403 Forbidden", "{\"status\":\"error\",\"code\":\"unauthorized\"}");
    }

    char *body = read_body(req);
    if (!body)
    {
        LOGI(TAG, "Device update failed: request body missing");
        return send_json(req, "400 Bad Request", "{\"status\":\"error\",\"code\":\"invalid_request\"}");
    }

    char ssid[sizeof(g_state.ap_ssid)];
    bool has_ssid = form_get_field(body, "ssid", ssid, sizeof(ssid));
    free(body);

    if (!has_ssid || ssid[0] == '\0')
    {
        LOGI(TAG, "Device update failed: invalid SSID");
        return send_json(req, "200 OK", "{\"status\":\"error\",\"code\":\"invalid_ssid\"}");
    }

    esp_err_t err = nvm_write_string(NVM_WIFI_PARTITION, ADMIN_PORTAL_NAMESPACE, ADMIN_PORTAL_KEY_SSID, ssid);
    if (err != ESP_OK)
    {
        LOGI(TAG, "Device update failed: unable to store SSID (err=0x%x)", (unsigned)err);
        return send_json(req, "500 Internal Server Error", "{\"status\":\"error\",\"code\":\"storage_failed\"}");
    }

    admin_portal_state_set_ssid(&g_state, ssid);
    admin_portal_device_set_ap_ssid(ssid);

    LOGI(TAG, "Device update successful, redirecting to main page (ssid=\"%s\")", ssid);
    return send_json(req, "200 OK", "{\"status\":\"ok\",\"redirect\":\"/main/\"}");
}

static esp_err_t handle_page_request(httpd_req_t *req, const admin_portal_page_descriptor_t *desc)
{
    if (!desc)
        return ESP_ERR_INVALID_ARG;

    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));
    admin_portal_page_t target = admin_portal_state_resolve_page(&g_state, desc->page, status);
    LOGI(TAG, "Handle page request resolve page: %s", admin_portal_page_to_str(target));

    if (!admin_portal_state_has_password(&g_state) &&
        target != ADMIN_PORTAL_PAGE_ENROLL &&
        target != ADMIN_PORTAL_PAGE_BUSY &&
        target != ADMIN_PORTAL_PAGE_OFF)
    {
        LOGI(TAG, "Password missing, forcing redirect to enroll page");
        target = ADMIN_PORTAL_PAGE_ENROLL;
    }

    LOGI(TAG,
         "GET %s: session=%s authorized=%s resolved=%s",
         desc->route,
         session_status_name(status),
         admin_portal_state_session_authorized(&g_state) ? "true" : "false",
         page_name(target));

    if (target == ADMIN_PORTAL_PAGE_OFF && status == ADMIN_PORTAL_SESSION_EXPIRED)
    {
        admin_portal_state_clear_session(&g_state);
        set_session_cookie(req, NULL, 0);
        LOGI(TAG, "Session expired while requesting %s; clearing cookie", desc->route);
    }

    // First ensure we have a valid session
    if (status == ADMIN_PORTAL_SESSION_NONE && desc->page != ADMIN_PORTAL_PAGE_BUSY && desc->page != ADMIN_PORTAL_PAGE_OFF) {
        ensure_session_claim(req, &status, token, sizeof(token));
    }

    // Then check if we need to redirect
    if (target != desc->page) {
        set_session_cookie(req, token, DEFAULT_SESSION_MAX_AGE);  // Ensure cookie is set before redirect
        return send_redirect(req, target);
    }

    LOGI(TAG, "Serving page %s", page_name(desc->page));
    return send_page_content(req, desc);
}

static esp_err_t handle_root(httpd_req_t *req)
{
    const admin_portal_page_descriptor_t *main_page = &admin_portal_page_main;
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));
    admin_portal_page_t target = admin_portal_state_resolve_page(&g_state, main_page->page, status);
    LOGI(TAG, "Handle root resolve page: %s", admin_portal_page_to_str(target));

    if (!admin_portal_state_has_password(&g_state) &&
        target != ADMIN_PORTAL_PAGE_ENROLL &&
        target != ADMIN_PORTAL_PAGE_BUSY &&
        target != ADMIN_PORTAL_PAGE_OFF)
    {
        LOGI(TAG, "Password missing on root request, redirecting to enroll");
        target = ADMIN_PORTAL_PAGE_ENROLL;
    }

    LOGI(TAG,
         "GET /: session=%s authorized=%s resolved=%s",
         session_status_name(status),
         admin_portal_state_session_authorized(&g_state) ? "true" : "false",
         page_name(target));

    if (target == ADMIN_PORTAL_PAGE_OFF && status == ADMIN_PORTAL_SESSION_EXPIRED)
    {
        admin_portal_state_clear_session(&g_state);
        set_session_cookie(req, NULL, 0);
        LOGI(TAG, "Session expired on root request; clearing cookie");
    }

    return send_redirect(req, target);
}

static esp_err_t handle_page_entry(httpd_req_t *req)
{
    const admin_portal_page_descriptor_t *desc = req->user_ctx;
    if (!desc)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Page not configured");

    return handle_page_request(req, desc);
}

static esp_err_t handle_asset_entry(httpd_req_t *req)
{
    const admin_portal_asset_t *asset = req->user_ctx;
    if (!asset)
        return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Asset not configured");

    LOGI(TAG, "Serving asset %s", asset->uri);
    return send_asset(req, asset);
}

esp_err_t admin_portal_http_service_start(httpd_handle_t server)
{
    if (!server)
        return ESP_ERR_INVALID_ARG;

    g_server = server;
    load_initial_state();

    httpd_uri_t root_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = handle_root,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_get));

    for (size_t i = 0; i < sizeof(g_pages) / sizeof(g_pages[0]); ++i)
    {
        const admin_portal_page_descriptor_t *desc = g_pages[i];
        httpd_uri_t page_uri = {
            .uri = desc->route,
            .method = HTTP_GET,
            .handler = handle_page_entry,
            .user_ctx = (void *)desc,
        };
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &page_uri));

        if (desc->alternate_route)
        {
            httpd_uri_t alt_uri = page_uri;
            alt_uri.uri = desc->alternate_route;
            ESP_ERROR_CHECK(httpd_register_uri_handler(server, &alt_uri));
        }
    }

    for (size_t i = 0; i < sizeof(g_assets) / sizeof(g_assets[0]); ++i)
    {
        const admin_portal_asset_t *asset = &g_assets[i];
        httpd_uri_t asset_uri = {
            .uri = asset->uri,
            .method = HTTP_GET,
            .handler = handle_asset_entry,
            .user_ctx = (void *)asset,
        };
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &asset_uri));
    }


    httpd_uri_t initial_data = {
        .uri = "/api/initial-data.js",
        .method = HTTP_GET,
        .handler = handle_initial_data,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &initial_data));

    httpd_uri_t api_session = {
        .uri = "/api/session",
        .method = HTTP_GET,
        .handler = handle_session_info,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &api_session));

    httpd_uri_t api_enroll = {
        .uri = "/api/enroll",
        .method = HTTP_POST,
        .handler = handle_enroll,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &api_enroll));

    httpd_uri_t api_login = {
        .uri = "/api/login",
        .method = HTTP_POST,
        .handler = handle_login,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &api_login));

    httpd_uri_t api_change = {
        .uri = "/api/change-password",
        .method = HTTP_POST,
        .handler = handle_change_password,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &api_change));

    httpd_uri_t api_device = {
        .uri = "/api/device",
        .method = HTTP_POST,
        .handler = handle_update_device,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &api_device));

    LOGI(TAG, "Admin portal service registered");
    return ESP_OK;
}

void admin_portal_http_service_stop(void)
{
    if (!g_server)
        return;

    g_server = NULL;
    admin_portal_state_clear_session(&g_state);
}

