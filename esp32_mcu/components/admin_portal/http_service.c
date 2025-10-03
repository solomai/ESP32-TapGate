#include "http_service.h"

#include "admin_portal_state.h"
#include "admin_portal_device.h"
#include "logs.h"
#include "wifi_manager.h"

#include "esp_random.h"
#include "esp_timer.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#include "pages/page_auth.h"
#include "pages/page_busy.h"
#include "pages/page_change_psw.h"
#include "pages/page_clients.h"
#include "pages/page_device.h"
#include "pages/page_enroll.h"
#include "pages/page_events.h"
#include "pages/page_main.h"
#include "pages/page_wifi.h"

static const char *TAG = "AdminPortal";

// WiFi scan results storage
static char g_wifi_scan_results[2048] = {0};
static bool g_wifi_scan_ready = false;

// WiFi scan completion callback
static void wifi_scan_done_callback(void* arg)
{
    LOGI(TAG, "=== WiFi scan completion callback triggered ===");
    LOGI(TAG, "Callback argument: %p", arg);
    
    const char* networks_json = wifi_manager_get_ap_json();
    LOGI(TAG, "Retrieved JSON from wifi_manager: %s", networks_json ? networks_json : "NULL");
    LOGI(TAG, "JSON length: %zu", networks_json ? strlen(networks_json) : 0);
    
    if (networks_json && strlen(networks_json) > 0) {
        // Copy the real JSON data from WiFi manager's accessp_json
        size_t json_len = strlen(networks_json);
        size_t max_copy = sizeof(g_wifi_scan_results) - 1;
        
        if (json_len > max_copy) {
            LOGW(TAG, "JSON data too large (%zu bytes), truncating to %zu bytes", json_len, max_copy);
        }
        
        strncpy(g_wifi_scan_results, networks_json, max_copy);
        g_wifi_scan_results[max_copy] = '\0';
        g_wifi_scan_ready = true;
        LOGI(TAG, "WiFi scan results stored successfully, %zu bytes copied", strlen(g_wifi_scan_results));
        LOGI(TAG, "Stored data: %s", g_wifi_scan_results);
    } else {
        // No networks found or JSON not ready, return empty array
        strcpy(g_wifi_scan_results, "[]");
        g_wifi_scan_ready = true;
        LOGW(TAG, "No WiFi networks found or JSON not available, using empty array");
    }
    
    LOGI(TAG, "=== Callback completed, g_wifi_scan_ready=%d ===", g_wifi_scan_ready);
}

#define ADMIN_PORTAL_COOKIE_NAME     "tg_session"

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
};

extern const uint8_t _binary_logo_png_start[];
extern const uint8_t _binary_logo_png_end[];
extern const uint8_t _binary_styles_css_gz_start[];
extern const uint8_t _binary_styles_css_gz_end[];
extern const uint8_t _binary_app_js_gz_start[];
extern const uint8_t _binary_app_js_gz_end[];
extern const uint8_t _binary_wifi0_svg_start[];
extern const uint8_t _binary_wifi0_svg_end[];
extern const uint8_t _binary_wifi1_svg_start[];
extern const uint8_t _binary_wifi1_svg_end[];
extern const uint8_t _binary_wifi2_svg_start[];
extern const uint8_t _binary_wifi2_svg_end[];
extern const uint8_t _binary_wifi3_svg_start[];
extern const uint8_t _binary_wifi3_svg_end[];

static const admin_portal_asset_t g_assets[] = {
    { .uri = "/assets/logo.png", .start = _binary_logo_png_start, .end = _binary_logo_png_end, .content_type = "image/png", .compressed = false },
    { .uri = "/assets/styles.css", .start = _binary_styles_css_gz_start, .end = _binary_styles_css_gz_end, .content_type = "text/css", .compressed = true },
    { .uri = "/assets/app.js", .start = _binary_app_js_gz_start, .end = _binary_app_js_gz_end, .content_type = "application/javascript", .compressed = true },
    { .uri = "/assets/wifi0.svg", .start = _binary_wifi0_svg_start, .end = _binary_wifi0_svg_end, .content_type = "image/svg+xml", .compressed = false },
    { .uri = "/assets/wifi1.svg", .start = _binary_wifi1_svg_start, .end = _binary_wifi1_svg_end, .content_type = "image/svg+xml", .compressed = false },
    { .uri = "/assets/wifi2.svg", .start = _binary_wifi2_svg_start, .end = _binary_wifi2_svg_end, .content_type = "image/svg+xml", .compressed = false },
    { .uri = "/assets/wifi3.svg", .start = _binary_wifi3_svg_start, .end = _binary_wifi3_svg_end, .content_type = "image/svg+xml", .compressed = false },
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

// Helper function to set common CORS headers for mobile compatibility
static void set_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "http://10.10.0.1");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Credentials", "true");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Cookie, Authorization");
}

