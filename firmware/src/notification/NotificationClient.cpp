/**
 * NotificationClient.cpp
 */
#include "NotificationClient.h"
#include "DisabledNotificationTransport.h"
#include "../diagnostics/Logger.h"
#include <string.h>

static const char *TAG = "NotificationClient";

static DisabledNotificationTransport _disabledTransport;

void NotificationClient::begin(const DeviceConfig &cfg) {
    if (strlen(cfg.notificationEndpoint) > 0 &&
        strlen(cfg.notificationToken)    > 0) {
        // Future: instantiate WebhookNotificationTransport here
        // _transport = new WebhookNotificationTransport(cfg.notificationEndpoint,
        //                                               cfg.notificationToken);
        LOG_W(TAG, "Webhook transport not yet implemented – falling back to disabled");
        _transport = &_disabledTransport;
    } else {
        LOG_I(TAG, "No notification endpoint configured – transport disabled");
        _transport = &_disabledTransport;
    }
}

void NotificationClient::loop() {
    // Future: flush the outbound queue and handle retries here
}
