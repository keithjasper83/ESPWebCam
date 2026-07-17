# ESPWebCam – Security Notes

## ⚠️ Committed Credentials – Action Required

The following Wi-Fi credentials were committed in the original repository
(git commit `5fb8fec`, file `src/main.cpp`):

```
SSID:     KJ iPhone
Password: hello123
```

**These credentials must be treated as compromised.**

If any device or network is still using these credentials:

1. Change the Wi-Fi network password immediately.
2. Review access logs for unexpected connections.
3. Rotate any other credentials that may have been shared through the same
   network (router admin password, etc.).

---

## Current Credential Handling

Wi-Fi credentials are no longer stored in source code.

### How to configure credentials

1. Copy `firmware/include/secrets.h.example` to `firmware/include/secrets.h`.
2. Edit `secrets.h` and fill in your real credentials.
3. `secrets.h` is listed in `.gitignore` and **must never be committed**.

```
firmware/include/
├── secrets.h.example   ← committed template (placeholder values only)
└── secrets.h           ← your real values (gitignored)
```

### What secrets.h contains

```c
#define WIFI_SSID     "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"
#define DEVICE_ID     "front-door-camera"
#define NOTIFICATION_ENDPOINT ""
#define NOTIFICATION_TOKEN    ""
```

---

## Credentials Never Logged

The firmware never prints Wi-Fi passwords or notification tokens to Serial or
any other log output. Only the SSID (network name) is logged during connection
to aid debugging.

---

## API Security

- CORS: The stream server currently sends `Access-Control-Allow-Origin: *`.
  This is acceptable for a local network camera but should be restricted
  to known origins before any internet-facing deployment.

- HTTP only: The firmware does not implement TLS. All communication between
  the iPhone app and the ESP occurs over the local Wi-Fi network. Do not
  expose the ESP to the internet without a TLS-terminating reverse proxy.

- No authentication: API endpoints are currently unauthenticated. This is
  acceptable for a single-user local network device. If multiple untrusted
  users share the Wi-Fi network, consider adding a shared secret header.

---

## Notification Service Credentials

The `NOTIFICATION_ENDPOINT` and `NOTIFICATION_TOKEN` fields in `secrets.h`
are empty by default. They are used only when a webhook notification transport
is configured (Phase 9). Until then, the disabled transport is active and no
network requests are made to external services.

---

## Build Security Checklist

Before flashing or distributing a firmware binary:

- [ ] `secrets.h` exists and contains real credentials
- [ ] `secrets.h` is NOT staged or committed in git
- [ ] Wi-Fi credentials have been changed if `hello123` was ever in use
- [ ] Notification token (if configured) is a secret value not reused elsewhere
