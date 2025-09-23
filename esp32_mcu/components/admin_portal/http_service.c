#include "http_service.h"

#include "admin_portal_core.h"
#include "esp_timer.h"
#include "logs.h"

#include "pages/page_common.h"
#include "pages/page_main.h"
#include "pages/page_enroll/page_enroll.h"
#include "pages/page_auth/page_auth.h"
#include "pages/page_change_psw/page_change_psw.h"
#include "pages/page_device/page_device.h"
#include "pages/page_wifi/page_wifi.h"
#include "pages/page_clients/page_clients.h"
#include "pages/page_events/page_events.h"
#include "pages/page_busy/page_busy.h"
#include "pages/page_off/page_off.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "admin_http";

static admin_portal_state_t g_state;

#define SESSION_COOKIE_NAME "tapgate_session"
#define FORM_BUFFER_MAX     512

extern const uint8_t _binary_assets_logo_png_start[] asm("_binary_assets_logo_png_start");
extern const uint8_t _binary_assets_logo_png_end[] asm("_binary_assets_logo_png_end");
extern const uint8_t _binary_assets_lock_svg_start[] asm("_binary_assets_lock_svg_start");
extern const uint8_t _binary_assets_lock_svg_end[] asm("_binary_assets_lock_svg_end");
extern const uint8_t _binary_assets_settings_svg_start[] asm("_binary_assets_settings_svg_start");
extern const uint8_t _binary_assets_settings_svg_end[] asm("_binary_assets_settings_svg_end");
extern const uint8_t _binary_assets_wifi0_svg_start[] asm("_binary_assets_wifi0_svg_start");
extern const uint8_t _binary_assets_wifi0_svg_end[] asm("_binary_assets_wifi0_svg_end");
extern const uint8_t _binary_assets_wifi1_svg_start[] asm("_binary_assets_wifi1_svg_start");
extern const uint8_t _binary_assets_wifi1_svg_end[] asm("_binary_assets_wifi1_svg_end");
extern const uint8_t _binary_assets_wifi2_svg_start[] asm("_binary_assets_wifi2_svg_start");
extern const uint8_t _binary_assets_wifi2_svg_end[] asm("_binary_assets_wifi2_svg_end");
extern const uint8_t _binary_assets_wifi3_svg_start[] asm("_binary_assets_wifi3_svg_start");
extern const uint8_t _binary_assets_wifi3_svg_end[] asm("_binary_assets_wifi3_svg_end");

typedef struct {
    const char *uri;
    const uint8_t *start;
    const uint8_t *end;
    const char *content_type;
} asset_entry_t;

static const asset_entry_t ASSETS[] = {
    {"/assets/logo.png", _binary_assets_logo_png_start, _binary_assets_logo_png_end, "image/png"},
    {"/assets/lock.svg", _binary_assets_lock_svg_start, _binary_assets_lock_svg_end, "image/svg+xml"},
    {"/assets/settings.svg", _binary_assets_settings_svg_start, _binary_assets_settings_svg_end, "image/svg+xml"},
    {"/assets/wifi0.svg", _binary_assets_wifi0_svg_start, _binary_assets_wifi0_svg_end, "image/svg+xml"},
    {"/assets/wifi1.svg", _binary_assets_wifi1_svg_start, _binary_assets_wifi1_svg_end, "image/svg+xml"},
    {"/assets/wifi2.svg", _binary_assets_wifi2_svg_start, _binary_assets_wifi2_svg_end, "image/svg+xml"},
    {"/assets/wifi3.svg", _binary_assets_wifi3_svg_start, _binary_assets_wifi3_svg_end, "image/svg+xml"},
};

esp_err_t admin_portal_service_init(void)
{
    return admin_portal_state_init(&g_state);
}

void admin_portal_service_stop(void)
{
    admin_portal_session_end(&g_state);
}

static esp_err_t serve_asset(httpd_req_t *req, const char *uri)
{
    for (size_t i = 0; i < sizeof(ASSETS) / sizeof(ASSETS[0]); ++i)
    {
        if (strcmp(uri, ASSETS[i].uri) == 0)
        {
            const asset_entry_t *asset = &ASSETS[i];
            size_t length = (size_t)(asset->end - asset->start);
            httpd_resp_set_type(req, asset->content_type);
            httpd_resp_set_hdr(req, "Cache-Control", "no-store");
            return httpd_resp_send(req, (const char *)asset->start, length);
        }
    }
    return ESP_ERR_NOT_FOUND;
}