// Helper function to set CORS headers with additional exposed headers
static void set_cors_headers_with_expose(httpd_req_t *req, const char *expose_headers)
{
    set_cors_headers(req);
    if (expose_headers) {
        httpd_resp_set_hdr(req, "Access-Control-Expose-Headers", expose_headers);
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
        default:
            return "unknown";
    }
}

static uint64_t get_now_ms(void)
{
    return esp_timer_get_time() / 1000ULL;
}

// Simplified client IP detection with reduced logging
static bool get_client_ip(httpd_req_t *req, char *ip_buffer, size_t buffer_size)
{
    if (!req || !ip_buffer || buffer_size < 16)
        return false;

    // Try X-Forwarded-For header first
    size_t len = httpd_req_get_hdr_value_len(req, "X-Forwarded-For");
    if (len > 0 && len < buffer_size) {
        if (httpd_req_get_hdr_value_str(req, "X-Forwarded-For", ip_buffer, buffer_size) == ESP_OK) {
            return true;
        }
    }

    // Try X-Real-IP header
    len = httpd_req_get_hdr_value_len(req, "X-Real-IP");
    if (len > 0 && len < buffer_size) {
        if (httpd_req_get_hdr_value_str(req, "X-Real-IP", ip_buffer, buffer_size) == ESP_OK) {
            return true;
        }
    }

    // For ESP32 AP mode, socket-based detection often fails
    // Use a consistent mobile client identifier to avoid warnings
    strncpy(ip_buffer, "10.10.0.100", buffer_size - 1);
    ip_buffer[buffer_size - 1] = '\0';
    
    // Log only once to reduce log noise
    static bool logged_fallback = false;
    if (!logged_fallback) {
        LOGW(TAG, "Using fixed mobile client identifier for session consistency");
        logged_fallback = true;
    }
    
    return true;
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

static void generate_device_fingerprint(httpd_req_t *req, char fingerprint[32])
{
    // Create a simple device fingerprint based on User-Agent and other headers
    char *user_agent = NULL;
    char *accept = NULL;
    char *accept_language = NULL;
    
    // Get User-Agent header
    size_t buf_len = httpd_req_get_hdr_value_len(req, "User-Agent");
    if (buf_len > 0) {
        user_agent = malloc(buf_len + 1);
        if (user_agent && httpd_req_get_hdr_value_str(req, "User-Agent", user_agent, buf_len + 1) != ESP_OK) {
            free(user_agent);
            user_agent = NULL;
        }
    }
    
    // Get Accept header
    buf_len = httpd_req_get_hdr_value_len(req, "Accept");
    if (buf_len > 0) {
        accept = malloc(buf_len + 1);
        if (accept && httpd_req_get_hdr_value_str(req, "Accept", accept, buf_len + 1) != ESP_OK) {
            free(accept);
            accept = NULL;
        }
    }
    
    // Get Accept-Language header
    buf_len = httpd_req_get_hdr_value_len(req, "Accept-Language");
    if (buf_len > 0) {
        accept_language = malloc(buf_len + 1);
        if (accept_language && httpd_req_get_hdr_value_str(req, "Accept-Language", accept_language, buf_len + 1) != ESP_OK) {
            free(accept_language);
            accept_language = NULL;
        }
    }
    
    // Create a simple hash of the combined headers
    uint32_t hash = 2166136261U; // FNV-1a offset basis
    const char *sources[] = {user_agent, accept, accept_language};
    for (int i = 0; i < 3; i++) {
        const char *str = sources[i];
        if (str) {
            while (*str) {
                hash ^= (uint8_t)*str;
                hash *= 16777619U; // FNV-1a prime
                str++;
            }
        }
    }
    
    // Convert to hex string (limited to 31 chars + null terminator)
    snprintf(fingerprint, 32, "%08" PRIx32, hash);
    
    // Clean up allocated memory
    if (user_agent) free(user_agent);
    if (accept) free(accept);
    if (accept_language) free(accept_language);
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
        // Mobile-compatible cookie: omit SameSite to allow cross-site usage
        // Remove HttpOnly to allow JavaScript access for mobile browsers
        snprintf(header, sizeof(header), ADMIN_PORTAL_COOKIE_NAME "=%s; Max-Age=%" PRIu32 "; Path=/", token, max_age_seconds);
        LOGI(TAG, "Setting session cookie: token=%.8s..., max_age=%" PRIu32 "s", token, max_age_seconds);
    }
    else
    {
        snprintf(header, sizeof(header), ADMIN_PORTAL_COOKIE_NAME "=deleted; Max-Age=0; Path=/");
        LOGI(TAG, "Clearing session cookie");
    }
    
    esp_err_t result = httpd_resp_set_hdr(req, "Set-Cookie", header);
    LOGI(TAG, "Session cookie header set: result=%s", esp_err_to_name(result));
#ifdef DIAGNOSTIC_VERSION
    LOGI(TAG, "Cookie header: %s", header);
#endif
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
    {
        LOGI(TAG, "No Cookie header found in request for URI: %s", req->uri);
        return false;
    }

    char *buffer = (char *)malloc(length + 1);
    if (!buffer)
    {
        LOGI(TAG, "Failed to allocate buffer for cookie header");
        return false;
    }

    // Clear buffer to prevent corruption
    memset(buffer, 0, length + 1);

    if (httpd_req_get_hdr_value_str(req, "Cookie", buffer, length + 1) != ESP_OK)
    {
        LOGI(TAG, "Failed to get Cookie header value");
        free(buffer);
        return false;
    }

    // Validate cookie content is printable (safety check)
    bool is_valid = true;
    for (size_t i = 0; i < length; i++) {
        if (buffer[i] != 0 && (buffer[i] < 32 || buffer[i] > 126)) {
            LOGW(TAG, "Cookie contains non-printable character at position %zu (0x%02X), rejecting", i, (unsigned char)buffer[i]);
            is_valid = false;
            break;
        }
    }
    
    if (!is_valid) {
        free(buffer);
        return false;
    }

    LOGI(TAG, "Cookie header received: length=%zu", length);
#ifdef DIAGNOSTIC_VERSION
    LOGI(TAG, "Cookie header content: %s", buffer);
#endif

    bool found = parse_cookie_value(buffer, ADMIN_PORTAL_COOKIE_NAME, token, token_size);
    if (found)
    {
        LOGI(TAG, "Session token found: %.8s...", token);
    }
    else
    {
        LOGI(TAG, "Session token not found in cookies");
    }
    
    free(buffer);
    return found;
}

