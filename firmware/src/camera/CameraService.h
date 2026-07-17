/**
 * CameraService.h
 *
 * Owns camera initialisation and settings management.
 * Camera frames are accessed via esp_camera_fb_get / esp_camera_fb_return.
 */
#pragma once
#include "esp_camera.h"
#include "../config/DeviceConfig.h"

struct CameraSettingsRequest {
    // Fields left at -999 are treated as "no change"
    int frameSize    = -999;   // cast to framesize_t
    int jpegQuality  = -999;
    int brightness   = -999;
    int contrast     = -999;
    int saturation   = -999;
    int aeLevel      = -999;
    int hmirror      = -999;
    int vflip        = -999;
};

struct CameraSettingsSnapshot {
    framesize_t frameSize;
    uint8_t jpegQuality;
    int8_t  brightness;
    int8_t  contrast;
    int8_t  saturation;
    int8_t  aeLevel;
    uint8_t hmirror;
    uint8_t vflip;
    uint16_t sensorPid;
};

class CameraService {
public:
    bool init(const DeviceConfig &cfg);
    bool applySettings(const CameraSettingsRequest &req);
    CameraSettingsSnapshot currentSettings() const;
    bool isInitialised() const { return _initialised; }

private:
    bool _initialised = false;
    void applyDefaultOrientation(sensor_t *s);

    // Validated applied values (what was actually set on the sensor)
    mutable CameraSettingsSnapshot _applied = {};
};
