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
#include "pages/page_off.h"
#include "pages/page_wifi.h"

static const char *TAG = "AdminPortal";

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

// Get client IP address from HTTP request
static bool get_client_ip(httpd_req_t *req, char *ip_buffer, size_t buffer_size)
{
    if (!req || !ip_buffer || buffer_size < 16)
        return false;

    // Try ESP-IDF specific way first - X-Forwarded-For header
    size_t len = httpd_req_get_hdr_value_len(req, "X-Forwarded-For");
    if (len > 0 && len < buffer_size) {
        if (httpd_req_get_hdr_value_str(req, "X-Forwarded-For", ip_buffer, buffer_size) == ESP_OK) {
            LOGI(TAG, "Got client IP from X-Forwarded-For: %s", ip_buffer);
            return true;
        }
    }

    // Try X-Real-IP header (common in proxies)
    len = httpd_req_get_hdr_value_len(req, "X-Real-IP");
    if (len > 0 && len < buffer_size) {
        if (httpd_req_get_hdr_value_str(req, "X-Real-IP", ip_buffer, buffer_size) == ESP_OK) {
            LOGI(TAG, "Got client IP from X-Real-IP: %s", ip_buffer);
            return true;
        }
    }

    // Fallback to socket method
    int sockfd = httpd_req_to_sockfd(req);
    if (sockfd < 0) {
        LOGW(TAG, "Failed to get socket descriptor from request");
        return false;
    }

    // Try both getpeername (peer address) and getsockname (local address)
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    
    // First try getpeername (remote peer address)
    if (getpeername(sockfd, (struct sockaddr*)&addr, &addr_len) == 0) {
        if (inet_ntop(AF_INET, &addr.sin_addr, ip_buffer, buffer_size) != NULL) {
            // Validate the IP is not 0.0.0.0 or localhost
            if (strcmp(ip_buffer, "0.0.0.0") != 0 && strcmp(ip_buffer, "127.0.0.1") != 0) {
                LOGI(TAG, "Got client IP from getpeername: %s", ip_buffer);
                return true;
            }
        }
    }
    
    // ESP32 AP mode often can't detect client IP properly via socket calls
    // For single-user admin portal, use a consistent mobile client identifier
    LOGW(TAG, "Socket-based IP detection failed, using mobile client identifier");
    
    // For mobile browsers connecting to ESP32 AP, use a consistent identifier
    // This ensures session consistency across requests
    strncpy(ip_buffer, "10.10.0.100", buffer_size - 1);  // Fixed mobile client ID
    ip_buffer[buffer_size - 1] = '\0';
    LOGI(TAG, "Using fixed mobile client identifier: %s", ip_buffer);
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
    
    // Add CORS headers for mobile compatibility
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "http://10.10.0.1");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Credentials", "true");
    
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
    
    // Add CORS headers for mobile compatibility (allow JavaScript to read Location header)
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Credentials", "true");
    httpd_resp_set_hdr(req, "Access-Control-Expose-Headers", "Location, Set-Cookie");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Cookie, Authorization");
    
    set_cache_headers(req);
    
    LOGI(TAG, "Redirect response headers set, sending empty body");
    return httpd_resp_send(req, "", 0);
}

// CORS preflight handler for mobile browser compatibility
static esp_err_t handle_options(httpd_req_t *req)
{
    LOGI(TAG, "Handling OPTIONS request for CORS preflight: %s", req->uri);
    
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "http://10.10.0.1");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Credentials", "true");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Cookie, Authorization");
    httpd_resp_set_hdr(req, "Access-Control-Max-Age", "86400"); // 24 hours
    
    return httpd_resp_send(req, "", 0);
}