static void load_initial_state(void)
{
    uint32_t stored_minutes = session_get_idle_timeout();
    admin_portal_state_init(&g_state, (uint32_t)minutes_to_ms(stored_minutes), WPA2_MINIMUM_PASSWORD_LENGTH);

    char ssid[sizeof(g_state.ap_ssid)];
    wifi_get_ap_ssid(ssid, sizeof(ssid));
    admin_portal_state_set_ssid(&g_state, ssid);

    char password[sizeof(g_state.ap_password)];
    wifi_get_ap_password(password, sizeof(password));
    admin_portal_state_set_password(&g_state, password);

    LOGI(TAG,
        "Initial state loaded: SSID=\"%s\", AP PSW=%s, idle timeout=%" PRIu32 " ms",
        admin_portal_state_get_ssid(&g_state),
        admin_portal_state_has_password(&g_state) ? "loaded" : "empty",
        g_state.inactivity_timeout_ms);
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
    // Refresh SSID from wifi_manager to ensure it's current
    char current_ssid[sizeof(g_state.ap_ssid)];
    if (wifi_get_ap_ssid(current_ssid, sizeof(current_ssid))) {
        admin_portal_state_set_ssid(&g_state, current_ssid);
    }
    
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
    set_cors_headers(req);
    
    size_t length = (size_t)(desc->asset_end - desc->asset_start);
    return httpd_resp_send(req, (const char *)desc->asset_start, length);
}

