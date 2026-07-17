/**
 * ApiRoutes.h
 *
 * Declares handler functions for all /api/v1 routes and the compatibility
 * legacy routes.  All functions are registered by HttpServer.cpp.
 */
#pragma once
#include "esp_http_server.h"
#include "../camera/CameraService.h"
#include "../doorbell/DoorbellEventService.h"
#include "../config/DeviceConfig.h"

// Context passed to every handler via user_ctx
struct ApiContext {
    CameraService        *camera;
    DoorbellEventService *doorbell;
    const DeviceConfig   *cfg;
};

// Helper: send a JSON success envelope
esp_err_t sendJsonOk(httpd_req_t *req, const char *dataJson);

// Helper: send a JSON error envelope
esp_err_t sendJsonError(httpd_req_t *req, int statusCode,
                        const char *errorCode, const char *message);

// Helper: read the full request body into a stack or heap buffer
esp_err_t readBody(httpd_req_t *req, char *buf, size_t bufLen);

// ---- /api/v1 handlers ----
esp_err_t handler_health(httpd_req_t *req);
esp_err_t handler_status(httpd_req_t *req);
esp_err_t handler_camera_settings_get(httpd_req_t *req);
esp_err_t handler_camera_settings_put(httpd_req_t *req);
esp_err_t handler_camera_snapshot(httpd_req_t *req);
esp_err_t handler_doorbell_status(httpd_req_t *req);
esp_err_t handler_doorbell_test(httpd_req_t *req);

// ---- Legacy compatibility aliases ----
// Deprecated – kept for backward compatibility with existing browser clients.
esp_err_t handler_legacy_index(httpd_req_t *req);
esp_err_t handler_legacy_settings_get(httpd_req_t *req);
esp_err_t handler_legacy_settings_post(httpd_req_t *req);
esp_err_t handler_legacy_status(httpd_req_t *req);
