/**
 * WiFiService.h
 *
 * Non-blocking Wi-Fi connection with bounded exponential backoff.
 * Never logs the Wi-Fi password.
 */
#pragma once
#include <WiFi.h>
#include "../config/DeviceConfig.h"

enum class WiFiConnectionState {
    IDLE,
    CONNECTING,
    CONNECTED,
    BACKOFF
};

class WiFiService {
public:
    void begin(const char *ssid, const char *password, const char *hostname);
    void loop();   // call from the application loop – non-blocking

    WiFiConnectionState state() const { return _state; }
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    String ipAddress() const { return WiFi.localIP().toString(); }

private:
    void startConnect();
    void enterBackoff();

    const char *_ssid     = nullptr;
    const char *_password = nullptr;
    const char *_hostname = nullptr;

    WiFiConnectionState _state = WiFiConnectionState::IDLE;
    uint32_t _connectStartMs  = 0;
    uint32_t _backoffEndMs    = 0;
    uint32_t _backoffMs       = WIFI_BACKOFF_BASE_MS;
    uint8_t  _fastRetryCount  = 0;
};
