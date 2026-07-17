/**
 * ApiRoutes.cpp
 *
 * All /api/v1 handler implementations.
 * Uses ArduinoJson for parsing (no strstr/atoi JSON "parsing").
 */
#include "ApiRoutes.h"
#include "../diagnostics/Logger.h"
#include "../diagnostics/DeviceStatus.h"
#include "../config/DeviceConfig.h"
#include <ArduinoJson.h>
#include <esp_timer.h>
#include <esp_camera.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "ApiRoutes";

// ---- Helpers ----------------------------------------------------------------

esp_err_t sendJsonOk(httpd_req_t *req, const char *dataJson) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    // Simple envelope: {"ok":true,"data":<dataJson>}
    char buf[1024];
    snprintf(buf, sizeof(buf), "{\"ok\":true,\"data\":%s}", dataJson);
    return httpd_resp_send(req, buf, strlen(buf));
}

esp_err_t sendJsonError(httpd_req_t *req, int statusCode,
                        const char *errorCode, const char *message) {
    httpd_resp_set_status(req, statusCode == 400 ? "400 Bad Request"
                             : statusCode == 404 ? "404 Not Found"
                             : statusCode == 405 ? "405 Method Not Allowed"
                             : "500 Internal Server Error");
    httpd_resp_set_type(req, "application/json");
    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"ok\":false,\"error\":{\"code\":\"%s\",\"message\":\"%s\"}}",
             errorCode, message);
    return httpd_resp_send(req, buf, strlen(buf));
}

esp_err_t readBody(httpd_req_t *req, char *buf, size_t bufLen) {
    if (req->content_len == 0) {
        buf[0] = '\0';
        return ESP_OK;
    }
    if (req->content_len >= bufLen) {
        return sendJsonError(req, 400, "REQUEST_TOO_LARGE",
                             "Request body exceeds maximum allowed size");
    }
    int received = httpd_req_recv(req, buf, req->content_len);
    if (received <= 0) {
        return sendJsonError(req, 500, "READ_ERROR", "Failed to read request body");
    }
    buf[received] = '\0';
    return ESP_OK;
}

// Helper to get the ApiContext from any registered handler
static ApiContext *ctx(httpd_req_t *req) {
    return (ApiContext *)req->user_ctx;
}

// ---- Framing-size integer to name mapping -----------------------------------
static const char *frameSizeName(framesize_t fs) {
    switch (fs) {
        case FRAMESIZE_96X96:  return "96x96";
        case FRAMESIZE_QQVGA:  return "QQVGA";
        case FRAMESIZE_QCIF:   return "QCIF";
        case FRAMESIZE_HQVGA:  return "HQVGA";
        case FRAMESIZE_240X240:return "240x240";
        case FRAMESIZE_QVGA:   return "QVGA";
        case FRAMESIZE_CIF:    return "CIF";
        case FRAMESIZE_HVGA:   return "HVGA";
        case FRAMESIZE_VGA:    return "VGA";
        case FRAMESIZE_SVGA:   return "SVGA";
        case FRAMESIZE_XGA:    return "XGA";
        case FRAMESIZE_HD:     return "HD";
        case FRAMESIZE_SXGA:   return "SXGA";
        case FRAMESIZE_UXGA:   return "UXGA";
        default:               return "unknown";
    }
}

// ---- /api/v1/health ---------------------------------------------------------

esp_err_t handler_health(httpd_req_t *req) {
    if (req->method != HTTP_GET)
        return sendJsonError(req, 405, "METHOD_NOT_ALLOWED", "GET required");

    DeviceStatus s = DeviceStatusService::collect();
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"status\":\"ok\",\"uptime\":%u,\"freeHeap\":%u,"
             "\"wifiConnected\":%s,\"streamClients\":%u,"
             "\"firmwareVersion\":\"%s\",\"apiVersion\":\"v1\"}",
             s.uptimeSeconds, s.freeHeapBytes,
             s.wifiState == WifiState::CONNECTED ? "true" : "false",
             s.streamClients,
             FIRMWARE_VERSION);
    return sendJsonOk(req, buf);
}

// ---- /api/v1/status ---------------------------------------------------------

esp_err_t handler_status(httpd_req_t *req) {
    if (req->method != HTTP_GET)
        return sendJsonError(req, 405, "METHOD_NOT_ALLOWED", "GET required");

    DeviceStatus s = DeviceStatusService::collect();
    const DeviceConfig *c = ctx(req)->cfg;
    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"deviceId\":\"%s\",\"hostname\":\"%s\","
             "\"firmwareVersion\":\"%s\",\"apiVersion\":\"v1\","
             "\"uptime\":%u,\"freeHeap\":%u,\"minFreeHeap\":%u,"
             "\"rssi\":%d,\"ipAddress\":\"%s\","
             "\"wifiConnected\":%s,\"streamClients\":%u}",
             c->deviceId, c->hostname,
             FIRMWARE_VERSION,
             s.uptimeSeconds, s.freeHeapBytes, s.minFreeHeapBytes,
             s.rssi, s.ipAddress,
             s.wifiState == WifiState::CONNECTED ? "true" : "false",
             s.streamClients);
    return sendJsonOk(req, buf);
}