static size_t get_cookie(httpd_req_t *req, char *buffer, size_t buffer_size)
{
    if (!buffer || buffer_size == 0)
        return 0;

    size_t cookie_len = httpd_req_get_hdr_value_len(req, "Cookie");
    if (cookie_len <= 0 || cookie_len >= buffer_size)
        return 0;

    if (httpd_req_get_hdr_value_str(req, "Cookie", buffer, buffer_size) != ESP_OK)
        return 0;

    return strnlen(buffer, buffer_size);
}

static bool extract_session_id(const char *cookie_header, char *session_id, size_t session_size)
{
    if (!cookie_header || !session_id || session_size == 0)
        return false;

    const char *found = strstr(cookie_header, SESSION_COOKIE_NAME "=");
    if (!found)
        return false;

    found += strlen(SESSION_COOKIE_NAME) + 1;
    const char *end = found;
    while (*end && *end != ';')
        ++end;

    size_t length = (size_t)(end - found);
    if (length >= session_size)
        length = session_size - 1;
    memcpy(session_id, found, length);
    session_id[length] = '\0';
    return length > 0;
}

static void set_session_cookie(httpd_req_t *req, const char *session_id)
{
    if (!session_id || session_id[0] == '\0')
        return;
    char buffer[64];
    snprintf(buffer, sizeof(buffer), SESSION_COOKIE_NAME "=%s; Path=/; HttpOnly", session_id);
    httpd_resp_set_hdr(req, "Set-Cookie", buffer);
}

static void clear_session_cookie(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Set-Cookie", SESSION_COOKIE_NAME "=; Path=/; Max-Age=0");
}

static bool uri_matches(const char *uri, const char *path)
{
    size_t len = strlen(path);
    if (strncmp(uri, path, len) != 0)
        return false;
    return uri[len] == '\0' || uri[len] == '/' || uri[len] == '?';
}

static admin_portal_page_id_t path_to_page(const char *uri)
{
    if (!uri || uri[0] == '\0' || strcmp(uri, "/") == 0)
        return ADMIN_PAGE_MAIN;
    if (uri_matches(uri, "/main"))
        return ADMIN_PAGE_MAIN;
    if (uri_matches(uri, "/device"))
        return ADMIN_PAGE_DEVICE;
    if (uri_matches(uri, "/wifi"))
        return ADMIN_PAGE_WIFI;
    if (uri_matches(uri, "/clients"))
        return ADMIN_PAGE_CLIENTS;
    if (uri_matches(uri, "/events"))
        return ADMIN_PAGE_EVENTS;
    if (uri_matches(uri, "/auth"))
        return ADMIN_PAGE_AUTH;
    if (uri_matches(uri, "/enroll"))
        return ADMIN_PAGE_ENROLL;
    if (uri_matches(uri, "/psw"))
        return ADMIN_PAGE_CHANGE_PASSWORD;
    if (uri_matches(uri, "/busy"))
        return ADMIN_PAGE_BUSY;
    if (uri_matches(uri, "/off"))
        return ADMIN_PAGE_OFF;
    return ADMIN_PAGE_MAIN;
}

static const char *page_to_uri(admin_portal_page_id_t page)
{
    switch (page)
    {
        case ADMIN_PAGE_MAIN: return "/main";
        case ADMIN_PAGE_DEVICE: return "/device";
        case ADMIN_PAGE_WIFI: return "/wifi";
        case ADMIN_PAGE_CLIENTS: return "/clients";
        case ADMIN_PAGE_EVENTS: return "/events";
        case ADMIN_PAGE_AUTH: return "/auth";
        case ADMIN_PAGE_ENROLL: return "/enroll";
        case ADMIN_PAGE_CHANGE_PASSWORD: return "/psw";
        case ADMIN_PAGE_BUSY: return "/busy";
        case ADMIN_PAGE_OFF: return "/off";
        default: return "/";
    }
}

typedef struct {
    char key[32];
    char value[128];
} form_pair_t;

typedef struct {
    form_pair_t pairs[6];
    size_t count;
} form_data_t;

