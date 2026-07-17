/**
 * NotificationTransport.h
 *
 * Abstract interface for delivering doorbell events to a notification service.
 * The production implementation (WebhookNotificationTransport) is configured
 * at runtime with an endpoint URL and bearer token.
 *
 * The default implementation (DisabledNotificationTransport) is a no-op and
 * is used until a notification endpoint is explicitly configured.
 */
#pragma once
#include "../doorbell/DoorbellEventService.h"

/**
 * Forward declaration so DoorbellEventService.h can reference this interface
 * without a circular include.  The struct DoorbellEvent is already defined
 * in DoorbellEventService.h.
 */

class INotificationTransport {
public:
    virtual ~INotificationTransport() = default;

    /**
     * Enqueue or send a doorbell event asynchronously.
     * Must return quickly – must not block the calling thread.
     * Must not duplicate notifications from a single press.
     */
    virtual void sendDoorbellEvent(const DoorbellEvent &event) = 0;

    virtual bool isEnabled() const = 0;
};
