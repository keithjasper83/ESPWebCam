/**
 * DoorbellEventService.cpp
 */
#include "DoorbellEventService.h"
#include "../notification/NotificationTransport.h"
#include "../diagnostics/Logger.h"
#include "../diagnostics/DeviceStatus.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "DoorbellEventService";

void DoorbellEventService::begin(const char *deviceId,
                                  INotificationTransport *transport) {
    strncpy(_deviceId, deviceId, sizeof(_deviceId) - 1);
    _transport = transport;
}

void DoorbellEventService::onButtonPress(const DoorbellPressEvent &press) {
    _totalEvents++;
    DeviceStatusService::incrementDoorbellEvents();

    memset(&_lastEvent, 0, sizeof(_lastEvent));
    strncpy(_lastEvent.eventType, "doorbell.pressed", sizeof(_lastEvent.eventType) - 1);
    snprintf(_lastEvent.eventId, sizeof(_lastEvent.eventId),
             "%s-%u", _deviceId, press.sequenceNumber);
    strncpy(_lastEvent.deviceId, _deviceId, sizeof(_lastEvent.deviceId) - 1);
    _lastEvent.occurredAtMs = press.timestamp;
    _lastEvent.sequence     = press.sequenceNumber;
    _hasEvent = true;

    LOG_I(TAG, "Doorbell event – id=%s seq=%u",
          _lastEvent.eventId, _lastEvent.sequence);

    // Dispatch to notification transport (non-blocking; transport handles queuing)
    if (_transport) {
        _transport->sendDoorbellEvent(_lastEvent);
    }
}

const DoorbellEvent *DoorbellEventService::lastEvent() const {
    return _hasEvent ? &_lastEvent : nullptr;
}