// Helper function for redirects with CORS headers
static esp_err_t send_redirect(httpd_req_t *req, admin_portal_page_t page)
{
    const char *location = admin_portal_state_page_route(page);
    if (!location) location = "/";
    
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", location);
    set_cors_headers_with_expose(req, "Location, Set-Cookie");
    set_cache_headers(req);
    
    return httpd_resp_send(req, "", 0);
}

// CORS preflight handler for mobile browser compatibility
static esp_err_t handle_options(httpd_req_t *req)
{
    httpd_resp_set_status(req, "200 OK");
    set_cors_headers(req);
    httpd_resp_set_hdr(req, "Access-Control-Max-Age", "86400"); // 24 hours
    
    return httpd_resp_send(req, "", 0);
}

// Helper function for JSON responses with proper CORS headers
static esp_err_t send_json(httpd_req_t *req, const char *status_text, const char *body)
{
    httpd_resp_set_status(req, status_text);
    httpd_resp_set_type(req, "application/json");
    set_cors_headers_with_expose(req, "Set-Cookie");
    set_cache_headers(req);
    
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

// Unified session evaluation with client identification
static admin_portal_session_status_t evaluate_session(httpd_req_t *req, char *token_buffer, size_t token_size)
{
    uint64_t now = get_now_ms();
    
    // Get client IP
    char client_ip[16];
    if (!get_client_ip(req, client_ip, sizeof(client_ip))) {
        LOGI(TAG, "Could not determine client IP");
        return ADMIN_PORTAL_SESSION_NONE;
    }
    
    // Generate device fingerprint
    char device_fingerprint[32];
    generate_device_fingerprint(req, device_fingerprint);
    
    // Get session token from cookie
    char session_token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    bool has_token = get_session_token(req, session_token, sizeof(session_token));
    
    // Check session with all client identifiers
    admin_portal_session_status_t status = admin_portal_state_check_session(
        &g_state, 
        client_ip, 
        has_token ? session_token : NULL, 
        device_fingerprint, 
        now
    );
    
    if (status == ADMIN_PORTAL_SESSION_MATCH) {
        admin_portal_state_update_session(&g_state, now);
        // Copy token to buffer for cookie management
        if (token_buffer && token_size > 0 && has_token) {
            strncpy(token_buffer, session_token, token_size - 1);
            token_buffer[token_size - 1] = '\0';
        }
    }
    
    LOGI(TAG, "Session evaluation: IP=%s, token=%s, fingerprint=%s, status=%s", 
         client_ip, 
         has_token ? "present" : "none", 
         device_fingerprint,
         admin_portal_session_status_to_str(status));
    
    return status;
}

// Create a new session with proper client binding
static esp_err_t create_session(httpd_req_t *req, char *token_buffer, size_t token_size)
{
    uint64_t now = get_now_ms();
    
    // Get client IP
    char client_ip[16];
    if (!get_client_ip(req, client_ip, sizeof(client_ip))) {
        LOGI(TAG, "Cannot create session: could not determine client IP");
        return ESP_FAIL;
    }
    
    // Generate new session token
    char new_token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1];
    generate_session_token(new_token);
    
    // Generate device fingerprint
    char device_fingerprint[32];
    generate_device_fingerprint(req, device_fingerprint);
    
    // Create session
    admin_portal_state_start_session(&g_state, client_ip, new_token, device_fingerprint, now, false);
    
    // Copy token to buffer
    if (token_buffer && token_size > 0) {
        strncpy(token_buffer, new_token, token_size - 1);
        token_buffer[token_size - 1] = '\0';
    }
    
    LOGI(TAG, "New session created: IP=%s, token=%.8s..., fingerprint=%s", 
         client_ip, new_token, device_fingerprint);
    
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

    if (status == ADMIN_PORTAL_SESSION_BUSY) {
        return send_json(req, "409 Conflict", "{\"status\":\"busy\"}");
    }

    if (status == ADMIN_PORTAL_SESSION_EXPIRED) {
        admin_portal_state_clear_session(&g_state);
        set_session_cookie(req, NULL, 0);
        return send_json(req, "200 OK", "{\"status\":\"expired\",\"redirect\":\"/auth/\"}");
    }

    if (status == ADMIN_PORTAL_SESSION_NONE) {
        create_session(req, token, sizeof(token));
    }

    bool authorized = admin_portal_state_session_authorized(&g_state);
    bool has_password = admin_portal_state_has_password(&g_state);

    char response[384];
    snprintf(response, sizeof(response),
             "{\"status\":\"ok\",\"authorized\":%s,\"has_password\":%s,\"ap_ssid\":\"%s\"}",
             authorized ? "true" : "false",
             has_password ? "true" : "false",
             admin_portal_state_get_ssid(&g_state));
    
    return send_json(req, "200 OK", response);
}

