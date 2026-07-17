/**
 * StaticWebRoutes.cpp
 *
 * Serves styles.css and app.js, both embedded as binary sections via
 * PlatformIO's component_embed_txtfiles build option.
 */
#include "StaticWebRoutes.h"
#include <string.h>
#include "esp_http_server.h"

extern const char STYLES_CSS_START[] asm("_binary_styles_css_start");
extern const char STYLES_CSS_END[]   asm("_binary_styles_css_end");
extern const char APP_JS_START[]     asm("_binary_app_js_start");
extern const char APP_JS_END[]       asm("_binary_app_js_end");

esp_err_t handler_static_css(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/css");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");
    return httpd_resp_send(req, STYLES_CSS_START,
                           STYLES_CSS_END - STYLES_CSS_START);
}

esp_err_t handler_static_js(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");
    return httpd_resp_send(req, APP_JS_START,
                           APP_JS_END - APP_JS_START);
}
