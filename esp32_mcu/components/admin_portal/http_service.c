#include "http_service.h"

#include "admin_portal_device.h"
#include "admin_portal_state.h"
#include "logs.h"
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

#include "esp_timer.h"
#include "esp_random.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "admin_http";

static admin_portal_state_t g_state;
static httpd_handle_t g_server = NULL;
static bool g_service_started = false;

static uint64_t current_time_ms(void)
{
    return (uint64_t)esp_timer_get_time() / 1000ULL;
}

static void set_no_cache_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
}

static void set_cache_headers_static(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");
}

static void set_session_cookie(httpd_req_t *req, const char *token)
{
    if (!token)
        return;
    uint32_t max_age = g_state.inactivity_timeout_ms / 1000U;
    if (max_age == 0)
        max_age = ADMIN_PORTAL_DEFAULT_IDLE_TIMEOUT_MIN * 60U;
    char buffer[128];
    snprintf(buffer, sizeof(buffer),
             "tg_session=%s; Path=/; Max-Age=%" PRIu32 "; HttpOnly; SameSite=Lax",
             token, max_age);
    httpd_resp_set_hdr(req, "Set-Cookie", buffer);
}

static void clear_session_cookie(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Set-Cookie",
                       "tg_session=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
}

static bool get_session_token(httpd_req_t *req, char *buffer, size_t buffer_len)
{
    if (!buffer || buffer_len == 0)
        return false;
    size_t cookie_len = httpd_req_get_hdr_value_len(req, "Cookie");
    if (cookie_len == 0 || cookie_len + 1 > buffer_len)
        return false;
    if (httpd_req_get_hdr_value_str(req, "Cookie", buffer, buffer_len) != ESP_OK)
        return false;

    const char *needle = "tg_session=";
    char *start = strstr(buffer, needle);
    if (!start)
        return false;
    start += strlen(needle);
    size_t i = 0;
    while (start[i] && start[i] != ';' && !isspace((unsigned char)start[i]) && i < ADMIN_PORTAL_SESSION_TOKEN_LENGTH)
    {
        buffer[i] = start[i];
        ++i;
    }
    buffer[i] = '\0';
    return i > 0;
}

static void generate_session_token(char *buffer, size_t buffer_len)
{
    if (!buffer || buffer_len == 0)
        return;

    uint8_t raw[ADMIN_PORTAL_SESSION_TOKEN_LENGTH / 2] = {0};
    esp_fill_random(raw, sizeof(raw));

    static const char hex[] = "0123456789abcdef";
    size_t pos = 0;
    for (size_t i = 0; i < sizeof(raw) && pos + 1 < buffer_len; ++i)
    {
        buffer[pos++] = hex[(raw[i] >> 4) & 0x0F];
        if (pos < buffer_len - 1)
            buffer[pos++] = hex[raw[i] & 0x0F];
    }
    buffer[pos < buffer_len ? pos : buffer_len - 1] = '\0';
}

typedef struct {
    const char *path;
    const uint8_t *start;
    const uint8_t *end;
    size_t size;
    const char *content_type;
    bool compressed;
} admin_portal_asset_t;

extern const uint8_t _binary_app_js_gz_start[];
extern const uint8_t _binary_app_js_gz_end[];
extern const uint8_t _binary_styles_css_gz_start[];
extern const uint8_t _binary_styles_css_gz_end[];
extern const uint8_t _binary_logo_png_start[];
extern const uint8_t _binary_logo_png_end[];
extern const uint8_t _binary_settings_svg_start[];
extern const uint8_t _binary_settings_svg_end[];
extern const uint8_t _binary_lock_svg_start[];
extern const uint8_t _binary_lock_svg_end[];
extern const uint8_t _binary_wifi0_svg_start[];
extern const uint8_t _binary_wifi0_svg_end[];
extern const uint8_t _binary_wifi1_svg_start[];
extern const uint8_t _binary_wifi1_svg_end[];
extern const uint8_t _binary_wifi2_svg_start[];
extern const uint8_t _binary_wifi2_svg_end[];
extern const uint8_t _binary_wifi3_svg_start[];
extern const uint8_t _binary_wifi3_svg_end[];