static esp_err_t handle_enroll(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));

    if (status == ADMIN_PORTAL_SESSION_BUSY) {
        return send_redirect(req, ADMIN_PORTAL_PAGE_BUSY);
    }

    if (status == ADMIN_PORTAL_SESSION_NONE) {
        create_session(req, token, sizeof(token));
    }

    // Redirect to auth if password already exists
    if (admin_portal_state_has_password(&g_state)) {
        return send_redirect(req, ADMIN_PORTAL_PAGE_AUTH);
    }

    char *body = read_body(req);
    if (!body) {
        return send_redirect(req, ADMIN_PORTAL_PAGE_ENROLL);
    }

    char portal_name[sizeof(g_state.ap_ssid)];
    char password[sizeof(g_state.ap_password)];
    bool has_portal_name = form_get_field(body, "portal", portal_name, sizeof(portal_name));
    bool has_password = form_get_field(body, "password", password, sizeof(password));
    
    free(body);

    if (!has_portal_name || portal_name[0] == '\0' ||
        !has_password || !admin_portal_state_password_valid(&g_state, password)) {
        return send_redirect(req, ADMIN_PORTAL_PAGE_ENROLL);
    }

    wifi_set_ap_ssid(portal_name);
    wifi_set_ap_password(password);
    admin_portal_state_set_ssid(&g_state, portal_name);
    admin_portal_state_set_password(&g_state, password);
    admin_portal_state_authorize_session(&g_state);
    
    // Set session cookie for desktop browsers
    uint32_t max_age = g_state.inactivity_timeout_ms ? 
                       (uint32_t)(g_state.inactivity_timeout_ms / 1000UL) : 60U;
    set_session_cookie(req, token, max_age);

    return send_redirect(req, ADMIN_PORTAL_PAGE_MAIN);
}

static esp_err_t handle_login(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));

    if (status == ADMIN_PORTAL_SESSION_BUSY) {
        return send_redirect(req, ADMIN_PORTAL_PAGE_BUSY);
    }

    if (status == ADMIN_PORTAL_SESSION_NONE) {
        create_session(req, token, sizeof(token));
    }

    if (!admin_portal_state_has_password(&g_state)) {
        return send_redirect(req, ADMIN_PORTAL_PAGE_ENROLL);
    }

    char *body = read_body(req);
    if (!body) {
        return send_redirect(req, ADMIN_PORTAL_PAGE_AUTH);
    }

    char password[sizeof(g_state.ap_password)];
    bool has_password = form_get_field(body, "password", password, sizeof(password));
    
    free(body);

    if (!has_password || !admin_portal_state_password_matches(&g_state, password)) {
        return send_redirect(req, ADMIN_PORTAL_PAGE_AUTH);
    }

    admin_portal_state_authorize_session(&g_state);
    
    // Set session cookie for desktop browsers
    uint32_t max_age = g_state.inactivity_timeout_ms ? 
                       (uint32_t)(g_state.inactivity_timeout_ms / 1000UL) : 60U;
    set_session_cookie(req, token, max_age);
    
    return send_redirect(req, ADMIN_PORTAL_PAGE_MAIN);
}

