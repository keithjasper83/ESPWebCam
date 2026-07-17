# ESPWebCam

A monorepo containing firmware and a native iPhone app for a door camera
built on the **Freenove ESP32-S3 WROOM**.

## ⚠️ Security Notice

Wi-Fi credentials previously committed to this repository must be treated as
**compromised**. See [docs/SECURITY.md](docs/SECURITY.md) for details and
remediation steps.

---

## Repository Structure

```
ESPWebCam/
├── firmware/          PlatformIO/Arduino firmware for ESP32-S3
├── ios/               Native Swift/SwiftUI iPhone application
├── shared/            OpenAPI spec and shared contracts
├── docs/              Architecture, API, hardware, iOS, security docs
└── .github/workflows/ CI workflows
```

## Quick Start

### Firmware

1. **Install prerequisites**: [PlatformIO](https://platformio.org/install)

2. **Configure credentials**:
   ```bash
   cp firmware/include/secrets.h.example firmware/include/secrets.h
   # Edit secrets.h with your Wi-Fi SSID and password
   ```

3. **Build and flash**:
   ```bash
   cd firmware
   pio run -e freenove_esp32_s3_wroom        # build only
   pio run -e freenove_esp32_s3_wroom -t upload  # build + flash
   pio device monitor                        # serial output
   ```

4. The camera will be discoverable at `http://espwebcam.local` on the local
   network (or by its IP address printed to the serial monitor).

### iOS App

1. Open `ios/ESPWebCam.xcodeproj` in **Xcode 15+**
2. Select your development team under Signing & Capabilities
3. Connect an iPhone or select a simulator
4. Press `⌘R` to build and run

The app discovers cameras via mDNS (`espwebcam.local`) or allows manual
entry of a hostname or IP address.

---

## API

The firmware exposes a versioned REST API at `/api/v1`.

| Endpoint | Method | Description |
|---|---|---|
| `/api/v1/health` | GET | Liveness probe |
| `/api/v1/status` | GET | Device status |
| `/api/v1/camera/settings` | GET / PUT | Read / update camera settings |
| `/api/v1/camera/snapshot` | GET | JPEG still image |
| `/api/v1/camera/stream` | GET | MJPEG stream (port 81) |
| `/api/v1/doorbell/status` | GET | Last doorbell event |
| `/api/v1/doorbell/test` | POST | Trigger test event |

Full documentation: [docs/API.md](docs/API.md)  
OpenAPI spec: [shared/openapi.yaml](shared/openapi.yaml)

---

## Recording Rules

Recording is an **explicit user action only**.

- ✅ Recording starts when the user taps **Record** in the iPhone app
- ❌ Recording does NOT start on app launch
- ❌ Recording does NOT start on reconnect
- ❌ Recording does NOT start on doorbell press
- ❌ Recording does NOT happen on the ESP device
- ❌ SD card is NOT used for recording (disabled by default)

Recorded video is saved as MP4 to the iPhone's Photos library.

---

## Documentation

| File | Description |
|---|---|
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | System design and data flows |
| [docs/API.md](docs/API.md) | HTTP API reference |
| [docs/HARDWARE.md](docs/HARDWARE.md) | Board, camera, GPIO, doorbell wiring |
| [docs/IOS.md](docs/IOS.md) | iOS app architecture |
| [docs/NOTIFICATIONS.md](docs/NOTIFICATIONS.md) | Notification service contract |
| [docs/SECURITY.md](docs/SECURITY.md) | Credentials handling and security notes |
| [docs/TESTING.md](docs/TESTING.md) | Test strategy |

---

## Hardware

**Board**: Freenove ESP32-S3 WROOM  
**Camera**: OV2640 (auto-detected)  
**Doorbell button**: GPIO 14 (see [docs/HARDWARE.md](docs/HARDWARE.md))

---

## License

See [LICENSE](LICENSE).