static esp_err_t send_json(httpd_req_t *req, const char *status_text, const char *body)
{
    LOGI(TAG, "Sending JSON response: status=%s, body_length=%zu", 
         status_text, body ? strlen(body) : 0);
#ifdef DIAGNOSTIC_VERSION
    LOGI(TAG, "JSON Response body: %s", body ? body : "<null>");
#endif
    
    httpd_resp_set_status(req, status_text);
    httpd_resp_set_type(req, "application/json");
    
    // Add CORS headers to allow credentials (mobile-compatible)
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "http://10.10.0.1");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Credentials", "true");
    httpd_resp_set_hdr(req, "Access-Control-Expose-Headers", "Set-Cookie");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Cookie");
    
    set_cache_headers(req);
    
    esp_err_t result = httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
    LOGI(TAG, "JSON response sent: result=%s", esp_err_to_name(result));
    return result;
}

static admin_portal_session_status_t evaluate_session(httpd_req_t *req,
                                                      char *token_buffer,
                                                      size_t token_size)
{
    bool has_token = get_session_token(req, token_buffer, token_size);
    uint64_t now = get_now_ms();
    
    LOGI(TAG, "Evaluating session: has_token=%s, now=%" PRIu64 "ms", 
         has_token ? "yes" : "no", now);
    
    admin_portal_session_status_t status = admin_portal_state_check_session(&g_state, has_token ? token_buffer : NULL, now);
    
    LOGI(TAG, "Session evaluation result: status=%s", session_status_name(status));
    
    if (status == ADMIN_PORTAL_SESSION_MATCH)
    {
        admin_portal_state_update_session(&g_state, now);
        LOGI(TAG, "Session updated with current timestamp");
    }
    
    return status;
}

// Simple session evaluation (simplified for single-client admin portal)
static admin_portal_session_status_t evaluate_session_by_ip(httpd_req_t *req)
{
    char client_ip[16];
    bool has_ip = get_client_ip(req, client_ip, sizeof(client_ip));
    uint64_t now = get_now_ms();
    
    LOGI(TAG, "Evaluating simple session: client_ip=%s, now=%" PRIu64 "ms", 
         has_ip ? client_ip : "unknown", now);
    
    // For admin portal, we allow only single session, so just check if any session exists
    admin_portal_session_status_t status;
    
    if (has_ip && strlen(client_ip) > 0 && strcmp(client_ip, "0.0.0.0") != 0) {
        // Use IP-based check if we have valid IP
        status = admin_portal_state_check_session_by_ip(&g_state, client_ip, now);
    } else {
        // Fallback: just check if any session is active (admin portal = single user)
        if (g_state.session.active) {
            // Check timeout
            if (g_state.inactivity_timeout_ms > 0) {
                uint64_t elapsed = now - g_state.session.last_activity_ms;
                if (elapsed > g_state.inactivity_timeout_ms) {
                    admin_portal_state_clear_session(&g_state);
                    status = ADMIN_PORTAL_SESSION_EXPIRED;
                } else {
                    status = ADMIN_PORTAL_SESSION_MATCH;
                }
            } else {
                status = ADMIN_PORTAL_SESSION_MATCH;
            }
        } else {
            status = ADMIN_PORTAL_SESSION_NONE;
        }
        LOGI(TAG, "Using session fallback (no valid IP), session active=%s", 
             g_state.session.active ? "yes" : "no");
    }
    
    LOGI(TAG, "Simple session evaluation result: status=%s", session_status_name(status));
    
    if (status == ADMIN_PORTAL_SESSION_MATCH)
    {
        admin_portal_state_update_session(&g_state, now);
        LOGI(TAG, "Session updated with current timestamp");
    }
    
    return status;
}