static esp_err_t handle_logoff(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));

    // Clear session regardless of current status
    admin_portal_state_clear_session(&g_state);
    
    // Clear session cookie
    set_session_cookie(req, NULL, 0);
    
    LOGI(TAG, "User logged off - session cleared");
    
    return send_redirect(req, ADMIN_PORTAL_PAGE_AUTH);
}

static esp_err_t handle_change_password(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));

    if (status == ADMIN_PORTAL_SESSION_BUSY) {
        return send_json(req, "409 Conflict", "{\"status\":\"busy\"}");
    }
    
    if (status != ADMIN_PORTAL_SESSION_MATCH || !admin_portal_state_session_authorized(&g_state)) {
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

    wifi_set_ap_password(next);
    admin_portal_state_set_password(&g_state, next);
    admin_portal_state_authorize_session(&g_state);

    // Set session cookie BEFORE sending response to ensure authorized session is maintained
    uint32_t max_age = g_state.inactivity_timeout_ms ? (uint32_t)(g_state.inactivity_timeout_ms / 1000UL) : 60U;
    set_session_cookie(req, token, max_age);

    LOGI(TAG, "Change password successful, redirecting to device page");
    return send_json(req, "200 OK", "{\"status\":\"ok\",\"redirect\":\"/device/\"}");
}

static esp_err_t handle_update_device(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));

    if (status == ADMIN_PORTAL_SESSION_BUSY) {
        return send_json(req, "409 Conflict", "{\"status\":\"busy\"}");
    }
    
    if (status != ADMIN_PORTAL_SESSION_MATCH || !admin_portal_state_session_authorized(&g_state)) {
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

    wifi_set_ap_ssid(ssid);
    admin_portal_state_set_ssid(&g_state, ssid);
    wifi_set_ap_ssid(ssid);

    LOGI(TAG, "Device update successful, redirecting to main page (ssid=\"%s\")", ssid);
    return send_json(req, "200 OK", "{\"status\":\"ok\",\"redirect\":\"/main/\"}");
}

static esp_err_t handle_wifi_networks(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));

    if (status == ADMIN_PORTAL_SESSION_BUSY) {
        LOGW(TAG, "WiFi networks API: session busy");
        return send_json(req, "409 Conflict", "{\"status\":\"busy\"}");
    }
    
    if (status != ADMIN_PORTAL_SESSION_MATCH || !admin_portal_state_session_authorized(&g_state)) {
        LOGW(TAG, "WiFi networks API: unauthorized (status=%d, authorized=%d)", status, admin_portal_state_session_authorized(&g_state));
        return send_json(req, "403 Forbidden", "{\"status\":\"error\",\"code\":\"unauthorized\"}");
    }

    LOGI(TAG, "WiFi networks API called, scan_ready=%d, results_length=%zu", g_wifi_scan_ready, strlen(g_wifi_scan_results));
    LOGI(TAG, "Current scan results: %s", g_wifi_scan_results);

    char response[2560];
    if (g_wifi_scan_ready && strlen(g_wifi_scan_results) > 0) {
        snprintf(response, sizeof(response), "{\"status\":\"ok\",\"networks\":%s,\"scanning\":false}", g_wifi_scan_results);
        LOGI(TAG, "Returning completed scan results");
    } else {
        snprintf(response, sizeof(response), "{\"status\":\"ok\",\"networks\":[],\"scanning\":true}");
        LOGI(TAG, "Scan still in progress or no results, returning scanning=true");
    }
    
    return send_json(req, "200 OK", response);
}

