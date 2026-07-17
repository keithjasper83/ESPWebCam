# ESPWebCam – Notification Service

## Overview

The firmware includes an abstraction layer for delivering doorbell events to
an external notification service. The current default implementation is a
no-op (`DisabledNotificationTransport`) that silently discards all events
until a real endpoint is configured.

The intended future service is:

```
https://notifications.kjdev.uk
```

This document describes the proposed API contract so that the service can be
built to match the firmware's expectations.

---

## Interface

```cpp
class INotificationTransport {
public:
    virtual void sendDoorbellEvent(const DoorbellEvent &event) = 0;
    virtual bool isEnabled() const = 0;
};
```

Implementations must:
- Return immediately (non-blocking)
- Queue the event internally if the network is unavailable
- Retry with bounded exponential backoff
- Use the `eventId` field to prevent duplicate delivery
- Discard events older than the configured expiry (see below)

---

## Proposed API Contract

### Endpoint

```
POST https://notifications.kjdev.uk/api/v1/events/doorbell
Authorization: ******
Content-Type: application/json
```

### Request Body

```json
{
  "eventType": "doorbell.pressed",
  "eventId": "front-door-camera-17",
  "deviceId": "front-door-camera",
  "occurredAt": "2026-07-17T14:42:31Z",
  "sequence": 17
}
```

| Field | Type | Description |
|-------|------|-------------|
| `eventType` | string | Always `"doorbell.pressed"` |
| `eventId` | string | `"<deviceId>-<sequence>"` – idempotency key |
| `deviceId` | string | Configured `DEVICE_ID` from `secrets.h` |
| `occurredAt` | string | ISO 8601 UTC timestamp |
| `sequence` | integer | Monotonically increasing per device boot |

### Expected Responses

| Code | Meaning |
|------|---------|
| 200 or 202 | Accepted – do not retry |
| 409 | Duplicate event ID – do not retry |
| 429 | Rate limited – retry after `Retry-After` header |
| 5xx | Server error – retry with backoff |
| 401/403 | Authentication failed – do not retry; log error |

---

## Retry Policy

| Attempt | Delay before retry |
|---------|--------------------|
| 1st     | 5 s |
| 2nd     | 15 s |
| 3rd     | 60 s |
| 4th     | 5 min |
| 5th+    | 15 min (maximum) |

Events older than **30 minutes** are discarded without delivery.
The firmware keeps a bounded in-memory queue of at most 8 events.

---

## Idempotency

The service must accept `eventId` as an idempotency key. If the same
`eventId` is received more than once (due to network retry), the service
should return `409 Conflict` or `200 OK` without delivering a duplicate
notification.

---

## Configuration

The webhook transport is enabled by setting both fields in `secrets.h`:

```c
#define NOTIFICATION_ENDPOINT "https://notifications.kjdev.uk/api/v1/events/doorbell"
#define NOTIFICATION_TOKEN    "your-bearer-token"
```

If either field is empty, the `DisabledNotificationTransport` is used.

---

## Future: Apple Push Notification Service (APNs)

For production iPhone notifications without polling, the notification service
should relay each verified doorbell event to APNs using a device token
registered by the iOS app. The firmware does not need to know about APNs; it
only needs to deliver the event to the webhook endpoint.

The iOS app should:
1. Register for push notifications on first launch.
2. Send the APNs device token to `notifications.kjdev.uk`.
3. Associate the device token with a `deviceId` (e.g. `"front-door-camera"`).
4. The service then sends a push notification for each doorbell event
   targeting that `deviceId`.
