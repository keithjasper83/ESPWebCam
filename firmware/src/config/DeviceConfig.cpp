/**
 * DeviceConfig.cpp
 */
#include "DeviceConfig.h"
#include <string.h>

DeviceConfig defaultConfig() {
    DeviceConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    strncpy(cfg.hostname,  DEFAULT_HOSTNAME, sizeof(cfg.hostname) - 1);
    strncpy(cfg.deviceId,  DEFAULT_HOSTNAME, sizeof(cfg.deviceId) - 1);
    cfg.frameSize      = DEFAULT_FRAME_SIZE;
    cfg.jpegQuality    = DEFAULT_JPEG_QUALITY;
    cfg.brightness     = DEFAULT_BRIGHTNESS;
    cfg.contrast       = DEFAULT_CONTRAST;
    cfg.saturation     = DEFAULT_SATURATION;
    cfg.aeLevel        = DEFAULT_AE_LEVEL;
    cfg.xclkFreqHz     = DEFAULT_XCLK_HZ;
    cfg.doorbellGpio   = DOORBELL_GPIO;
    cfg.doorbellActiveLow = DOORBELL_ACTIVE_LOW;
    cfg.notificationEndpoint[0] = '\0';
    cfg.notificationToken[0]    = '\0';
    return cfg;
}
