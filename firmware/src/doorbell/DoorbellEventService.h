/**
 * DoorbellEventService.h
 *
 * Produces structured doorbell events and forwards them to any registered
 * notification transport.  Does not trigger recording.
 */
#pragma once
#include <stdint.h>
#include "DoorbellButton.h"

struct DoorbellEvent {
    char     eventType[32];   // "doorbell.pressed"
    char     eventId[48];     // device-id + sequence, e.g. "front-door-camera-17"
    char     deviceId[64];
    uint32_t occurredAtMs;    // millis()
    uint32_t sequence;
};

class INotificationTransport;  // forward declaration

class DoorbellEventService {
public:
    void begin(const char *deviceId, INotificationTransport *transport);

    // Called by DoorbellButton on a confirmed press
    void onButtonPress(const DoorbellPressEvent &press);

    // Returns the last emitted event (or nullptr if none yet)
    const DoorbellEvent *lastEvent() const;
    uint32_t totalEvents() const { return _totalEvents; }

private:
    char     _deviceId[64] = {};
    INotificationTransport *_transport = nullptr;
    DoorbellEvent _lastEvent = {};
    bool     _hasEvent    = false;
    uint32_t _totalEvents = 0;
};