static admin_portal_asset_t g_assets[] = {
    {"/assets/app.js", _binary_app_js_gz_start, _binary_app_js_gz_end, 0, "application/javascript", true},
    {"/assets/styles.css", _binary_styles_css_gz_start, _binary_styles_css_gz_end, 0, "text/css", true},
    {"/assets/logo.png", _binary_logo_png_start, _binary_logo_png_end, 0, "image/png", false},
    {"/assets/settings.svg", _binary_settings_svg_start, _binary_settings_svg_end, 0, "image/svg+xml", false},
    {"/assets/lock.svg", _binary_lock_svg_start, _binary_lock_svg_end, 0, "image/svg+xml", false},
    {"/assets/wifi0.svg", _binary_wifi0_svg_start, _binary_wifi0_svg_end, 0, "image/svg+xml", false},
    {"/assets/wifi1.svg", _binary_wifi1_svg_start, _binary_wifi1_svg_end, 0, "image/svg+xml", false},
    {"/assets/wifi2.svg", _binary_wifi2_svg_start, _binary_wifi2_svg_end, 0, "image/svg+xml", false},
    {"/assets/wifi3.svg", _binary_wifi3_svg_start, _binary_wifi3_svg_end, 0, "image/svg+xml", false},
};

static const admin_portal_page_descriptor_t *g_pages[ADMIN_PORTAL_PAGE_COUNT];
static char g_page_trimmed_uris[ADMIN_PORTAL_PAGE_COUNT][64];
static bool g_page_trimmed_uris_valid[ADMIN_PORTAL_PAGE_COUNT];

static void ensure_assets_initialized(void)
{
    for (size_t i = 0; i < sizeof(g_assets) / sizeof(g_assets[0]); ++i)
    {
        if (g_assets[i].size == 0 && g_assets[i].start && g_assets[i].end)
            g_assets[i].size = (size_t)(g_assets[i].end - g_assets[i].start);
    }
}

static void ensure_pages_initialized(void)
{
    if (g_pages[ADMIN_PORTAL_PAGE_ENROLL])
        return;

    g_pages[ADMIN_PORTAL_PAGE_ENROLL] = admin_portal_page_enroll_descriptor();
    g_pages[ADMIN_PORTAL_PAGE_AUTH] = admin_portal_page_auth_descriptor();
    g_pages[ADMIN_PORTAL_PAGE_MAIN] = admin_portal_page_main_descriptor();
    g_pages[ADMIN_PORTAL_PAGE_DEVICE] = admin_portal_page_device_descriptor();
    g_pages[ADMIN_PORTAL_PAGE_WIFI] = admin_portal_page_wifi_descriptor();
    g_pages[ADMIN_PORTAL_PAGE_CLIENTS] = admin_portal_page_clients_descriptor();
    g_pages[ADMIN_PORTAL_PAGE_EVENTS] = admin_portal_page_events_descriptor();
    g_pages[ADMIN_PORTAL_PAGE_CHANGE_PASSWORD] = admin_portal_page_change_psw_descriptor();
    g_pages[ADMIN_PORTAL_PAGE_BUSY] = admin_portal_page_busy_descriptor();
    g_pages[ADMIN_PORTAL_PAGE_OFF] = admin_portal_page_off_descriptor();

    for (size_t i = 0; i < sizeof(g_pages) / sizeof(g_pages[0]); ++i)
    {
        const char *uri = g_pages[i]->uri;
        size_t len = strlen(uri);
        size_t trimmed_len = trim_trailing_slashes(uri, len);
        if (trimmed_len < len && trimmed_len > 0 && trimmed_len < sizeof(g_page_trimmed_uris[i]))
        {
            memcpy(g_page_trimmed_uris[i], uri, trimmed_len);
            g_page_trimmed_uris[i][trimmed_len] = '\0';
            g_page_trimmed_uris_valid[i] = true;
        }
    }
}

static size_t trim_trailing_slashes(const char *uri, size_t len)
{
    while (len > 1 && uri[len - 1] == '/')
        --len;
    return len;
}

static bool match_page_uri_exact_or_trimmed(const char *template_uri, const char *requested_uri, size_t requested_len)
{
    if (!template_uri || !requested_uri)
        return false;

    size_t template_len = trim_trailing_slashes(template_uri, strlen(template_uri));
    size_t actual_len = requested_len ? trim_trailing_slashes(requested_uri, requested_len)
                                      : trim_trailing_slashes(requested_uri, strlen(requested_uri));

    if (template_len != actual_len)
        return false;

    return strncmp(template_uri, requested_uri, template_len) == 0;
}

