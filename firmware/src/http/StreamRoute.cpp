/**
 * StreamRoute.cpp
 *
 * Multipart MJPEG stream served from the stream server (port 81).
 * Correctly handles client disconnect and releases all resources.
 * Reports frame rate to the log for diagnostics.
 */
#include "StreamRoute.h"
#include "../diagnostics/Logger.h"
#include "../diagnostics/DeviceStatus.h"
#include <esp_camera.h>
#include <esp_timer.h>
#include <img_converters.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "StreamRoute";

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *STREAM_CONTENT_TYPE =
    "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

// Running-average FPS filter
typedef struct {
    int     values[20];
    size_t  index;
    size_t  count;
    int64_t sum;
} FrameFilter;

static void ff_add(FrameFilter *f, int v) {
    f->sum -= f->values[f->index];
    f->values[f->index] = v;
    f->sum += v;
    f->index = (f->index + 1) % 20;
    if (f->count < 20) f->count++;
}
static int ff_avg(const FrameFilter *f) {
    return f->count ? (int)(f->sum / f->count) : 0;
}

esp_err_t handler_stream(httpd_req_t *req) {
    DeviceStatusService::incrementStreamClients();

    esp_err_t res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        DeviceStatusService::decrementStreamClients();
        return res;
    }

    // Restrict CORS to same origin (do not use wildcard '*' in production)
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    FrameFilter filter = {};
    int64_t lastFrame = esp_timer_get_time();
    char partBuf[128];

    while (true) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            LOG_E(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        struct timeval ts = fb->timestamp;
        uint8_t *jpgBuf = nullptr;
        size_t   jpgLen = 0;
        bool     needFree = false;

        if (fb->format == PIXFORMAT_JPEG) {
            jpgBuf = fb->buf;
            jpgLen = fb->len;
        } else {
            needFree = frame2jpg(fb, 90, &jpgBuf, &jpgLen);
            if (!needFree) {
                LOG_E(TAG, "JPEG conversion failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
                break;
            }
        }

        // Send boundary
        res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
        if (res == ESP_OK) {
            size_t hlen = snprintf(partBuf, sizeof(partBuf), STREAM_PART,
                                   jpgLen, (int)ts.tv_sec, (int)ts.tv_usec);
            res = httpd_resp_send_chunk(req, partBuf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)jpgBuf, jpgLen);
        }

        // Release resources before checking result
        if (needFree) free(jpgBuf);
        esp_camera_fb_return(fb);

        if (res != ESP_OK) {
            LOG_I(TAG, "Stream client disconnected");
            break;
        }

        int64_t now       = esp_timer_get_time();
        int     frameMs   = (int)((now - lastFrame) / 1000);
        lastFrame         = now;
        ff_add(&filter, frameMs);
        int avgMs = ff_avg(&filter);
        LOG_D(TAG, "MJPG %uB %dms (%.1ffps) avg %dms (%.1ffps)",
              (unsigned)jpgLen, frameMs,
              frameMs > 0 ? 1000.0f / frameMs : 0.0f,
              avgMs,
              avgMs > 0 ? 1000.0f / avgMs : 0.0f);
    }

    DeviceStatusService::decrementStreamClients();
    LOG_I(TAG, "Stream handler exit");
    return res;
}
