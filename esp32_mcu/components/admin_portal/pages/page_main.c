#include "page_main.h"

#include <stdio.h>
#include <string.h>

static const char LOGO_DATA_URI[] =
    "data:image/png;base64,"
    "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";

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
            ".logo{height:calc(var(--button-height)*2);object-fit:contain;}"
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
                "<img class=\"logo\" src=\"";

static const char PAGE_MAIN_AFTER_LOGO[] =
                "\" alt=\"TapGate logo\">"
                "<div class=\"buttons\">"
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

    err = httpd_resp_sendstr_chunk(req, LOGO_DATA_URI);
    if (err != ESP_OK) {
        httpd_resp_sendstr_chunk(req, NULL);
        return err;
    }

    err = httpd_resp_sendstr_chunk(req, PAGE_MAIN_AFTER_LOGO);
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