static esp_err_t handle_page_request(httpd_req_t *req, const admin_portal_page_descriptor_t *desc)
{
    if (!desc)
        return ESP_ERR_INVALID_ARG;

    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));
    
    admin_portal_page_t target = admin_portal_state_resolve_page(&g_state, desc->page, status);

    if (!admin_portal_state_has_password(&g_state) &&
        target != ADMIN_PORTAL_PAGE_ENROLL &&
        target != ADMIN_PORTAL_PAGE_BUSY) {
        target = ADMIN_PORTAL_PAGE_ENROLL;
    }

    if (target != desc->page)
        return send_redirect(req, target);

    // Only auto-create sessions for pages that need them
    // AUTH page should not auto-create sessions - sessions should only be created upon successful authentication
    // BUSY page should never create sessions
    if (status == ADMIN_PORTAL_SESSION_NONE && 
        desc->page != ADMIN_PORTAL_PAGE_BUSY && 
        desc->page != ADMIN_PORTAL_PAGE_AUTH)
        create_session(req, token, sizeof(token));

    // Trigger start scan Wifi routine
    if (desc->page == ADMIN_PORTAL_PAGE_WIFI) {
        // Clear previous scan results and start new scan
        g_wifi_scan_ready = false;
        strcpy(g_wifi_scan_results, "[]");
        trigger_scan_wifi();
        LOGI(TAG, "WiFi page accessed, scan triggered");
    }

    return send_page_content(req, desc);
}

static esp_err_t handle_root(httpd_req_t *req)
{
    const admin_portal_page_descriptor_t *main_page = &admin_portal_page_main;
    
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    admin_portal_session_status_t status = evaluate_session(req, token, sizeof(token));
    
    admin_portal_page_t target = admin_portal_state_resolve_page(&g_state, main_page->page, status);

    if (!admin_portal_state_has_password(&g_state) &&
        target != ADMIN_PORTAL_PAGE_ENROLL &&
        target != ADMIN_PORTAL_PAGE_BUSY) {
        target = ADMIN_PORTAL_PAGE_ENROLL;
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
    
    // Register WiFi scan callback
    LOGI(TAG, "Attempting to register WiFi scan callback...");
    esp_err_t callback_err = wifi_manager_set_callback(WM_EVENT_SCAN_DONE, wifi_scan_done_callback);
    if (callback_err != ESP_OK) {
        LOGW(TAG, "Failed to register WiFi scan callback: %s", esp_err_to_name(callback_err));
    } else {
        LOGI(TAG, "WiFi scan callback registered successfully");
    }
    
    // Test callback registration by checking if we can retrieve current JSON
    const char* test_json = wifi_manager_get_ap_json();
    LOGI(TAG, "Current wifi_manager JSON state: %s", test_json ? test_json : "NULL");
    
    // Clear previous scan results
    g_wifi_scan_ready = false;
    strcpy(g_wifi_scan_results, "[]");
    LOGI(TAG, "WiFi scan system initialized, ready for scan requests");

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

    httpd_uri_t api_logoff = {
        .uri = "/api/logoff",
        .method = HTTP_POST,
        .handler = handle_logoff,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &api_logoff));

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

        }

    // Test endpoint to verify registration works
    httpd_uri_t api_test = {
        .uri = "/api/test",
        .method = HTTP_GET,
        .handler = handle_wifi_networks, // Reuse same handler for test
        .user_ctx = NULL,
    };
    esp_err_t test_api_err = httpd_register_uri_handler(server, &api_test);
    if (test_api_err == ESP_OK) {
        LOGI(TAG, "Test API endpoint registered successfully at /api/test");
    } else {
        LOGE(TAG, "Failed to register test API endpoint: %s", esp_err_to_name(test_api_err));
    }

    httpd_uri_t api_wifi_networks = {
        .uri = "/api/wifi_networks",
        .method = HTTP_GET,
        .handler = handle_wifi_networks,
        .user_ctx = NULL,
    };
    esp_err_t wifi_api_err = httpd_register_uri_handler(server, &api_wifi_networks);
    if (wifi_api_err == ESP_OK) {
        LOGI(TAG, "WiFi networks API endpoint registered successfully at /api/wifi_networks");
    } else {
        LOGE(TAG, "Failed to register WiFi networks API endpoint: %s", esp_err_to_name(wifi_api_err));
    }

    // Register single wildcard OPTIONS handler for all API endpoints (mobile browser compatibility)
    httpd_uri_t api_options_wildcard = {
        .uri = "/api/*",
        .method = HTTP_OPTIONS,
        .handler = handle_options,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &api_options_wildcard));

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

