/**
 * StreamRoute.h / .cpp
 *
 * MJPEG multipart stream handler.
 * Runs on stream_httpd (port 81) to isolate the long-lived stream connection
 * from the API server.
 */
#pragma once
#include "esp_http_server.h"

esp_err_t handler_stream(httpd_req_t *req);
