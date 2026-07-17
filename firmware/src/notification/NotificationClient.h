/**
 * NotificationClient.h
 *
 * Selects and owns the active notification transport.
 * Instantiates WebhookNotificationTransport when an endpoint is configured,
 * otherwise uses DisabledNotificationTransport.
 */
#pragma once
#include "NotificationTransport.h"
#include "../config/DeviceConfig.h"

class NotificationClient {
public:
    // Call once during initialisation
    void begin(const DeviceConfig &cfg);

    // Process any queued outgoing events – call from loop()
    void loop();

    INotificationTransport *transport() { return _transport; }

private:
    INotificationTransport *_transport = nullptr;
};
