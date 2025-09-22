#include "page_ap.h"

#include <stdio.h>
#include <string.h>

static void html_escape(const char *input, char *output, size_t output_len)
{
    if (!input || !output || output_len == 0) {
        return;
    }

    size_t out_idx = 0;
    for (size_t i = 0; input[i] != '\0' && out_idx + 1 < output_len; ++i) {
        char c = input[i];
        const char *entity = NULL;
        switch (c) {
            case '&': entity = "&amp;"; break;
            case '<': entity = "&lt;"; break;
            case '>': entity = "&gt;"; break;
            case '"': entity = "&quot;"; break;
            case '\'': entity = "&#39;"; break;
            default: break;
        }

        if (entity) {
            size_t entity_len = strlen(entity);
            if (out_idx + entity_len < output_len) {
                memcpy(&output[out_idx], entity, entity_len);
                out_idx += entity_len;
            } else {
                break;
            }
        } else {
            output[out_idx++] = c;
        }
    }
    output[out_idx] = '\0';
}

esp_err_t page_ap_render(httpd_req_t *req, const page_ap_context_t *context)
{
    if (!req || !context) {
        return ESP_ERR_INVALID_ARG;
    }

    char ssid_escaped[96];
    char password_escaped[96];
    char status_escaped[160];

    html_escape(context->ssid ? context->ssid : "", ssid_escaped, sizeof(ssid_escaped));
    html_escape(context->password ? context->password : "", password_escaped, sizeof(password_escaped));
    html_escape(context->status_message ? context->status_message : "", status_escaped, sizeof(status_escaped));

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");

    const char *status_class = context->status_is_error ? "status error" : "status";
    const char *ssid_field_class = context->highlight_ssid ? "field error" : "field";
    const char *password_field_class = context->highlight_password ? "field error" : "field";

    const char *cancel_visibility = context->show_cancel ? "flex" : "none";

    const char *head =
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
            "<meta charset=\"UTF-8\">"
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
            "<title>TapGate AP Setup</title>"
            "<style>"
                "*{box-sizing:border-box;font-family:'Inter','Segoe UI',sans-serif;}"
                ":root{--bg:#0D1520;--card:#111927;--accent:#3B82F6;--text:#E6EEF8;--muted:#9CA3AF;"
                "--input-border:#2A3642;--error:#F87171;--button-height:48px;}"
                "body{margin:0;background:var(--bg);color:var(--text);min-height:100vh;}"
                ".page{min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px;}"
                ".card{width:100%;max-width:420px;background:var(--card);border-radius:20px;"
                "padding:28px 24px;box-shadow:0 20px 45px rgba(8,12,24,0.45);}"
                ".card h1{margin:0 0 24px;font-size:24px;font-weight:600;text-align:center;}"
                ".field{display:flex;flex-direction:column;margin-bottom:20px;}"
                ".field label{font-size:14px;color:var(--muted);margin-bottom:6px;font-weight:500;transition:color .2s;}"
                ".field input{height:var(--button-height);padding:0 16px;border-radius:12px;border:1px solid var(--input-border);"
                "background:linear-gradient(180deg,#10161D 0%,#0C1117 100%);color:var(--text);font-size:16px;"
                "transition:border .2s,box-shadow .2s;}"
                ".field input::placeholder{color:rgba(122,138,159,0.7);}"
                ".field:focus-within label{color:var(--accent);}"
                ".field input:focus{outline:none;border-color:var(--accent);box-shadow:0 0 0 3px rgba(59,130,246,0.35);}"
                ".field.error label{color:var(--error);}"
                ".field.error input{border-color:var(--error);}"
                ".actions{display:flex;flex-direction:column;gap:14px;margin-top:10px;}"
                ".button{height:var(--button-height);border:none;border-radius:14px;"
                "background:linear-gradient(135deg,#0b2451 0%,#1c4c8c 45%,#3a8fdd 100%);"
                "color:#fff;font-size:17px;font-weight:600;letter-spacing:.3px;"
                "box-shadow:0 4px 12px rgba(0,0,0,0.3);cursor:pointer;transition:transform .15s,box-shadow .15s,filter .15s;}"
                ".button:hover,.button:focus{filter:brightness(1.08);outline:none;}"
                ".button:active{transform:translateY(1px);box-shadow:0 2px 6px rgba(0,0,0,0.25);filter:brightness(.92);}"
                ".button.secondary{background:transparent;border:1px solid rgba(255,255,255,0.2);"
                "box-shadow:none;}"
                ".button[disabled]{opacity:.55;cursor:not-allowed;box-shadow:none;}"
                ".status{min-height:22px;margin-top:6px;font-size:14px;color:var(--muted);text-align:center;}"
                ".status.error{color:var(--error);}"
                "form{display:flex;flex-direction:column;}"
            "</style>"
        "</head>"
        "<body>"
            "<div class=\"page\">"
                "<div class=\"card\">"
                    "<h1>Access Point Setup</h1>"
                    "<form method=\"post\" action=\"/api/v1/ap\" autocomplete=\"off\">";

    esp_err_t err = httpd_resp_send_chunk(req, head, strlen(head));
    if (err != ESP_OK) {
        httpd_resp_sendstr_chunk(req, NULL);
        return err;
    }

    char field_buffer[512];
    int written = snprintf(field_buffer, sizeof(field_buffer),
        "<div class=\"%s\">"
            "<label for=\"ssid\">AP SSID</label>"
            "<input id=\"ssid\" name=\"ssid\" type=\"text\" maxlength=\"32\" value=\"%s\" required>"
        "</div>",
        ssid_field_class,
        ssid_escaped);
    if (written < 0 || (size_t)written >= sizeof(field_buffer)) {
        httpd_resp_sendstr_chunk(req, NULL);
        return ESP_ERR_INVALID_SIZE;
    }
    err = httpd_resp_send_chunk(req, field_buffer, written);
    if (err != ESP_OK) {
        httpd_resp_sendstr_chunk(req, NULL);
        return err;
    }

    written = snprintf(field_buffer, sizeof(field_buffer),
        "<div class=\"%s\">"
            "<label for=\"password\">AP Password</label>"
            "<input id=\"password\" name=\"password\" type=\"password\" maxlength=\"64\" value=\"%s\" required>"
        "</div>",
        password_field_class,
        password_escaped);
    if (written < 0 || (size_t)written >= sizeof(field_buffer)) {
        httpd_resp_sendstr_chunk(req, NULL);
        return ESP_ERR_INVALID_SIZE;
    }
    err = httpd_resp_send_chunk(req, field_buffer, written);
    if (err != ESP_OK) {
        httpd_resp_sendstr_chunk(req, NULL);
        return err;
    }

    written = snprintf(field_buffer, sizeof(field_buffer),
        "<div class=\"%s\">%s</div>"
        "<div class=\"actions\">"
            "<button class=\"button\" type=\"submit\">OK</button>"
            "<button class=\"button secondary\" type=\"button\" style=\"display:%s;\" onclick=\"window.location.href='/api/v1/main'\">Cancel</button>"
        "</div>"
        "</form>"
        "</div>"
        "</div>"
        "</body>"
        "</html>",
        status_class,
        status_escaped,
        cancel_visibility);
    if (written < 0 || (size_t)written >= sizeof(field_buffer)) {
        httpd_resp_sendstr_chunk(req, NULL);
        return ESP_ERR_INVALID_SIZE;
    }

    err = httpd_resp_send_chunk(req, field_buffer, written);
    if (err != ESP_OK) {
        httpd_resp_sendstr_chunk(req, NULL);
        return err;
    }

    return httpd_resp_sendstr_chunk(req, NULL);
}