static const admin_portal_page_descriptor_t *find_page_by_uri(const char *uri)
{
    ensure_pages_initialized();
    if (!uri)
        return NULL;
    for (size_t i = 0; i < sizeof(g_pages) / sizeof(g_pages[0]); ++i)
    {
        if (match_page_uri_exact_or_trimmed(g_pages[i]->uri, uri, 0))
            return g_pages[i];
    }
    return NULL;
}

static const admin_portal_page_descriptor_t *find_page_by_type(admin_portal_page_t page)
{
    ensure_pages_initialized();
    for (size_t i = 0; i < sizeof(g_pages) / sizeof(g_pages[0]); ++i)
    {
        if (g_pages[i]->page == page)
            return g_pages[i];
    }
    return NULL;
}

static esp_err_t send_asset(httpd_req_t *req, admin_portal_asset_t *asset)
{
    ensure_assets_initialized();
    if (!asset)
        return ESP_ERR_NOT_FOUND;
    httpd_resp_set_type(req, asset->content_type);
    if (asset->compressed)
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    set_cache_headers_static(req);
    if (asset->size == 0 && asset->start && asset->end)
        asset->size = (size_t)(asset->end - asset->start);
    return httpd_resp_send(req, (const char *)asset->start, asset->size);
}

static esp_err_t handle_asset(httpd_req_t *req)
{
    admin_portal_asset_t *asset = (admin_portal_asset_t *)req->user_ctx;
    if (!asset)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Found");
        return ESP_FAIL;
    }
    return send_asset(req, asset);
}

static esp_err_t send_page_response(httpd_req_t *req, const admin_portal_page_descriptor_t *descriptor)
{
    ensure_pages_initialized();
    if (!descriptor)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, descriptor->content_type);
    if (descriptor->compressed)
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    set_no_cache_headers(req);
    if (descriptor->size == 0 && descriptor->data)
    {
        switch (descriptor->page)
        {
        case ADMIN_PORTAL_PAGE_AUTH:
            admin_portal_page_auth_descriptor();
            break;
        case ADMIN_PORTAL_PAGE_BUSY:
            admin_portal_page_busy_descriptor();
            break;
        case ADMIN_PORTAL_PAGE_CHANGE_PASSWORD:
            admin_portal_page_change_psw_descriptor();
            break;
        case ADMIN_PORTAL_PAGE_CLIENTS:
            admin_portal_page_clients_descriptor();
            break;
        case ADMIN_PORTAL_PAGE_DEVICE:
            admin_portal_page_device_descriptor();
            break;
        case ADMIN_PORTAL_PAGE_ENROLL:
            admin_portal_page_enroll_descriptor();
            break;
        case ADMIN_PORTAL_PAGE_EVENTS:
            admin_portal_page_events_descriptor();
            break;
        case ADMIN_PORTAL_PAGE_MAIN:
            admin_portal_page_main_descriptor();
            break;
        case ADMIN_PORTAL_PAGE_OFF:
            admin_portal_page_off_descriptor();
            break;
        case ADMIN_PORTAL_PAGE_WIFI:
            admin_portal_page_wifi_descriptor();
            break;
        default:
            break;
        }
    }
    return httpd_resp_send(req, (const char *)descriptor->data, descriptor->size);
}

static void escape_js_string(char *dest, size_t dest_len, const char *src)
{
    if (!dest || dest_len == 0)
        return;
    size_t pos = 0;
    if (!src)
    {
        dest[0] = '\0';
        return;
    }
    while (*src && pos + 1 < dest_len)
    {
        char c = *src++;
        switch (c)
        {
        case '\\':
        case '"':
            if (pos + 2 >= dest_len)
            {
                dest[pos] = '\0';
                return;
            }
            dest[pos++] = '\\';
            dest[pos++] = c;
            break;
        case '\n':
            if (pos + 2 >= dest_len)
            {
                dest[pos] = '\0';
                return;
            }
            dest[pos++] = '\\';
            dest[pos++] = 'n';
            break;
        case '\r':
            if (pos + 2 >= dest_len)
            {
                dest[pos] = '\0';
                return;
            }
            dest[pos++] = '\\';
            dest[pos++] = 'r';
            break;
        case '\t':
            if (pos + 2 >= dest_len)
            {
                dest[pos] = '\0';
                return;
            }
            dest[pos++] = '\\';
            dest[pos++] = 't';
            break;
        default:
            dest[pos++] = c;
            break;
        }
    }
    dest[pos] = '\0';
}

