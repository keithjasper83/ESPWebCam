/**
 * WiFiService.cpp
 */
#include "WiFiService.h"
#include "../diagnostics/Logger.h"
#include <Arduino.h>

static const char *TAG = "WiFiService";

void WiFiService::begin(const char *ssid, const char *password, const char *hostname) {
    _ssid     = ssid;
    _password = password;
    _hostname = hostname;

    WiFi.setHostname(hostname);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  // We manage reconnection ourselves
    startConnect();
}

void WiFiService::loop() {
    switch (_state) {
        case WiFiConnectionState::CONNECTING: {
            if (WiFi.status() == WL_CONNECTED) {
                _state = WiFiConnectionState::CONNECTED;
                _backoffMs       = WIFI_BACKOFF_BASE_MS;
                _fastRetryCount  = 0;
                LOG_I(TAG, "Connected – IP: %s  RSSI: %d dBm",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
            } else if (millis() - _connectStartMs > WIFI_CONNECT_TIMEOUT_MS) {
                LOG_W(TAG, "Connection timeout – entering backoff");
                enterBackoff();
            }
            break;
        }

        case WiFiConnectionState::CONNECTED: {
            if (WiFi.status() != WL_CONNECTED) {
                LOG_W(TAG, "Wi-Fi lost – reconnecting");
                startConnect();
            }
            break;
        }

        case WiFiConnectionState::BACKOFF: {
            if (millis() >= _backoffEndMs) {
                LOG_I(TAG, "Backoff complete – retrying");
                startConnect();
            }
            break;
        }

        case WiFiConnectionState::IDLE:
        default:
            break;
    }
}

void WiFiService::startConnect() {
    _state          = WiFiConnectionState::CONNECTING;
    _connectStartMs = millis();
    // Log SSID but never log the password
    LOG_I(TAG, "Connecting to SSID: %s", _ssid);
    WiFi.begin(_ssid, _password);
}

void WiFiService::enterBackoff() {
    WiFi.disconnect(true);
    _state       = WiFiConnectionState::BACKOFF;
    _backoffEndMs = millis() + _backoffMs;
    LOG_I(TAG, "Backoff for %u ms", _backoffMs);
    _fastRetryCount++;
    // Double backoff up to max
    _backoffMs = min(_backoffMs * 2, (uint32_t)WIFI_BACKOFF_MAX_MS);
}
