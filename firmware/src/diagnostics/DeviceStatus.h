/**
 * DeviceStatus.h
 *
 * Collects runtime health information for the /api/v1/health endpoint.
 */
#pragma once
#include <stdint.h>

enum class WifiState { DISCONNECTED, CONNECTING, CONNECTED };
enum class StreamState { IDLE, STREAMING };

struct DeviceStatus {
    WifiState  wifiState   = WifiState::DISCONNECTED;
    StreamState streamState = StreamState::IDLE;
    uint32_t   uptimeSeconds = 0;
    uint32_t   freeHeapBytes = 0;
    uint32_t   minFreeHeapBytes = 0;
    int        rssi          = 0;
    char       ipAddress[16] = {};
    uint32_t   streamClients = 0;
    uint32_t   doorbellEventCount = 0;
};

class DeviceStatusService {
public:
    static DeviceStatus collect();
    static void incrementStreamClients();
    static void decrementStreamClients();
    static void incrementDoorbellEvents();

private:
    static volatile uint32_t _streamClients;
    static volatile uint32_t _doorbellEvents;
};
