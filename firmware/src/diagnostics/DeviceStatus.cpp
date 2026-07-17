/**
 * DeviceStatus.cpp
 */
#include "DeviceStatus.h"
#include <Arduino.h>
#include <WiFi.h>

volatile uint32_t DeviceStatusService::_streamClients = 0;
volatile uint32_t DeviceStatusService::_doorbellEvents = 0;

DeviceStatus DeviceStatusService::collect() {
    DeviceStatus s;
    s.uptimeSeconds    = millis() / 1000;
    s.freeHeapBytes    = esp_get_free_heap_size();
    s.minFreeHeapBytes = esp_get_minimum_free_heap_size();
    s.streamClients    = _streamClients;
    s.doorbellEventCount = _doorbellEvents;

    if (WiFi.status() == WL_CONNECTED) {
        s.wifiState = WifiState::CONNECTED;
        s.rssi = WiFi.RSSI();
        strncpy(s.ipAddress, WiFi.localIP().toString().c_str(), sizeof(s.ipAddress) - 1);
    } else {
        s.wifiState = WifiState::DISCONNECTED;
    }
    return s;
}

void DeviceStatusService::incrementStreamClients() { _streamClients++; }
void DeviceStatusService::decrementStreamClients() {
    if (_streamClients > 0) _streamClients--;
}
void DeviceStatusService::incrementDoorbellEvents() { _doorbellEvents++; }