// Create session using hybrid approach (IP-based + cookie fallback)
static esp_err_t ensure_session_claim(httpd_req_t *req,
                                      admin_portal_session_status_t *status,
                                      char *token_buffer,
                                      size_t token_size)
{
    if (!status || !token_buffer)
        return ESP_ERR_INVALID_ARG;

    if (*status != ADMIN_PORTAL_SESSION_NONE)
    {
        LOGI(TAG, "Session already exists, not creating new session");
        return ESP_OK;
    }

    LOGI(TAG, "Creating new hybrid session claim (IP + cookie)");
    
    // Try to get client IP for IP-based session
    char client_ip[16];
    bool has_ip = get_client_ip(req, client_ip, sizeof(client_ip));
    
    uint64_t now = get_now_ms();
    
    if (has_ip)
    {
        // Create IP-based session (mobile-friendly)
        admin_portal_state_start_session_by_ip(&g_state, client_ip, now, false);
        LOGI(TAG, "Created IP-based session for client: %s", client_ip);
    }
    
    // Also create cookie-based session for desktop browsers
    char new_token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1];
    generate_session_token(new_token);
    LOGI(TAG, "Generated session token: %.8s...", new_token);
    
    admin_portal_state_start_session(&g_state, new_token, now, false);
    uint32_t max_age = g_state.inactivity_timeout_ms ? (uint32_t)(g_state.inactivity_timeout_ms / 1000UL) : 60U;
    
    // For API endpoints, cookie will be set after authorization to avoid duplicate headers
    // For GET requests (pages), set cookie immediately for desktop compatibility
    bool is_api_request = req && req->uri[0] != '\0' && strstr(req->uri, "/api/") != NULL;
    
    if (!is_api_request)
    {
        LOGI(TAG, "Setting session cookie for page request: %s", req ? req->uri : "<null>");
        set_session_cookie(req, new_token, max_age);
    }
    else
    {
        LOGI(TAG, "API request detected, cookie will be set after authorization: %s", req ? req->uri : "<null>");
    }
    
    LOGI(TAG,
         "Hybrid session started for URI %s (IP=%s, max_age=%" PRIu32 " s)",
         req ? req->uri : "<null>",
         has_ip ? client_ip : "unavailable",
         max_age);
    
    strncpy(token_buffer, new_token, token_size - 1);
    token_buffer[token_size - 1] = '\0';
    *status = ADMIN_PORTAL_SESSION_MATCH;
    LOGI(TAG, "Hybrid session claim completed successfully");
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
    // Use IP-based session evaluation with cookie fallback
    admin_portal_session_status_t status = evaluate_session_by_ip(req);
    
    // If IP-based fails, try cookie-based for desktop browsers
    if (status == ADMIN_PORTAL_SESSION_NONE)
    {
        char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
        status = evaluate_session(req, token, sizeof(token));
    }

    LOGI(TAG,
         "API /api/enroll: status=%s has_password=%s",
         session_status_name(status),
         admin_portal_state_has_password(&g_state) ? "true" : "false");

    // Handle busy portal
    if (status == ADMIN_PORTAL_SESSION_BUSY)
    {
        LOGI(TAG, "Enrollment denied: portal busy");
        return send_redirect(req, ADMIN_PORTAL_PAGE_BUSY);
    }

    // Start simple session if needed (with IP if available, fallback to cookie-only)
    if (status == ADMIN_PORTAL_SESSION_NONE)
    {
        char client_ip[16];
        bool has_ip = get_client_ip(req, client_ip, sizeof(client_ip));
        
        uint64_t now = get_now_ms();
        
        if (has_ip && strlen(client_ip) > 0 && strcmp(client_ip, "0.0.0.0") != 0) {
            admin_portal_state_start_session_by_ip(&g_state, client_ip, now, false);
            LOGI(TAG, "Started IP-based session for client: %s", client_ip);
        } else {
            // Fallback: start simple session without IP
            char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1];
            generate_session_token(token);
            admin_portal_state_start_session(&g_state, token, now, false);
            LOGI(TAG, "Started simple session (no valid IP)");
        }
    }

    // Redirect to auth if password already exists
    if (admin_portal_state_has_password(&g_state))
    {
        LOGI(TAG, "Enrollment redirected to auth because password already set");
        return send_redirect(req, ADMIN_PORTAL_PAGE_AUTH);
    }

    char *body = read_body(req);
    if (!body)
    {
        LOGI(TAG, "Enrollment failed: request body missing");
        // Return to enroll page with error - could be handled by query param
        return send_redirect(req, ADMIN_PORTAL_PAGE_ENROLL);
    }

    char portal_name[sizeof(g_state.ap_ssid)];
    char password[sizeof(g_state.ap_password)];
    bool has_portal_name = form_get_field(body, "portal", portal_name, sizeof(portal_name));
    bool has_password = form_get_field(body, "password", password, sizeof(password));
    
    LOGI(TAG, "Enrollment form data: has_portal_name=%s, has_password=%s", 
         has_portal_name ? "yes" : "no", has_password ? "yes" : "no");
