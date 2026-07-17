/**
 * HttpServer.h
 *
 * Owns two httpd_handle_t instances:
 *   - camera_httpd  on HTTP_SERVER_PORT  (80): API, web UI
 *   - stream_httpd  on STREAM_SERVER_PORT (81): MJPEG stream
 */
#pragma once
#include "esp_http_server.h"
#include "../camera/CameraService.h"
#include "../doorbell/DoorbellEventService.h"
#include "../diagnostics/DeviceStatus.h"
#include "../config/DeviceConfig.h"

class HttpServer {
public:
    bool begin(CameraService *camera, DoorbellEventService *doorbell,
               const DeviceConfig *cfg);
    void stop();

private:
    httpd_handle_t _cameraHttpd = nullptr;
    httpd_handle_t _streamHttpd = nullptr;
};
