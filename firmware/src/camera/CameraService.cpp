/**
 * CameraService.cpp
 */
#include "CameraService.h"
#include "CameraProfiles.h"
#include "../diagnostics/Logger.h"
#include <Arduino.h>

static const char *TAG = "CameraService";

// Allowed framesize range for validation
static const framesize_t MIN_FRAME_SIZE = FRAMESIZE_QVGA;   // 320x240
static const framesize_t MAX_FRAME_SIZE = FRAMESIZE_UXGA;   // 1600x1200

bool CameraService::init(const DeviceConfig &cfg) {
    camera_config_t cam;
    cam.ledc_channel = LEDC_CHANNEL_0;
    cam.ledc_timer   = LEDC_TIMER_0;
    cam.pin_d0    = Y2_GPIO_NUM;
    cam.pin_d1    = Y3_GPIO_NUM;
    cam.pin_d2    = Y4_GPIO_NUM;
    cam.pin_d3    = Y5_GPIO_NUM;
    cam.pin_d4    = Y6_GPIO_NUM;
    cam.pin_d5    = Y7_GPIO_NUM;
    cam.pin_d6    = Y8_GPIO_NUM;
    cam.pin_d7    = Y9_GPIO_NUM;
    cam.pin_xclk  = XCLK_GPIO_NUM;
    cam.pin_pclk  = PCLK_GPIO_NUM;
    cam.pin_vsync = VSYNC_GPIO_NUM;
    cam.pin_href  = HREF_GPIO_NUM;
    cam.pin_sccb_sda = SIOD_GPIO_NUM;
    cam.pin_sccb_scl = SIOC_GPIO_NUM;
    cam.pin_pwdn  = PWDN_GPIO_NUM;
    cam.pin_reset = RESET_GPIO_NUM;
    cam.xclk_freq_hz = cfg.xclkFreqHz;
    cam.frame_size   = cfg.frameSize;
    cam.pixel_format = PIXFORMAT_JPEG;
    cam.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;
    cam.fb_location  = CAMERA_FB_IN_PSRAM;
    cam.jpeg_quality = cfg.jpegQuality;
    cam.fb_count     = 1;

    esp_err_t err = esp_camera_init(&cam);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NOT_SUPPORTED) {
            // Fall back to RGB565 if JPEG not supported by this sensor
            cam.pixel_format = PIXFORMAT_RGB565;
            err = esp_camera_init(&cam);
        }
        if (err != ESP_OK) {
            LOG_E(TAG, "Camera init failed: 0x%x", err);
            return false;
        }
    }

    sensor_t *s = esp_camera_sensor_get();
    if (!s) {
        LOG_E(TAG, "Camera sensor not found after init");
        return false;
    }

    applyDefaultOrientation(s);
    s->set_brightness(s, cfg.brightness);
    s->set_contrast(s,   cfg.contrast);
    s->set_saturation(s, cfg.saturation);
    s->set_ae_level(s,   cfg.aeLevel);

    _initialised = true;
    _applied = currentSettings();
    LOG_I(TAG, "Camera initialised – PID=0x%04x", s->id.PID);
    return true;
}

void CameraService::applyDefaultOrientation(sensor_t *s) {
    uint16_t pid = s->id.PID;
    if (pid == OV2640_PID) {
        s->set_hmirror(s, 1);
        s->set_vflip(s, 1);
    } else if (pid == OV3660_PID) {
        s->set_hmirror(s, 1);
        s->set_vflip(s, 0);
    } else if (pid == GC2145_PID || pid == GC0308_PID) {
        s->set_hmirror(s, 0);
        delay(500);
        s->set_vflip(s, 0);
    } else {
        s->set_hmirror(s, 1);
        s->set_vflip(s, 0);
    }
}

bool CameraService::applySettings(const CameraSettingsRequest &req) {
    if (!_initialised) return false;
    sensor_t *s = esp_camera_sensor_get();
    if (!s) return false;

    // Validate and apply each field that was provided
    if (req.frameSize != -999) {
        framesize_t fs = (framesize_t)req.frameSize;
        if (fs < MIN_FRAME_SIZE || fs > MAX_FRAME_SIZE) {
            LOG_W(TAG, "frameSize %d out of range", req.frameSize);
            return false;
        }
        s->set_framesize(s, fs);
    }
    if (req.jpegQuality != -999) {
        if (req.jpegQuality < 4 || req.jpegQuality > 63) {
            LOG_W(TAG, "jpegQuality %d out of range (4–63)", req.jpegQuality);
            return false;
        }
        s->set_quality(s, req.jpegQuality);
    }
    if (req.brightness != -999) {
        if (req.brightness < -2 || req.brightness > 2) {
            LOG_W(TAG, "brightness %d out of range", req.brightness);
            return false;
        }
        s->set_brightness(s, req.brightness);
    }
    if (req.contrast != -999) {
        if (req.contrast < -2 || req.contrast > 2) {
            LOG_W(TAG, "contrast %d out of range", req.contrast);
            return false;
        }
        s->set_contrast(s, req.contrast);
    }
    if (req.saturation != -999) {
        if (req.saturation < -2 || req.saturation > 2) {
            LOG_W(TAG, "saturation %d out of range", req.saturation);
            return false;
        }
        s->set_saturation(s, req.saturation);
    }
    if (req.aeLevel != -999) {
        if (req.aeLevel < -5 || req.aeLevel > 5) {
            LOG_W(TAG, "aeLevel %d out of range", req.aeLevel);
            return false;
        }
        s->set_ae_level(s, req.aeLevel);
    }
    if (req.hmirror != -999) {
        s->set_hmirror(s, req.hmirror ? 1 : 0);
    }
    if (req.vflip != -999) {
        s->set_vflip(s, req.vflip ? 1 : 0);
    }

    _applied = currentSettings();
    LOG_I(TAG, "Camera settings applied");
    return true;
}

CameraSettingsSnapshot CameraService::currentSettings() const {
    CameraSettingsSnapshot snap = {};
    sensor_t *s = esp_camera_sensor_get();
    if (!s) return snap;
    snap.frameSize   = (framesize_t)s->status.framesize;
    snap.jpegQuality = s->status.quality;
    snap.brightness  = s->status.brightness;
    snap.contrast    = s->status.contrast;
    snap.saturation  = s->status.saturation;
    snap.aeLevel     = s->status.ae_level;
    snap.hmirror     = s->status.hmirror;
    snap.vflip       = s->status.vflip;
    snap.sensorPid   = s->id.PID;
    return snap;
}
