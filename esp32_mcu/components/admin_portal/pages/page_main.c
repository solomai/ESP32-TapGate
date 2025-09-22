#include "page_main.h"

#include <stdio.h>

static const char LOGO_DATA_URI[] =
    "data:image/png;base64,"
    "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";

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

    const char *page =
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
                    "<img class=\"logo\" src=\"%s\" alt=\"TapGate logo\">"
                    "<div class=\"buttons\">"
                        "<button class=\"button\" type=\"button\" disabled>TapGate</button>"
                        "<button class=\"button\" type=\"button\" onclick=\"window.location.href='/api/v1/ap'\">AP setting</button>"
                        "<button class=\"button\" type=\"button\" disabled>WiFi connection</button>"
                        "<button class=\"button\" type=\"button\" disabled>Clients</button>"
                        "<button class=\"button\" type=\"button\" disabled>Events</button>"
                    "</div>"
                    "<div class=\"version\">%s</div>"
                "</div>"
            "</div>"
        "</body>"
        "</html>";

    char buffer[1024];
    int written = snprintf(buffer, sizeof(buffer), page, LOGO_DATA_URI, version);
    if (written < 0 || (size_t)written >= sizeof(buffer)) {
        return ESP_ERR_INVALID_SIZE;
    }

    return httpd_resp_send(req, buffer, written);
}