static int hex_to_int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static void url_decode(char *str)
{
    char *src = str;
    char *dst = str;
    while (*src)
    {
        if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else if (*src == '%' && src[1] && src[2])
        {
            int high = hex_to_int(src[1]);
            int low = hex_to_int(src[2]);
            if (high >= 0 && low >= 0)
            {
                *dst++ = (char)((high << 4) | low);
                src += 3;
            }
            else
            {
                *dst++ = *src++;
            }
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static void parse_form_data(form_data_t *form, char *buffer)
{
    if (!form || !buffer)
        return;

    char *saveptr = NULL;
    char *token = strtok_r(buffer, "&", &saveptr);
    while (token && form->count < sizeof(form->pairs) / sizeof(form->pairs[0]))
    {
        char *equal = strchr(token, '=');
        if (!equal)
        {
            token = strtok_r(NULL, "&", &saveptr);
            continue;
        }
        *equal = '\0';
        char *key = token;
        char *value = equal + 1;
        url_decode(key);
        url_decode(value);
        strncpy(form->pairs[form->count].key, key, sizeof(form->pairs[form->count].key) - 1);
        form->pairs[form->count].key[sizeof(form->pairs[form->count].key) - 1] = '\0';
        strncpy(form->pairs[form->count].value, value, sizeof(form->pairs[form->count].value) - 1);
        form->pairs[form->count].value[sizeof(form->pairs[form->count].value) - 1] = '\0';
        form->count++;
        token = strtok_r(NULL, "&", &saveptr);
    }
}

static const char *form_get(const form_data_t *form, const char *key)
{
    if (!form || !key)
        return NULL;
    for (size_t i = 0; i < form->count; ++i)
    {
        if (strcmp(form->pairs[i].key, key) == 0)
            return form->pairs[i].value;
    }
    return NULL;
}

static esp_err_t read_form(httpd_req_t *req, form_data_t *form)
{
    if (!req || !form)
        return ESP_ERR_INVALID_ARG;

    size_t content_length = req->content_len;
    if (content_length == 0 || content_length >= FORM_BUFFER_MAX)
        return ESP_ERR_INVALID_ARG;

    char *buffer = calloc(1, content_length + 1);
    if (!buffer)
        return ESP_ERR_NO_MEM;

    int received = httpd_req_recv(req, buffer, content_length);
    if (received < 0)
    {
        free(buffer);
        return ESP_FAIL;
    }
    buffer[received] = '\0';

    parse_form_data(form, buffer);
    free(buffer);
    return ESP_OK;
}

static esp_err_t send_redirect(httpd_req_t *req, const char *location)
{
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", location ? location : "/");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t handle_get(httpd_req_t *req, admin_portal_page_id_t page)
{
    switch (page)
    {
        case ADMIN_PAGE_MAIN:
            return page_main_render(req, &g_state);
        case ADMIN_PAGE_DEVICE:
            return page_device_render(req, &g_state, NULL, ADMIN_FOCUS_NONE);
        case ADMIN_PAGE_WIFI:
            return page_wifi_render(req, &g_state);
        case ADMIN_PAGE_CLIENTS:
            return page_clients_render(req, &g_state);
        case ADMIN_PAGE_EVENTS:
            return page_events_render(req, &g_state);
        case ADMIN_PAGE_AUTH:
            return page_auth_render(req, &g_state, NULL, ADMIN_FOCUS_NONE);
        case ADMIN_PAGE_ENROLL:
            return page_enroll_render(req, &g_state, NULL, ADMIN_FOCUS_NONE);
        case ADMIN_PAGE_CHANGE_PASSWORD:
            return page_change_psw_render(req, &g_state, NULL, ADMIN_FOCUS_NONE);
        case ADMIN_PAGE_BUSY:
            return page_busy_render(req);
        case ADMIN_PAGE_OFF:
            return page_off_render(req);
        default:
            return page_main_render(req, &g_state);
    }
}

static esp_err_t handle_post(httpd_req_t *req, const char *uri)
{
    form_data_t form = {0};
    esp_err_t err = read_form(req, &form);
    if (err != ESP_OK)
        return err;

    if (uri_matches(uri, "/enroll"))
    {
        const char *password = form_get(&form, "password");
        if (!admin_portal_password_valid(password))
        {
            return page_enroll_render(req, &g_state,
                                      "Password must be at least 8 characters", ADMIN_FOCUS_PASSWORD);
        }
        err = admin_portal_set_password(&g_state, password);
        if (err != ESP_OK)
        {
            return page_enroll_render(req, &g_state,
                                      "Failed to save password", ADMIN_FOCUS_PASSWORD);
        }
        admin_portal_touch(&g_state, esp_timer_get_time());
        return send_redirect(req, page_to_uri(ADMIN_PAGE_MAIN));
    }
    else if (uri_matches(uri, "/auth"))
    {
        const char *password = form_get(&form, "password");
        if (!password)
        {
            return page_auth_render(req, &g_state,
                                    "Password is required", ADMIN_FOCUS_PASSWORD);
        }
        err = admin_portal_verify_password(&g_state, password);
        if (err != ESP_OK)
        {
            return page_auth_render(req, &g_state,
                                    "Incorrect password", ADMIN_FOCUS_PASSWORD);
        }
        admin_portal_touch(&g_state, esp_timer_get_time());
        return send_redirect(req, page_to_uri(ADMIN_PAGE_MAIN));
    }
    else if (uri_matches(uri, "/psw"))
    {
        const char *old_password = form_get(&form, "old");
        const char *new_password = form_get(&form, "new");
        if (!old_password || !new_password)
        {
            return page_change_psw_render(req, &g_state,
                                          "Both password fields are required", ADMIN_FOCUS_PASSWORD_OLD);
        }
        err = admin_portal_change_password(&g_state, old_password, new_password);
        if (err == ESP_ERR_INVALID_STATE)
        {
            return page_change_psw_render(req, &g_state,
                                          "Old password is incorrect", ADMIN_FOCUS_PASSWORD_OLD);
        }
        if (err == ESP_ERR_INVALID_ARG)
        {
            return page_change_psw_render(req, &g_state,
                                          "New password must meet length requirements", ADMIN_FOCUS_PASSWORD_NEW);
        }
        if (err != ESP_OK)
        {
            return page_change_psw_render(req, &g_state,
                                          "Failed to update password", ADMIN_FOCUS_PASSWORD_OLD);
        }
        admin_portal_touch(&g_state, esp_timer_get_time());
        return send_redirect(req, page_to_uri(ADMIN_PAGE_DEVICE));
    }
    else if (uri_matches(uri, "/device"))
    {
        const char *ssid = form_get(&form, "ssid");
        if (!ssid || ssid[0] == '\0')
        {
            return page_device_render(req, &g_state,
                                      "SSID cannot be empty", ADMIN_FOCUS_SSID);
        }
        err = admin_portal_set_ap_ssid(&g_state, ssid);
        if (err != ESP_OK)
        {
            return page_device_render(req, &g_state,
                                      "Failed to save SSID", ADMIN_FOCUS_SSID);
        }
        admin_portal_touch(&g_state, esp_timer_get_time());
        return send_redirect(req, page_to_uri(ADMIN_PAGE_MAIN));
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t admin_portal_service_handle(httpd_req_t *req)
{
    if (!req)
        return ESP_ERR_INVALID_ARG;

    if (req->uri && strncmp(req->uri, "/assets/", 8) == 0)
    {
        esp_err_t err = serve_asset(req, req->uri);
        if (err == ESP_ERR_NOT_FOUND)
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Asset not found");
        return err;
    }

    uint64_t now = esp_timer_get_time();

    char cookie_header[128] = {0};
    get_cookie(req, cookie_header, sizeof(cookie_header));

    char session_id[ADMIN_PORTAL_SESSION_ID_MAX] = {0};
    bool has_session = extract_session_id(cookie_header, session_id, sizeof(session_id));

    char assigned_session[ADMIN_PORTAL_SESSION_ID_MAX] = {0};
    admin_session_result_t session_result = admin_portal_resolve_session(&g_state,
                                                                         has_session ? session_id : NULL,
                                                                         now,
                                                                         assigned_session,
                                                                         sizeof(assigned_session));

    if (session_result == ADMIN_SESSION_BUSY)
    {
        return page_busy_render(req);
    }
    if (session_result == ADMIN_SESSION_TIMED_OUT)
    {
        clear_session_cookie(req);
        return page_off_render(req);
    }

    if (session_result == ADMIN_SESSION_ACCEPTED_NEW)
    {
        set_session_cookie(req, assigned_session);
    }
    else if (!has_session)
    {
        set_session_cookie(req, assigned_session);
    }

    admin_portal_touch(&g_state, now);

    admin_portal_page_id_t requested_page = path_to_page(req->uri);
    admin_portal_page_id_t final_page = admin_portal_select_page(&g_state, requested_page);

    if (req->method == HTTP_GET && requested_page != final_page)
    {
        return send_redirect(req, page_to_uri(final_page));
    }

    if (req->method == HTTP_GET)
    {
        return handle_get(req, final_page);
    }
    else if (req->method == HTTP_POST)
    {
        esp_err_t err = handle_post(req, req->uri);
        if (err == ESP_ERR_NOT_FOUND)
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not supported");
        return err;
    }

    httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
    return ESP_ERR_INVALID_ARG;
}