#ifdef DIAGNOSTIC_VERSION
    LOGI(TAG, "Portal name: \"%s\", password length: %zu", 
         has_portal_name ? portal_name : "<missing>", 
         has_password ? strlen(password) : 0);
#endif
    
    free(body);

    if (!has_portal_name || portal_name[0] == '\0')
    {
        LOGI(TAG, "Enrollment failed: portal name missing");
        return send_redirect(req, ADMIN_PORTAL_PAGE_ENROLL);
    }

    if (!has_password || !admin_portal_state_password_valid(&g_state, password))
    {
        LOGI(TAG,
             "Enrollment failed: %s",
             has_password ? "password does not meet requirements" : "password missing");
        return send_redirect(req, ADMIN_PORTAL_PAGE_ENROLL);
    }

    wifi_set_ap_ssid(portal_name);
    wifi_set_ap_password(password);

    admin_portal_state_set_ssid(&g_state, portal_name);
    admin_portal_state_set_password(&g_state, password);
    
    LOGI(TAG, "Authorizing session after enrollment");
    admin_portal_state_authorize_session(&g_state);
    
    bool is_authorized = admin_portal_state_session_authorized(&g_state);
    LOGI(TAG, "Session authorization status: %s", is_authorized ? "authorized" : "not_authorized");
    
    wifi_set_ap_password(password);

    // Set session cookie for desktop browsers (mobile will use IP-based session)
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1];
    generate_session_token(token);
    uint32_t max_age = g_state.inactivity_timeout_ms ? (uint32_t)(g_state.inactivity_timeout_ms / 1000UL) : 60U;
    set_session_cookie(req, token, max_age);

    LOGI(TAG, "Enrollment successful, redirecting to main page (AP SSID=\"%s\")", portal_name);
    
    return send_redirect(req, ADMIN_PORTAL_PAGE_MAIN);
}

static esp_err_t handle_login(httpd_req_t *req)
{
    // Use IP-based session evaluation with cookie fallback
    admin_portal_session_status_t status = evaluate_session_by_ip(req);
    
    // If IP-based fails, try cookie-based for desktop browsers
    if (status == ADMIN_PORTAL_SESSION_NONE)
    {
        char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
        status = evaluate_session(req, token, sizeof(token));
    }

    LOGI(TAG,
         "API /api/login: status=%s has_password=%s authorized=%s",
         session_status_name(status),
         admin_portal_state_has_password(&g_state) ? "true" : "false",
         admin_portal_state_session_authorized(&g_state) ? "true" : "false");

    if (status == ADMIN_PORTAL_SESSION_BUSY)
    {
        LOGI(TAG, "Login denied: portal busy");
        return send_redirect(req, ADMIN_PORTAL_PAGE_BUSY);
    }

    // Start simple session if needed (with IP if available, fallback to cookie-only)
    if (status == ADMIN_PORTAL_SESSION_NONE)
    {
        char client_ip[16];
        bool has_ip = get_client_ip(req, client_ip, sizeof(client_ip));
        
        uint64_t now = get_now_ms();
        
        if (has_ip && strlen(client_ip) > 0 && strcmp(client_ip, "0.0.0.0") != 0) {
            admin_portal_state_start_session_by_ip(&g_state, client_ip, now, false);
            LOGI(TAG, "Started IP-based session for login client: %s", client_ip);
        } else {
            // Fallback: start simple session without IP
            char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1];
            generate_session_token(token);
            admin_portal_state_start_session(&g_state, token, now, false);
            LOGI(TAG, "Started simple session for login (no valid IP)");
        }
    }

    if (!admin_portal_state_has_password(&g_state))
    {
        LOGI(TAG, "Login redirected to enroll because password missing");
        return send_redirect(req, ADMIN_PORTAL_PAGE_ENROLL);
    }

    char *body = read_body(req);
    if (!body)
    {
        LOGI(TAG, "Login failed: request body missing");
        return send_redirect(req, ADMIN_PORTAL_PAGE_AUTH);
    }

    char password[sizeof(g_state.ap_password)];
    bool has_password = form_get_field(body, "password", password, sizeof(password));
    
    LOGI(TAG, "Login form data: has_password=%s", has_password ? "yes" : "no");
