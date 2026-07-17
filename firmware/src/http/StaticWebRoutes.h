/**
 * StaticWebRoutes.h
 *
 * Serves the static web UI assets embedded into the firmware via
 * PlatformIO's SPIFFS data upload or binary embedding.
 * Assets live in firmware/src/web/ and are embedded as binary objects.
 */
#pragma once
#include "esp_http_server.h"

esp_err_t handler_static_css(httpd_req_t *req);
esp_err_t handler_static_js(httpd_req_t *req);
