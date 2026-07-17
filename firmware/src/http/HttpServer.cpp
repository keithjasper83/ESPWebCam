/**
 * HttpServer.cpp
 *
 * Starts the API/web server (port 80) and stream server (port 81).
 * Registers all routes and passes the shared ApiContext to every handler.
 */
#include "HttpServer.h"
#include "ApiRoutes.h"
#include "StreamRoute.h"
#include "StaticWebRoutes.h"
#include "../diagnostics/Logger.h"

static const char *TAG = "HttpServer";

bool HttpServer::begin(CameraService *camera, DoorbellEventService *doorbell,
                       const DeviceConfig *cfg) {
    // Shared context for all API handlers
    static ApiContext apiCtx;
    apiCtx.camera   = camera;
    apiCtx.doorbell = doorbell;
    apiCtx.cfg      = cfg;

    // ---- API / web server (port 80) ----------------------------------------
    httpd_config_t apicfg = HTTPD_DEFAULT_CONFIG();
    apicfg.server_port    = HTTP_SERVER_PORT;
    apicfg.max_uri_handlers = 32;

    LOG_I(TAG, "Starting API server on port %d", apicfg.server_port);
    if (httpd_start(&_cameraHttpd, &apicfg) != ESP_OK) {
        LOG_E(TAG, "Failed to start API server");
        return false;
    }

    // ---- v1 routes ----------------------------------------------------------
    httpd_uri_t health_uri = {
        API_V1_PREFIX "/health", HTTP_GET, handler_health, &apiCtx };
    httpd_uri_t status_uri = {
        API_V1_PREFIX "/status", HTTP_GET, handler_status, &apiCtx };
    httpd_uri_t cam_settings_get = {
        API_V1_PREFIX "/camera/settings", HTTP_GET,
        handler_camera_settings_get, &apiCtx };
    httpd_uri_t cam_settings_put = {
        API_V1_PREFIX "/camera/settings", HTTP_PUT,
        handler_camera_settings_put, &apiCtx };
    httpd_uri_t cam_snapshot = {
        API_V1_PREFIX "/camera/snapshot", HTTP_GET,
        handler_camera_snapshot, &apiCtx };
    httpd_uri_t doorbell_status = {
        API_V1_PREFIX "/doorbell/status", HTTP_GET,
        handler_doorbell_status, &apiCtx };
    httpd_uri_t doorbell_test = {
        API_V1_PREFIX "/doorbell/test", HTTP_POST,
        handler_doorbell_test, &apiCtx };

    httpd_register_uri_handler(_cameraHttpd, &health_uri);
    httpd_register_uri_handler(_cameraHttpd, &status_uri);
    httpd_register_uri_handler(_cameraHttpd, &cam_settings_get);
    httpd_register_uri_handler(_cameraHttpd, &cam_settings_put);
    httpd_register_uri_handler(_cameraHttpd, &cam_snapshot);
    httpd_register_uri_handler(_cameraHttpd, &doorbell_status);
    httpd_register_uri_handler(_cameraHttpd, &doorbell_test);

    // ---- Legacy compatibility routes (deprecated) ---------------------------
    httpd_uri_t legacy_index = {
        "/", HTTP_GET, handler_legacy_index, &apiCtx };
    httpd_uri_t legacy_settings_get = {
        "/settings", HTTP_GET, handler_legacy_settings_get, &apiCtx };
    httpd_uri_t legacy_settings_post = {
        "/settings", HTTP_POST, handler_legacy_settings_post, &apiCtx };
    httpd_uri_t legacy_status = {
        "/status", HTTP_GET, handler_legacy_status, &apiCtx };

    httpd_register_uri_handler(_cameraHttpd, &legacy_index);
    httpd_register_uri_handler(_cameraHttpd, &legacy_settings_get);
    httpd_register_uri_handler(_cameraHttpd, &legacy_settings_post);
    httpd_register_uri_handler(_cameraHttpd, &legacy_status);

    // Static assets
    httpd_uri_t css_uri = { "/styles.css", HTTP_GET, handler_static_css, nullptr };
    httpd_uri_t js_uri  = { "/app.js",     HTTP_GET, handler_static_js,  nullptr };
    httpd_register_uri_handler(_cameraHttpd, &css_uri);
    httpd_register_uri_handler(_cameraHttpd, &js_uri);

    // ---- Stream server (port 81) --------------------------------------------
    httpd_config_t streamcfg = HTTPD_DEFAULT_CONFIG();
    streamcfg.server_port = STREAM_SERVER_PORT;
    streamcfg.ctrl_port   = streamcfg.ctrl_port + 1;
    streamcfg.max_uri_handlers = 4;

    LOG_I(TAG, "Starting stream server on port %d", streamcfg.server_port);
    if (httpd_start(&_streamHttpd, &streamcfg) != ESP_OK) {
        LOG_E(TAG, "Failed to start stream server");
        return false;
    }

    httpd_uri_t stream_uri = {
        "/stream", HTTP_GET, handler_stream, nullptr };
    // Also serve stream at /api/v1/camera/stream for API consistency
    httpd_uri_t stream_v1_uri = {
        API_V1_PREFIX "/camera/stream", HTTP_GET, handler_stream, nullptr };

    httpd_register_uri_handler(_streamHttpd, &stream_uri);
    httpd_register_uri_handler(_streamHttpd, &stream_v1_uri);

    LOG_I(TAG, "HTTP servers started");
    return true;
}

void HttpServer::stop() {
    if (_cameraHttpd) { httpd_stop(_cameraHttpd); _cameraHttpd = nullptr; }
    if (_streamHttpd) { httpd_stop(_streamHttpd); _streamHttpd = nullptr; }
}
