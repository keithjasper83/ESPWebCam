/**
 * DeviceConfig.h
 *
 * Compile-time and runtime device configuration.
 * Runtime values are loaded from NVS via ConfigStore.
 */
#pragma once
#include <stdint.h>
#include "esp_camera.h"

// Firmware version reported in API responses
#define FIRMWARE_VERSION "2.0.0"

// API version prefix
#define API_V1_PREFIX "/api/v1"

// Default mDNS hostname (resolves as espwebcam.local)
#define DEFAULT_HOSTNAME "espwebcam"

// Wi-Fi connection timeout in milliseconds before giving up and retrying
#define WIFI_CONNECT_TIMEOUT_MS  20000

// Maximum number of Wi-Fi reconnect attempts before entering long backoff
#define WIFI_MAX_FAST_RETRIES    5

// Base reconnect backoff in milliseconds (doubles each retry, capped at max)
#define WIFI_BACKOFF_BASE_MS     1000
#define WIFI_BACKOFF_MAX_MS      30000

// Default camera settings
#define DEFAULT_FRAME_SIZE    FRAMESIZE_VGA
#define DEFAULT_JPEG_QUALITY  12          // 1=highest, 63=lowest (sensor convention)
#define DEFAULT_BRIGHTNESS    1
#define DEFAULT_CONTRAST      0
#define DEFAULT_SATURATION    0
#define DEFAULT_AE_LEVEL     -3
#define DEFAULT_XCLK_HZ      20000000

// Camera stream server is on port 81; main API/web server is on port 80
#define HTTP_SERVER_PORT   80
#define STREAM_SERVER_PORT 81

// Maximum JSON body size accepted for PUT/POST requests
#define MAX_REQUEST_BODY_LEN 512

// Doorbell button GPIO (see docs/HARDWARE.md for pin selection rationale)
// GPIO 14 is selected: free on Freenove ESP32-S3 WROOM, not a bootstrap pin,
// not used by camera or SD_MMC, supports internal pull-up.
#define DOORBELL_GPIO         14
#define DOORBELL_ACTIVE_LOW   true   // button connects GPIO to GND
#define DOORBELL_DEBOUNCE_MS  50
#define DOORBELL_MIN_INTERVAL_MS 3000  // minimum ms between registered presses

// Doorbell event queue depth (kept small to limit RAM usage)
#define DOORBELL_EVENT_QUEUE_DEPTH 8

struct DeviceConfig {
    char hostname[64];
    char deviceId[64];
    framesize_t frameSize;
    uint8_t jpegQuality;
    int8_t  brightness;
    int8_t  contrast;
    int8_t  saturation;
    int8_t  aeLevel;
    uint32_t xclkFreqHz;
    int  doorbellGpio;
    bool doorbellActiveLow;
    char notificationEndpoint[256];
    char notificationToken[256];
};

DeviceConfig defaultConfig();