// ---- /api/v1/camera/settings (GET) -----------------------------------------

esp_err_t handler_camera_settings_get(httpd_req_t *req) {
    if (req->method != HTTP_GET)
        return sendJsonError(req, 405, "METHOD_NOT_ALLOWED", "GET required");

    CameraService *cam = ctx(req)->camera;
    if (!cam->isInitialised())
        return sendJsonError(req, 500, "CAMERA_NOT_READY", "Camera not initialised");

    CameraSettingsSnapshot s = cam->currentSettings();
    char buf[384];
    snprintf(buf, sizeof(buf),
             "{\"frameSize\":%d,\"frameSizeName\":\"%s\","
             "\"jpegQuality\":%d,\"brightness\":%d,"
             "\"contrast\":%d,\"saturation\":%d,"
             "\"aeLevel\":%d,\"hmirror\":%d,\"vflip\":%d,"
             "\"sensorPid\":\"0x%04X\"}",
             (int)s.frameSize, frameSizeName(s.frameSize),
             s.jpegQuality, s.brightness, s.contrast, s.saturation,
             s.aeLevel, s.hmirror, s.vflip, s.sensorPid);
    return sendJsonOk(req, buf);
}

// ---- /api/v1/camera/settings (PUT) -----------------------------------------

esp_err_t handler_camera_settings_put(httpd_req_t *req) {
    if (req->method != HTTP_PUT)
        return sendJsonError(req, 405, "METHOD_NOT_ALLOWED", "PUT required");

    // Validate Content-Type
    char ct[64] = {};
    if (httpd_req_get_hdr_value_str(req, "Content-Type", ct, sizeof(ct)) == ESP_OK) {
        if (strstr(ct, "application/json") == nullptr) {
            return sendJsonError(req, 400, "INVALID_CONTENT_TYPE",
                                 "Content-Type must be application/json");
        }
    }

    char body[MAX_REQUEST_BODY_LEN + 1] = {};
    esp_err_t err = readBody(req, body, sizeof(body));
    if (err != ESP_OK) return err;

    // Parse with ArduinoJson
    JsonDocument doc;
    DeserializationError jsonErr = deserializeJson(doc, body);
    if (jsonErr) {
        return sendJsonError(req, 400, "INVALID_JSON",
                             "Request body is not valid JSON");
    }

    CameraSettingsRequest sreq;
    if (doc["frameSize"].is<int>())    sreq.frameSize   = doc["frameSize"].as<int>();
    if (doc["jpegQuality"].is<int>())  sreq.jpegQuality = doc["jpegQuality"].as<int>();
    if (doc["brightness"].is<int>())   sreq.brightness  = doc["brightness"].as<int>();
    if (doc["contrast"].is<int>())     sreq.contrast    = doc["contrast"].as<int>();
    if (doc["saturation"].is<int>())   sreq.saturation  = doc["saturation"].as<int>();
    if (doc["aeLevel"].is<int>())      sreq.aeLevel     = doc["aeLevel"].as<int>();
    if (doc["hmirror"].is<int>())      sreq.hmirror     = doc["hmirror"].as<int>();
    if (doc["vflip"].is<int>())        sreq.vflip       = doc["vflip"].as<int>();

    CameraService *cam = ctx(req)->camera;
    if (!cam->applySettings(sreq)) {
        return sendJsonError(req, 400, "INVALID_CAMERA_SETTING",
                             "One or more settings are out of valid range");
    }

    // Return the actually-applied values (not just what was requested)
    return handler_camera_settings_get(req);
}

// ---- /api/v1/camera/snapshot ------------------------------------------------

esp_err_t handler_camera_snapshot(httpd_req_t *req) {
    if (req->method != HTTP_GET)
        return sendJsonError(req, 405, "METHOD_NOT_ALLOWED", "GET required");

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        return sendJsonError(req, 500, "CAPTURE_FAILED", "Camera capture failed");
    }

    esp_err_t res = ESP_OK;
    if (fb->format == PIXFORMAT_JPEG) {
        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_set_hdr(req, "Content-Disposition",
                           "inline; filename=\"snapshot.jpg\"");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    } else {
        // Convert non-JPEG format
        uint8_t *jpg_buf = nullptr;
        size_t   jpg_len = 0;
        if (frame2jpg(fb, 90, &jpg_buf, &jpg_len)) {
            httpd_resp_set_type(req, "image/jpeg");
            httpd_resp_set_hdr(req, "Content-Disposition",
                               "inline; filename=\"snapshot.jpg\"");
            httpd_resp_set_hdr(req, "Cache-Control", "no-store");
            res = httpd_resp_send(req, (const char *)jpg_buf, jpg_len);
            free(jpg_buf);
        } else {
            res = sendJsonError(req, 500, "CONVERSION_FAILED",
                                "JPEG conversion failed");
        }
    }

    esp_camera_fb_return(fb);
    return res;
}