static esp_err_t send_initial_data_script(httpd_req_t *req)
{
    const char *ssid = admin_portal_state_get_ssid(&g_state);
    bool has_password = admin_portal_state_has_password(&g_state);
    bool authorized = admin_portal_state_session_authorized(&g_state);

    char escaped_ssid[128];
    escape_js_string(escaped_ssid, sizeof(escaped_ssid), ssid);

    char script[512];
    snprintf(script, sizeof(script),
             "window.TAPGATE_INITIAL_DATA={ap_ssid:\"%s\",has_password:%s,authorized:%s};",
             escaped_ssid,
             has_password ? "true" : "false",
             authorized ? "true" : "false");

    httpd_resp_set_type(req, "application/javascript");
    set_no_cache_headers(req);
    return httpd_resp_send(req, script, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t send_json(httpd_req_t *req, const char *json)
{
    httpd_resp_set_type(req, "application/json");
    set_no_cache_headers(req);
    return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t send_busy_response(httpd_req_t *req)
{
    return send_json(req, "{\"status\":\"busy\",\"redirect\":\"/busy/\"}");
}

static esp_err_t send_expired_response(httpd_req_t *req)
{
    clear_session_cookie(req);
    return send_json(req, "{\"status\":\"expired\",\"redirect\":\"/off/\"}");
}

static esp_err_t read_request_body(httpd_req_t *req, char *buffer, size_t buffer_len)
{
    if (!buffer || buffer_len == 0)
        return ESP_ERR_INVALID_ARG;

    if (req->content_len >= buffer_len)
        return ESP_ERR_INVALID_SIZE;

    size_t remaining = req->content_len;
    size_t received = 0;
    while (remaining > 0)
    {
        int ret = httpd_req_recv(req, buffer + received, remaining);
        if (ret <= 0)
        {
            return ESP_FAIL;
        }
        received += (size_t)ret;
        remaining -= (size_t)ret;
    }
    buffer[received] = '\0';
    return ESP_OK;
}

static int hex_value(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

static size_t url_decode(char *dest, size_t dest_len, const char *src)
{
    if (!dest || dest_len == 0)
        return 0;
    size_t pos = 0;
    while (*src && pos + 1 < dest_len)
    {
        char c = *src++;
        if (c == '+')
        {
            dest[pos++] = ' ';
        }
        else if (c == '%' && src[0] && src[1])
        {
            int hi = hex_value((unsigned char)src[0]);
            int lo = hex_value((unsigned char)src[1]);
            if (hi >= 0 && lo >= 0)
            {
                dest[pos++] = (char)((hi << 4) | lo);
                src += 2;
            }
        }
        else
        {
            dest[pos++] = c;
        }
    }
    dest[pos] = '\0';
    return pos;
}

static bool form_get_value(const char *body, const char *key, char *out, size_t out_len)
{
    if (!body || !key || !out || out_len == 0)
        return false;

    size_t key_len = strlen(key);
    const char *cursor = body;
    while (*cursor)
    {
        const char *eq = strchr(cursor, '=');
        if (!eq)
            break;
        size_t name_len = (size_t)(eq - cursor);
        const char *value_start = eq + 1;
        const char *amp = strchr(value_start, '&');
        size_t value_len = amp ? (size_t)(amp - value_start) : strlen(value_start);

        if (name_len == key_len && strncmp(cursor, key, key_len) == 0)
        {
            char temp[256];
            size_t copy_len = value_len < sizeof(temp) - 1 ? value_len : sizeof(temp) - 1;
            memcpy(temp, value_start, copy_len);
            temp[copy_len] = '\0';
            url_decode(out, out_len, temp);
            return true;
        }

        if (!amp)
            break;
        cursor = amp + 1;
    }

    return false;
}

static bool validate_ssid(const char *ssid)
{
    if (!ssid)
        return false;
    size_t len = strlen(ssid);
    if (len == 0 || len >= MAX_SSID_SIZE)
        return false;
    return true;
}

static esp_err_t send_ok_redirect(httpd_req_t *req, const char *redirect)
{
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"status\":\"ok\",\"redirect\":\"%s\"}", redirect);
    return send_json(req, payload);
}

static esp_err_t send_error_code(httpd_req_t *req, const char *code)
{
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"status\":\"error\",\"code\":\"%s\"}", code);
    return send_json(req, payload);
}

static esp_err_t handle_initial_data(httpd_req_t *req)
{
    return send_initial_data_script(req);
}

static esp_err_t handle_session_info(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_SESSION_TOKEN_LENGTH + 1] = {0};
    bool has_token = get_session_token(req, token, sizeof(token));
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&g_state, has_token ? token : NULL, current_time_ms());

    if (status == ADMIN_PORTAL_SESSION_BUSY)
        return send_busy_response(req);
    if (status == ADMIN_PORTAL_SESSION_EXPIRED)
        return send_expired_response(req);

    admin_portal_page_t resolved =
        admin_portal_state_resolve_page(&g_state, ADMIN_PORTAL_PAGE_MAIN, status);
    bool has_password = admin_portal_state_has_password(&g_state);
    bool authorized = admin_portal_state_session_authorized(&g_state);

    char escaped_ssid[128];
    escape_js_string(escaped_ssid, sizeof(escaped_ssid), admin_portal_state_get_ssid(&g_state));

    char payload[256];
    const char *redirect = NULL;
    if (!has_password)
        redirect = "/enroll/";
    else if (!authorized)
        redirect = "/auth/";
    else if (resolved != ADMIN_PORTAL_PAGE_MAIN)
        redirect = "/main/";

    if (redirect)
    {
        snprintf(payload, sizeof(payload),
                 "{\"status\":\"redirect\",\"redirect\":\"%s\",\"ap_ssid\":\"%s\",\"authorized\":%s,\"has_password\":%s}",
                 redirect,
                 escaped_ssid,
                 authorized ? "true" : "false",
                 has_password ? "true" : "false");
    }
    else
    {
        snprintf(payload, sizeof(payload),
                 "{\"status\":\"ok\",\"ap_ssid\":\"%s\",\"authorized\":%s,\"has_password\":%s}",
                 escaped_ssid,
                 authorized ? "true" : "false",
                 has_password ? "true" : "false");
    }

    return send_json(req, payload);
}