#ifdef DIAGNOSTIC_VERSION
    LOGI(TAG, "Password length: %zu", has_password ? strlen(password) : 0);
#endif
    
    free(body);

    if (!has_password || !admin_portal_state_password_matches(&g_state, password))
    {
        LOGI(TAG, "Login failed: %s", 
             !has_password ? "password missing" : "wrong password");
        return send_redirect(req, ADMIN_PORTAL_PAGE_AUTH);
    }

    LOGI(TAG, "Authorizing session after login");
    admin_portal_state_authorize_session(&g_state);
    
    bool is_authorized = admin_portal_state_session_authorized(&g_state);
    LOGI(TAG, "Session authorization status: %s", is_authorized ? "authorized" : "not_authorized");
    
    // Set session cookie for desktop browsers (mobile will use IP-based session)
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1];
    generate_session_token(token);
    uint32_t max_age = g_state.inactivity_timeout_ms ? (uint32_t)(g_state.inactivity_timeout_ms / 1000UL) : 60U;
    set_session_cookie(req, token, max_age);
    
    LOGI(TAG, "Login successful, redirecting to main page");
    
    return send_redirect(req, ADMIN_PORTAL_PAGE_MAIN);
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

    wifi_set_ap_password(next);
    admin_portal_state_set_password(&g_state, next);
    admin_portal_state_authorize_session(&g_state);
    wifi_set_ap_password(next);

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

    wifi_set_ap_ssid(ssid);
    admin_portal_state_set_ssid(&g_state, ssid);
    wifi_set_ap_ssid(ssid);

    LOGI(TAG, "Device update successful, redirecting to main page (ssid=\"%s\")", ssid);
    return send_json(req, "200 OK", "{\"status\":\"ok\",\"redirect\":\"/main/\"}");
}

static esp_err_t handle_page_request(httpd_req_t *req, const admin_portal_page_descriptor_t *desc)
{
    if (!desc)
        return ESP_ERR_INVALID_ARG;

    // Try IP-based session first (mobile-friendly), fallback to cookies (desktop)
    admin_portal_session_status_t status = evaluate_session_by_ip(req);
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    
    if (status == ADMIN_PORTAL_SESSION_NONE)
    {
        status = evaluate_session(req, token, sizeof(token));
        LOGI(TAG, "IP-based session failed, trying cookie-based: %s", session_status_name(status));
    }
    else
    {
        LOGI(TAG, "Using IP-based session: %s", session_status_name(status));
    }
    
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

    if (target != desc->page)
        return send_redirect(req, target);

    if (status == ADMIN_PORTAL_SESSION_NONE && desc->page != ADMIN_PORTAL_PAGE_BUSY && desc->page != ADMIN_PORTAL_PAGE_OFF)
        ensure_session_claim(req, &status, token, sizeof(token));

    LOGI(TAG, "Serving page %s", page_name(desc->page));
    return send_page_content(req, desc);
}

static esp_err_t handle_root(httpd_req_t *req)
{
    const admin_portal_page_descriptor_t *main_page = &admin_portal_page_main;
    
    // Try IP-based session first (mobile-friendly), fallback to cookies (desktop)
    admin_portal_session_status_t status = evaluate_session_by_ip(req);
    char token[ADMIN_PORTAL_TOKEN_MAX_LEN + 1] = {0};
    
    if (status == ADMIN_PORTAL_SESSION_NONE)
    {
        status = evaluate_session(req, token, sizeof(token));
        LOGI(TAG, "IP-based session failed for root, trying cookie-based: %s", session_status_name(status));
    }
    else
    {
        LOGI(TAG, "Using IP-based session for root: %s", session_status_name(status));
    }
    
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

