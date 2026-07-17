# ESPWebCam – Testing Strategy

## Overview

| Component | Test type | Location |
|-----------|-----------|----------|
| Firmware | PlatformIO native unit tests | `firmware/test/` |
| iOS | XCTest unit tests | `ios/ESPWebCamTests/` |
| iOS | XCTest UI tests | `ios/ESPWebCamUITests/` |
| API contract | OpenAPI lint | `shared/openapi.yaml` |
| CI | GitHub Actions | `.github/workflows/` |

---

## Firmware Tests

PlatformIO supports running tests on the native platform (no hardware required)
for pure logic, and on embedded hardware for integration tests.

### Running native tests

```bash
cd firmware
pio test -e native
```

### Current test coverage

| Module | Test file | Status |
|--------|-----------|--------|
| `DoorbellButton` debounce logic | `firmware/test/test_doorbell/` | planned |
| `CameraService` settings validation | `firmware/test/test_camera/` | planned |
| JSON response helpers | `firmware/test/test_api/` | planned |

### Hardware integration tests

Physical hardware tests require the Freenove ESP32-S3 WROOM board. They are
not run in CI. Validation checklist:

- [ ] Camera initialises without error
- [ ] MJPEG stream is reachable at `http://<ip>:81/stream`
- [ ] `GET /api/v1/health` returns 200 with `"status":"ok"`
- [ ] `PUT /api/v1/camera/settings` applies and returns correct values
- [ ] Doorbell button press appears in `GET /api/v1/doorbell/status`
- [ ] Wi-Fi reconnects after router restart (within 60 s)
- [ ] mDNS resolves at `espwebcam.local`

---

## iOS Tests

### Unit tests (`ESPWebCamTests/`)

| Test class | Tests |
|---|---|
| `MJPEGFrameParserTests` | Parses complete frame, partial reads, malformed boundary, empty input |
| `CameraAPIClientTests` | Settings GET/PUT request formation, error envelope parsing |
| `RecordingServiceTests` | State machine: idle → recording → stopped; no auto-start |
| `ScreenshotServiceTests` | Filename format, timezone, metadata |

### Running iOS tests

```
⌘U in Xcode
```

or from the command line:

```bash
xcodebuild test \
  -project ios/ESPWebCam.xcodeproj \
  -scheme ESPWebCam \
  -destination 'platform=iOS Simulator,name=iPhone 15'
```

---

## CI Workflows

### `.github/workflows/firmware-build.yml`

Runs on every push and pull request:
1. Install PlatformIO
2. Copy `secrets.h.example` → `secrets.h`
3. `pio run -e freenove_esp32_s3_wroom`
4. Upload build artefacts

### `.github/workflows/ios-build.yml`

Runs on every push and pull request:
1. `xcodebuild build -scheme ESPWebCam`
2. `xcodebuild test -scheme ESPWebCam`

### `.github/workflows/api-lint.yml`

Lints `shared/openapi.yaml` with `spectral lint`.

---

## Known Test Gaps

- No end-to-end test against real hardware in CI (hardware not available)
- Recording service tests require AVFoundation mocking (not yet implemented)
- mDNS discovery not tested in CI (requires local network)