static esp_err_t ensure_session(httpd_req_t *req)
{
    if (admin_portal_state_session_active(&g_state))
        return ESP_OK;

    char token[ADMIN_PORTAL_SESSION_TOKEN_LENGTH + 1];
    generate_session_token(token, sizeof(token));
    admin_portal_state_start_session(&g_state, token, current_time_ms(), false);
    set_session_cookie(req, token);
    return ESP_OK;
}

static esp_err_t ensure_session_for_request(httpd_req_t *req,
                                            admin_portal_session_status_t status,
                                            bool has_token)
{
    if (status == ADMIN_PORTAL_SESSION_MATCH)
        return ESP_OK;

    if (!has_token && admin_portal_state_session_active(&g_state) &&
        admin_portal_state_has_password(&g_state))
    {
        return ESP_FAIL;
    }

    if (status == ADMIN_PORTAL_SESSION_NONE && !admin_portal_state_session_active(&g_state))
        return ensure_session(req);

    return ESP_OK;
}

static bool single_client_lockout(bool has_token)
{
    return (!has_token && admin_portal_state_session_active(&g_state) &&
            admin_portal_state_has_password(&g_state));
}

static esp_err_t handle_page_request(httpd_req_t *req)
{
    const admin_portal_page_descriptor_t *requested =
        (const admin_portal_page_descriptor_t *)req->user_ctx;
    if (!requested)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not Found");
        return ESP_FAIL;
    }

    char token[ADMIN_PORTAL_SESSION_TOKEN_LENGTH + 1] = {0};
    bool has_token = get_session_token(req, token, sizeof(token));
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&g_state, has_token ? token : NULL, current_time_ms());

    if (status == ADMIN_PORTAL_SESSION_BUSY)
        return send_page_response(req, find_page_by_type(ADMIN_PORTAL_PAGE_BUSY));
    if (status == ADMIN_PORTAL_SESSION_EXPIRED)
        return send_page_response(req, find_page_by_type(ADMIN_PORTAL_PAGE_OFF));

    if (ensure_session_for_request(req, status, has_token) != ESP_OK)
    {
        return send_page_response(req, find_page_by_type(ADMIN_PORTAL_PAGE_BUSY));
    }

    admin_portal_page_t resolved_page =
        admin_portal_state_resolve_page(&g_state, requested->page, status);
    const admin_portal_page_descriptor_t *resolved = find_page_by_type(resolved_page);
    if (!resolved)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Page not available");
        return ESP_FAIL;
    }

    if (resolved != requested)
    {
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", resolved->uri);
        set_no_cache_headers(req);
        return httpd_resp_send(req, NULL, 0);
    }

    if (status == ADMIN_PORTAL_SESSION_NONE && resolved->page != ADMIN_PORTAL_PAGE_BUSY &&
        resolved->page != ADMIN_PORTAL_PAGE_OFF)
    {
        ensure_session(req);
    }

    return send_page_response(req, resolved);
}

