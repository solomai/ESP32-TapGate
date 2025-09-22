#include "page_main.h"

#include <stdio.h>

static const char PAGE_MAIN_HEAD[] =
    "<!DOCTYPE html>"
    "<html lang=\"en\">"
    "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<title>TapGate Control</title>"
        "<style>"
            "*{box-sizing:border-box;font-family:'Inter','Segoe UI',sans-serif;}"
            ":root{--bg:#0D1520;--card:#111927;--text:#E6EEF8;--muted:#9CA3AF;--button-height:48px;}"
            "body{margin:0;background:var(--bg);color:var(--text);min-height:100vh;}"
            ".page{min-height:100vh;display:flex;align-items:center;justify-content:center;padding:28px;}"
            ".panel{width:100%;max-width:420px;background:var(--card);border-radius:22px;padding:32px 26px;"
            "box-shadow:0 20px 45px rgba(8,12,24,0.45);display:flex;flex-direction:column;align-items:center;gap:22px;}"
            ".buttons{width:100%;display:flex;flex-direction:column;gap:16px;}"
            ".button{height:var(--button-height);border:none;border-radius:14px;"
            "background:linear-gradient(135deg,#0b2451 0%,#1c4c8c 45%,#3a8fdd 100%);color:#fff;font-size:17px;font-weight:600;"
            "letter-spacing:.3px;box-shadow:0 4px 12px rgba(0,0,0,0.3);cursor:pointer;transition:transform .15s,box-shadow .15s,filter .15s;}"
            ".button:hover,.button:focus{filter:brightness(1.08);outline:none;}"
            ".button:active{transform:translateY(1px);box-shadow:0 2px 6px rgba(0,0,0,0.25);filter:brightness(.92);}"
            ".button[disabled]{opacity:.55;cursor:not-allowed;box-shadow:none;}"
            ".version{font-size:13px;color:var(--muted);text-align:center;margin-top:8px;}"
        "</style>"
    "</head>"
    "<body>"
        "<div class=\"page\">"
            "<div class=\"panel\">"
                "<div class=\"buttons\">";

static const char PAGE_MAIN_BODY[] =
                    "<button class=\"button\" type=\"button\" disabled>TapGate</button>"
                    "<button class=\"button\" type=\"button\" onclick=\"window.location.href='/api/v1/ap'\">AP setting</button>"
                    "<button class=\"button\" type=\"button\" disabled>WiFi connection</button>"
                    "<button class=\"button\" type=\"button\" disabled>Clients</button>"
                    "<button class=\"button\" type=\"button\" disabled>Events</button>"
                "</div>";

static const char PAGE_MAIN_TAIL[] =
                "</div>"
            "</div>"
        "</div>"
    "</body>"
"</html>";

static const char *safe_text(const char *text)
{
    return text ? text : "";
}

esp_err_t page_main_render(httpd_req_t *req, const page_main_context_t *context)
{
    if (!req || !context) {
        return ESP_ERR_INVALID_ARG;
    }

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");

    const char *version = safe_text(context->version_label);

    esp_err_t err = httpd_resp_sendstr_chunk(req, PAGE_MAIN_HEAD);
    if (err != ESP_OK) {
        httpd_resp_sendstr_chunk(req, NULL);
        return err;
    }

    err = httpd_resp_sendstr_chunk(req, PAGE_MAIN_BODY);
    if (err != ESP_OK) {
        httpd_resp_sendstr_chunk(req, NULL);
        return err;
    }

    char version_block[128];
    int written = snprintf(version_block, sizeof(version_block),
                           "<div class=\"version\">%s</div>", version);
    if (written < 0 || (size_t)written >= sizeof(version_block)) {
        httpd_resp_sendstr_chunk(req, NULL);
        return ESP_ERR_INVALID_SIZE;
    }

    err = httpd_resp_send_chunk(req, version_block, written);
    if (err != ESP_OK) {
        httpd_resp_sendstr_chunk(req, NULL);
        return err;
    }

    err = httpd_resp_sendstr_chunk(req, PAGE_MAIN_TAIL);
    if (err != ESP_OK) {
        httpd_resp_sendstr_chunk(req, NULL);
        return err;
    }

    return httpd_resp_sendstr_chunk(req, NULL);
}
