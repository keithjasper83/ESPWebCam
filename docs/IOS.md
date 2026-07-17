# ESPWebCam – iOS Application

## Overview

The iOS application is a native Swift/SwiftUI iPhone app located in `ios/`.

## Deployment Target

- **Minimum iOS version**: iOS 17.0
- **Language**: Swift 5.10+
- **UI framework**: SwiftUI
- **Xcode version**: 15.0+

iOS 17 is chosen because it provides a clean `Observable` macro (`@Observable`)
for state management, removes boilerplate compared to `ObservableObject`, and
all currently supported iPhone models (iPhone 12 and later) run iOS 17.

## Architecture

```
ios/ESPWebCam/
├── App/
│   └── ESPWebCamApp.swift          @main entry point
├── Models/
│   ├── CameraDevice.swift          Stored camera identity
│   ├── CameraSettings.swift        API settings model
│   └── DoorbellEvent.swift         Doorbell event model
├── Services/
│   ├── CameraAPIClient.swift       URLSession-based REST client
│   ├── MJPEGStreamClient.swift     Multipart stream parser
│   ├── MJPEGFrameParser.swift      Boundary/header/frame parser
│   ├── RecordingService.swift      AVAssetWriter video recording
│   ├── ScreenshotService.swift     Photos library / share sheet
│   ├── DoorbellNotificationService.swift  Doorbell polling
│   └── CameraDiscoveryService.swift       mDNS/Bonjour discovery
├── ViewModels/
│   ├── CameraViewModel.swift       Main stream + connection state
│   └── CameraSettingsViewModel.swift
├── Views/
│   ├── ContentView.swift           Root navigation
│   ├── CameraView.swift            Main live view
│   ├── CameraSettingsView.swift    Settings sheet
│   └── CameraDiscoveryView.swift   Add/select camera
└── Resources/
    └── Assets.xcassets
```

## Main Camera Screen

The main screen shows:
- Live MJPEG video stream
- Camera name and connection state
- Record / Stop button (explicit user action required)
- Screenshot button
- Doorbell event indicator
- Settings button
- Recording elapsed timer (visible only while recording)

## Recording Rules

**Recording never starts automatically.** The following events must NOT
trigger recording:

- App launch
- App foreground transition
- Network reconnect
- Doorbell press
- Timer or schedule

Recording starts only when the user explicitly taps **Record**.

When the app enters the background during an active recording:
- Recording is stopped and finalised immediately, or
- A short `beginBackgroundTask` window is used to finalise the file
- No covert recording continues in the background

## MJPEG Client

`MJPEGStreamClient` connects to `http://<host>:81/stream` and parses the
`multipart/x-mixed-replace` response stream.

`MJPEGFrameParser` handles:
- Partial network reads (data arrives in chunks)
- Boundary detection (`--123456789000000000000987654321`)
- `Content-Length` header parsing
- JPEG frame extraction
- `X-Timestamp` header extraction for PTS calculation

The parser is tested independently with byte fixture data (see
`ESPWebCamTests/MJPEGFrameParserTests.swift`).

## Video Recording

Uses `AVAssetWriter` with `AVAssetWriterInput` configured for H.264.

Incoming JPEG frames are decoded to `CVPixelBuffer` and appended to the
asset writer. Presentation timestamps are derived from `X-Timestamp` headers
or actual frame arrival time — **not** from the advisory `X-Framerate: 60`
header.

Output format: MP4, H.264 video, no audio track.

Metadata: camera device ID, recording start time.

Filename: `ESPWebCam_YYYY-MM-DD_HH-MM-SS.mp4`

## Screenshots

`ScreenshotService` captures the current displayed frame and saves it via
`PHPhotoLibrary` (or a share sheet if Photos permission is denied).

Snapshot endpoint `GET /api/v1/camera/snapshot` is preferred over extracting
a frame from the stream for higher JPEG quality.

Filename: `ESPWebCam_YYYY-MM-DD_HH-MM-SS.jpg`  
Timezone: Device local timezone.  
Metadata: Camera device ID in EXIF comment.

## Camera Discovery

`CameraDiscoveryService` uses `Network.framework` (`NWBrowser`) to discover
`_espwebcam-stream._tcp` Bonjour services on the local network.

Users can also manually enter a hostname or IP address.

Discovered and manually-entered cameras are persisted using `UserDefaults`.

## Permissions Required

| Permission | Purpose |
|------------|---------|
| Local Network | Connect to ESP camera |
| Photos (write) | Save screenshots and recordings |
| Camera | Not required (no local camera use) |

## Build Instructions

1. Open `ios/ESPWebCam.xcodeproj` in Xcode 15+
2. Select your development team in Signing & Capabilities
3. Select iPhone target device or simulator
4. Build and run (`⌘R`)

The app connects to the camera on the same Wi-Fi network. Ensure the iOS
device and the ESP32 are on the same network.