static esp_err_t handle_root(httpd_req_t *req)
{
    req->user_ctx = (void *)find_page_by_type(ADMIN_PORTAL_PAGE_MAIN);
    return handle_page_request(req);
}

static esp_err_t handle_favicon(httpd_req_t *req)
{
    set_no_cache_headers(req);
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t handle_time_probe(httpd_req_t *req)
{
    set_no_cache_headers(req);
    httpd_resp_set_type(req, "application/json");
    uint64_t now_ms = current_time_ms();
    char payload[64];
    snprintf(payload, sizeof(payload), "{\"epoch\":%llu}", (unsigned long long)(now_ms / 1000ULL));
    return httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
}

static bool ensure_content_type_form(httpd_req_t *req)
{
    size_t len = httpd_req_get_hdr_value_len(req, "Content-Type");
    if (len == 0)
        return false;

    char content_type[96];
    if (len >= sizeof(content_type))
        return false;

    if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type)) != ESP_OK)
        return false;
    return strstr(content_type, "application/x-www-form-urlencoded") != NULL;
}

static esp_err_t handle_enroll(httpd_req_t *req)
{
    if (admin_portal_state_has_password(&g_state))
        return send_json(req, "{\"status\":\"redirect\",\"redirect\":\"/auth/\"}");

    char token[ADMIN_PORTAL_SESSION_TOKEN_LENGTH + 1] = {0};
    bool has_token = get_session_token(req, token, sizeof(token));
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&g_state, has_token ? token : NULL, current_time_ms());

    if (status == ADMIN_PORTAL_SESSION_BUSY)
        return send_busy_response(req);
    if (status == ADMIN_PORTAL_SESSION_EXPIRED)
        return send_expired_response(req);
    if (single_client_lockout(has_token))
        return send_busy_response(req);

    if (!ensure_content_type_form(req))
        return send_error_code(req, "invalid_request");

    char body[256];
    if (read_request_body(req, body, sizeof(body)) != ESP_OK)
        return send_error_code(req, "invalid_request");

    char password[MAX_PASSWORD_SIZE] = {0};
    char portal[MAX_SSID_SIZE] = {0};
    form_get_value(body, "password", password, sizeof(password));
    form_get_value(body, "portal", portal, sizeof(portal));

    if (!admin_portal_state_password_valid(&g_state, password))
        return send_error_code(req, "invalid_password");
    if (!validate_ssid(portal))
        return send_error_code(req, "invalid_ssid");

    if (admin_portal_device_save_password(&g_state, password) != ESP_OK)
        return send_error_code(req, "storage_failed");
    if (admin_portal_device_save_ssid(&g_state, portal) != ESP_OK)
        return send_error_code(req, "storage_failed");

    if (!admin_portal_state_session_active(&g_state))
        ensure_session(req);
    admin_portal_state_authorize_session(&g_state);
    return send_ok_redirect(req, "/main/");
}

