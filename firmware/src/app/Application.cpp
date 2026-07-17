/**
 * Application.cpp
 *
 * Wires all services together. Manages the startup sequence and the
 * per-loop calls.  This is the only place where startup order matters.
 */
#include "Application.h"
#include "../diagnostics/Logger.h"
#include <Arduino.h>

static const char *TAG = "Application";

void Application::setup(const char *ssid, const char *password) {
    Serial.begin(115200);
    delay(500);
    Serial.setDebugOutput(true);
    Serial.println();

    LOG_I(TAG, "ESPWebCam firmware v%s starting", FIRMWARE_VERSION);

    // 1. Load config from NVS (falls back to compiled defaults)
    _cfg = ConfigStore::load();
    LOG_I(TAG, "Device ID: %s  Hostname: %s", _cfg.deviceId, _cfg.hostname);

    // 2. Initialise camera before network (camera init is slow)
    if (!_camera.init(_cfg)) {
        LOG_E(TAG, "Camera initialisation failed – continuing without camera");
    }

    // 3. Start Wi-Fi (non-blocking; actual connection happens in loop())
    _wifi.begin(ssid, password, _cfg.hostname);

    // 4. Initialise notification transport
    _notifications.begin(_cfg);

    // 5. Wire doorbell event service to notification client
    _doorbellEvents.begin(_cfg.deviceId, _notifications.transport());

    // 6. Attach physical doorbell button
    _doorbellButton.begin(
        _cfg.doorbellGpio,
        _cfg.doorbellActiveLow,
        DOORBELL_DEBOUNCE_MS,
        DOORBELL_MIN_INTERVAL_MS,
        [this](const DoorbellPressEvent &e) {
            _doorbellEvents.onButtonPress(e);
        }
    );

    LOG_I(TAG, "Setup complete – waiting for Wi-Fi");
}

void Application::loop() {
    // 1. Maintain Wi-Fi connection (non-blocking backoff reconnect)
    _wifi.loop();

    // 2. Start dependent services once Wi-Fi is connected
    if (_wifi.isConnected() && !_httpStarted) {
        _httpStarted = true;
        if (_httpServer.begin(&_camera, &_doorbellEvents, &_cfg)) {
            LOG_I(TAG, "HTTP servers started – http://%s",
                  _wifi.ipAddress().c_str());
        } else {
            LOG_E(TAG, "HTTP server start failed");
        }
    }

    if (_wifi.isConnected() && !_mdnsStarted) {
        _mdnsStarted = true;
        if (_mdns.begin(_cfg.hostname, _cfg.deviceId,
                        HTTP_SERVER_PORT, STREAM_SERVER_PORT)) {
            LOG_I(TAG, "mDNS: %s.local", _cfg.hostname);
        }
    }

    // 3. Process doorbell button (debounce runs here, not in ISR)
    _doorbellButton.loop();

    // 4. Process notification queue
    _notifications.loop();
}
