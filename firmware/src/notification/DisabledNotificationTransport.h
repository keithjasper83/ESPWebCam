/**
 * DisabledNotificationTransport.h / .cpp
 *
 * No-op transport used when no notification endpoint is configured.
 * All events are silently discarded.
 */
#pragma once
#include "NotificationTransport.h"

class DisabledNotificationTransport : public INotificationTransport {
public:
    void sendDoorbellEvent(const DoorbellEvent & /*event*/) override {
        // No-op: notification is disabled
    }
    bool isEnabled() const override { return false; }
};