static esp_err_t handle_login(httpd_req_t *req)
{
    if (!admin_portal_state_has_password(&g_state))
        return send_json(req, "{\"status\":\"redirect\",\"redirect\":\"/enroll/\"}");

    char token[ADMIN_PORTAL_SESSION_TOKEN_LENGTH + 1] = {0};
    bool has_token = get_session_token(req, token, sizeof(token));
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&g_state, has_token ? token : NULL, current_time_ms());

    if (status == ADMIN_PORTAL_SESSION_BUSY)
        return send_busy_response(req);
    if (status == ADMIN_PORTAL_SESSION_EXPIRED)
        return send_expired_response(req);
    if (single_client_lockout(has_token))
        return send_busy_response(req);

    if (!ensure_content_type_form(req))
        return send_error_code(req, "invalid_request");

    char body[128];
    if (read_request_body(req, body, sizeof(body)) != ESP_OK)
        return send_error_code(req, "invalid_request");

    char password[MAX_PASSWORD_SIZE] = {0};
    form_get_value(body, "password", password, sizeof(password));

    if (!admin_portal_state_verify_password(&g_state, password))
        return send_error_code(req, "wrong_password");

    if (!admin_portal_state_session_active(&g_state))
        ensure_session(req);
    admin_portal_state_authorize_session(&g_state);
    return send_ok_redirect(req, "/main/");
}

static esp_err_t handle_change_password(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_SESSION_TOKEN_LENGTH + 1] = {0};
    bool has_token = get_session_token(req, token, sizeof(token));
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&g_state, has_token ? token : NULL, current_time_ms());

    if (status == ADMIN_PORTAL_SESSION_BUSY)
        return send_busy_response(req);
    if (status == ADMIN_PORTAL_SESSION_EXPIRED)
        return send_expired_response(req);
    if (single_client_lockout(has_token))
        return send_busy_response(req);

    if (!admin_portal_state_session_authorized(&g_state))
        return send_json(req, "{\"status\":\"redirect\",\"redirect\":\"/auth/\"}");

    if (!ensure_content_type_form(req))
        return send_error_code(req, "invalid_request");

    char body[256];
    if (read_request_body(req, body, sizeof(body)) != ESP_OK)
        return send_error_code(req, "invalid_request");

    char current[MAX_PASSWORD_SIZE] = {0};
    char next[MAX_PASSWORD_SIZE] = {0};
    form_get_value(body, "current", current, sizeof(current));
    form_get_value(body, "next", next, sizeof(next));

    if (!admin_portal_state_verify_password(&g_state, current))
        return send_error_code(req, "wrong_password");
    if (!admin_portal_state_password_valid(&g_state, next))
        return send_error_code(req, "invalid_new_password");

    if (admin_portal_device_save_password(&g_state, next) != ESP_OK)
        return send_error_code(req, "storage_failed");

    return send_ok_redirect(req, "/device/");
}

static esp_err_t handle_update_device(httpd_req_t *req)
{
    char token[ADMIN_PORTAL_SESSION_TOKEN_LENGTH + 1] = {0};
    bool has_token = get_session_token(req, token, sizeof(token));
    admin_portal_session_status_t status =
        admin_portal_state_check_session(&g_state, has_token ? token : NULL, current_time_ms());

    if (status == ADMIN_PORTAL_SESSION_BUSY)
        return send_busy_response(req);
    if (status == ADMIN_PORTAL_SESSION_EXPIRED)
        return send_expired_response(req);
    if (single_client_lockout(has_token))
        return send_busy_response(req);

    if (!admin_portal_state_session_authorized(&g_state))
        return send_json(req, "{\"status\":\"redirect\",\"redirect\":\"/auth/\"}");

    if (!ensure_content_type_form(req))
        return send_error_code(req, "invalid_request");

    char body[256];
    if (read_request_body(req, body, sizeof(body)) != ESP_OK)
        return send_error_code(req, "invalid_request");

    char ssid[MAX_SSID_SIZE] = {0};
    form_get_value(body, "ssid", ssid, sizeof(ssid));
    if (!validate_ssid(ssid))
        return send_error_code(req, "invalid_ssid");

    if (admin_portal_device_save_ssid(&g_state, ssid) != ESP_OK)
        return send_error_code(req, "storage_failed");

    return send_ok_redirect(req, "/main/");
}

