# ESPWebCam – Architecture Overview

## Repository Layout

```
ESPWebCam/
├── firmware/          ESP32-S3 PlatformIO/Arduino firmware
│   ├── platformio.ini
│   ├── include/       secrets.h.example (copy to secrets.h before building)
│   └── src/
│       ├── main.cpp   Composition root only
│       ├── app/       Application wiring
│       ├── camera/    CameraService – init, settings, frame capture
│       ├── network/   WiFiService (backoff reconnect), MdnsService
│       ├── http/      HttpServer, ApiRoutes (v1), StreamRoute, StaticWebRoutes
│       ├── doorbell/  DoorbellButton (ISR+debounce), DoorbellEventService
│       ├── notification/ Transport abstraction + disabled/webhook transports
│       ├── config/    DeviceConfig, ConfigStore (NVS persistence)
│       ├── diagnostics/ DeviceStatus, Logger
│       └── web/       index.html, styles.css, app.js (embedded as binary)
├── ios/               Native Swift/SwiftUI iPhone application
├── shared/            openapi.yaml – versioned API contract
├── docs/              Architecture, API, hardware, iOS, notifications, security
└── .github/workflows/ CI stubs
```

## System Overview

```
┌──────────────────────────┐         ┌──────────────────────────┐
│   Freenove ESP32-S3      │         │   iPhone App             │
│   WROOM + OV2640         │◄──────►│   ESPWebCam (SwiftUI)    │
│                          │  Wi-Fi  │                          │
│  ┌────────────────────┐  │         │  ┌────────────────────┐  │
│  │ CameraService      │  │         │  │ MJPEGStreamClient  │  │
│  │ HttpServer (p.80)  │──┼──MJPEG─►│  │ CameraViewModel    │  │
│  │ StreamServer (p.81)│  │         │  │ RecordingService   │  │
│  │ WiFiService        │  │         │  │ ScreenshotService  │  │
│  │ DoorbellButton     │  │         │  │ CameraAPIClient    │  │
│  │ DoorbellEventSvc   │  │         │  └────────────────────┘  │
│  │ NotificationClient │  │         └──────────────────────────┘
│  └────────────────────┘  │
└──────────────────────────┘
         │ future
         ▼
notifications.kjdev.uk
```

## Design Decisions

### Two HTTP servers
The firmware runs two `httpd_handle_t` instances:
- **Port 80**: API (`/api/v1/*`), web UI, legacy routes
- **Port 81**: MJPEG stream only

This isolates the long-lived streaming connection from API requests so that
a stalled stream client does not block API calls.

### Recording on the phone, not on the ESP
The ESP firmware has no video recording responsibility. The iPhone application
uses AVFoundation to encode frames received from the MJPEG stream into MP4/MOV.
This avoids SD card wear, eliminates on-device storage limits, and keeps the
recorded video in the user's native Photos library.

### Doorbell → notification abstraction
A physical button press produces a `DoorbellEvent` struct. The event is
dispatched through `INotificationTransport`, which defaults to a no-op
`DisabledNotificationTransport`. A webhook transport can be wired in once
`notifications.kjdev.uk` is live, without touching the doorbell or camera code.

### NVS config persistence
Runtime-configurable parameters (hostname, camera settings, doorbell GPIO,
notification endpoint) are persisted to ESP-IDF NVS via the Arduino
`Preferences` library. Sensible defaults are compiled in and used if no NVS
key is present.

### Credential management
Wi-Fi credentials are never compiled into the firmware binary that is committed
to source control. They are placed in `firmware/include/secrets.h`, which is
`.gitignored`. See `docs/SECURITY.md` for the credentials-exposure notice.

## Data Flow: Screenshot

```
User taps Screenshot on iPhone
  → ScreenshotService captures the current frame displayed in the UI
       (or fetches /api/v1/camera/snapshot for higher quality)
  → Saves JPEG to iOS Photos library (or share sheet if permission denied)
  → Filename: ESPWebCam_YYYY-MM-DD_HH-MM-SS.jpg
  → ESP does NOT store anything
```

## Data Flow: Recording

```
User taps Record
  → RecordingService opens AVAssetWriter session
  → Each MJPEG frame received → pixel buffer → AVAssetWriterInput
  → User taps Stop
  → AVAssetWriter finalises MP4
  → Video saved to iPhone Photos library
  → ESP does NOT store anything
```

## Data Flow: Doorbell

```
Physical button press on GPIO14
  → DoorbellButton ISR sets rawPressed flag
  → DoorbellButton.loop() debounces and fires callback
  → DoorbellEventService.onButtonPress() builds DoorbellEvent
  → INotificationTransport.sendDoorbellEvent() (currently disabled/no-op)
  → Event available at GET /api/v1/doorbell/status
  → iPhone app polls endpoint and shows alert
```