// ---- /api/v1/doorbell/status ------------------------------------------------

esp_err_t handler_doorbell_status(httpd_req_t *req) {
    if (req->method != HTTP_GET)
        return sendJsonError(req, 405, "METHOD_NOT_ALLOWED", "GET required");

    DoorbellEventService *db = ctx(req)->doorbell;
    const DoorbellEvent  *ev = db->lastEvent();
    char buf[384];
    if (ev) {
        snprintf(buf, sizeof(buf),
                 "{\"totalEvents\":%u,\"lastEvent\":{"
                 "\"eventType\":\"%s\",\"eventId\":\"%s\","
                 "\"deviceId\":\"%s\",\"occurredAtMs\":%u,"
                 "\"sequence\":%u}}",
                 db->totalEvents(),
                 ev->eventType, ev->eventId,
                 ev->deviceId,  ev->occurredAtMs, ev->sequence);
    } else {
        snprintf(buf, sizeof(buf),
                 "{\"totalEvents\":0,\"lastEvent\":null}");
    }
    return sendJsonOk(req, buf);
}

// ---- /api/v1/doorbell/test --------------------------------------------------

esp_err_t handler_doorbell_test(httpd_req_t *req) {
    if (req->method != HTTP_POST)
        return sendJsonError(req, 405, "METHOD_NOT_ALLOWED", "POST required");

    DoorbellEventService *db = ctx(req)->doorbell;
    DoorbellPressEvent fakePress;
    fakePress.sequenceNumber = 0;  // DoorbellEventService will increment
    fakePress.timestamp = (uint32_t)(esp_timer_get_time() / 1000);
    db->onButtonPress(fakePress);

    return sendJsonOk(req, "{\"triggered\":true}");
}

// ---- Legacy compatibility handlers -----------------------------------------
// These redirect or proxy to the v1 equivalents so existing browser UIs
// continue to work.  Marked deprecated in docs/API.md.

esp_err_t handler_legacy_index(httpd_req_t *req) {
    // Serve the new web UI (web assets are embedded as raw strings)
    extern const char INDEX_HTML_START[] asm("_binary_index_html_start");
    extern const char INDEX_HTML_END[]   asm("_binary_index_html_end");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, INDEX_HTML_START,
                           INDEX_HTML_END - INDEX_HTML_START);
}

esp_err_t handler_legacy_settings_get(httpd_req_t *req) {
    // Proxy to v1 (same handler, same context)
    return handler_camera_settings_get(req);
}

esp_err_t handler_legacy_settings_post(httpd_req_t *req) {
    // The old POST /settings accepted JSON identical to PUT /api/v1/camera/settings
    // Reuse the v1 handler but tolerate missing Content-Type (legacy clients)
    CameraService *cam = ctx(req)->camera;
    char body[MAX_REQUEST_BODY_LEN + 1] = {};
    if (readBody(req, body, sizeof(body)) != ESP_OK) {
        return handler_camera_settings_get(req);  // Return current settings on error
    }

    JsonDocument doc;
    if (deserializeJson(doc, body) != DeserializationError::Ok) {
        return sendJsonError(req, 400, "INVALID_JSON", "Invalid JSON body");
    }

    CameraSettingsRequest sreq;
    // Support old field name "frame_size" as string (legacy) and new frameSize as int
    if (doc["frameSize"].is<int>()) {
        sreq.frameSize = doc["frameSize"].as<int>();
    } else if (doc["frame_size"].is<const char *>()) {
        const char *fs = doc["frame_size"];
        if      (strcmp(fs, "QVGA") == 0) sreq.frameSize = FRAMESIZE_QVGA;
        else if (strcmp(fs, "HVGA") == 0) sreq.frameSize = FRAMESIZE_HVGA;
        else if (strcmp(fs, "SVGA") == 0) sreq.frameSize = FRAMESIZE_SVGA;
        else if (strcmp(fs, "XGA")  == 0) sreq.frameSize = FRAMESIZE_XGA;
        else                              sreq.frameSize = FRAMESIZE_VGA;
    }
    if (doc["jpegQuality"].is<int>())  sreq.jpegQuality = doc["jpegQuality"].as<int>();
    if (doc["jpeg_quality"].is<int>()) sreq.jpegQuality = doc["jpeg_quality"].as<int>();

    cam->applySettings(sreq);
    char ok[] = "{\"status\":\"ok\"}";
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, ok, strlen(ok));
}

esp_err_t handler_legacy_status(httpd_req_t *req) {
    return handler_status(req);
}