esp_err_t admin_portal_http_service_start(httpd_handle_t server)
{
    if (!server)
        return ESP_ERR_INVALID_ARG;
    if (g_service_started)
        return ESP_OK;

    admin_portal_state_init(&g_state,
                            ADMIN_PORTAL_DEFAULT_IDLE_TIMEOUT_MIN * 60U * 1000U,
                            WPA2_MINIMUM_PASSWORD_LENGTH);
    admin_portal_device_load(&g_state);

    ensure_assets_initialized();
    ensure_pages_initialized();

    esp_err_t err;

    for (size_t i = 0; i < sizeof(g_assets) / sizeof(g_assets[0]); ++i)
    {
        httpd_uri_t asset_uri = {
            .uri = g_assets[i].path,
            .method = HTTP_GET,
            .handler = handle_asset,
            .user_ctx = (void *)&g_assets[i],
        };
        err = httpd_register_uri_handler(server, &asset_uri);
        if (err != ESP_OK)
        {
            LOGE(TAG, "Failed to register asset %s: %s", g_assets[i].path, esp_err_to_name(err));
            return err;
        }
    }

    for (size_t i = 0; i < sizeof(g_pages) / sizeof(g_pages[0]); ++i)
    {
        httpd_uri_t page_uri = {
            .uri = g_pages[i]->uri,
            .method = HTTP_GET,
            .handler = handle_page_request,
            .user_ctx = (void *)g_pages[i],
        };
        err = httpd_register_uri_handler(server, &page_uri);
        if (err != ESP_OK)
        {
            LOGE(TAG, "Failed to register page %s: %s", g_pages[i]->uri, esp_err_to_name(err));
            return err;
        }

        if (g_page_trimmed_uris_valid[i])
        {
            httpd_uri_t alt_uri = page_uri;
            alt_uri.uri = g_page_trimmed_uris[i];
            err = httpd_register_uri_handler(server, &alt_uri);
            if (err == ESP_ERR_HTTPD_URI_ALREADY_REGISTERED)
                err = ESP_OK;
            if (err != ESP_OK)
            {
                LOGE(TAG, "Failed to register alt page %s: %s", alt_uri.uri, esp_err_to_name(err));
                return err;
            }
        }
    }

    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = handle_root,
    };
    err = httpd_register_uri_handler(server, &root_uri);
    if (err != ESP_OK)
    {
        LOGE(TAG, "Failed to register root handler: %s", esp_err_to_name(err));
        return err;
    }

    httpd_uri_t initial_data_uri = {
        .uri = "/api/initial-data.js",
        .method = HTTP_GET,
        .handler = handle_initial_data,
    };
    err = httpd_register_uri_handler(server, &initial_data_uri);
    if (err != ESP_OK)
        return err;

    httpd_uri_t session_uri = {
        .uri = "/api/session",
        .method = HTTP_GET,
        .handler = handle_session_info,
    };
    err = httpd_register_uri_handler(server, &session_uri);
    if (err != ESP_OK)
        return err;

    httpd_uri_t enroll_uri = {
        .uri = "/api/enroll",
        .method = HTTP_POST,
        .handler = handle_enroll,
    };
    err = httpd_register_uri_handler(server, &enroll_uri);
    if (err != ESP_OK)
        return err;

    httpd_uri_t login_uri = {
        .uri = "/api/login",
        .method = HTTP_POST,
        .handler = handle_login,
    };
    err = httpd_register_uri_handler(server, &login_uri);
    if (err != ESP_OK)
        return err;

    httpd_uri_t change_psw_uri = {
        .uri = "/api/change-password",
        .method = HTTP_POST,
        .handler = handle_change_password,
    };
    err = httpd_register_uri_handler(server, &change_psw_uri);
    if (err != ESP_OK)
        return err;

    httpd_uri_t device_uri = {
        .uri = "/api/device",
        .method = HTTP_POST,
        .handler = handle_update_device,
    };
    err = httpd_register_uri_handler(server, &device_uri);
    if (err != ESP_OK)
        return err;

    httpd_uri_t favicon_uri = {
        .uri = "/favicon.ico",
        .method = HTTP_GET,
        .handler = handle_favicon,
    };
    err = httpd_register_uri_handler(server, &favicon_uri);
    if (err != ESP_OK)
        return err;

    httpd_uri_t time_uri = {
        .uri = "/time/1/current",
        .method = HTTP_GET,
        .handler = handle_time_probe,
    };
    err = httpd_register_uri_handler(server, &time_uri);
    if (err != ESP_OK)
        return err;

    g_server = server;
    g_service_started = true;
    LOGI(TAG, "Admin portal HTTP service started");
    return ESP_OK;
}

void admin_portal_http_service_stop(void)
{
    if (!g_service_started || !g_server)
        return;
    g_service_started = false;
    g_server = NULL;
    memset(&g_state.session, 0, sizeof(g_state.session));
}
