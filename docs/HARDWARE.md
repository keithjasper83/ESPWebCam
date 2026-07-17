# ESPWebCam – Hardware Reference

## Target Board

**Freenove ESP32-S3 WROOM**

| Item | Value |
|------|-------|
| MCU | ESP32-S3 |
| Camera connector | OV2640 / OV3660 / GC2145 / GC0308 (auto-detected) |
| PSRAM | 8 MB OPI PSRAM (required for frame buffers) |
| Flash | 8 MB |
| USB | USB-C (native CDC) |
| PlatformIO board ID | `freenove_esp32_s3_wroom` |

## Camera Pin Profile

The firmware selects `CAMERA_MODEL_ESP32S3_EYE` from `camera_pins.h`.

| Signal | GPIO |
|--------|------|
| XCLK | 15 |
| SIOD (SDA) | 4 |
| SIOC (SCL) | 5 |
| Y2–Y9 | 11, 9, 8, 10, 12, 18, 17, 16 |
| VSYNC | 6 |
| HREF | 7 |
| PCLK | 13 |
| PWDN | -1 (not used) |
| RESET | -1 (not used) |

## SD Card Pins (diagnostic feature, disabled by default)

The SD_MMC interface uses 1-bit mode.

| Signal | GPIO |
|--------|------|
| CMD | 38 |
| CLK | 39 |
| D0 | 40 |

SD card support is compiled out unless `-DENABLE_SD_CARD=1` is set in
`platformio.ini`. The normal camera and recording workflow never uses the
SD card.

## Doorbell Button

### GPIO Selection Rationale

**Selected: GPIO 14**

The following GPIOs are excluded:

| GPIO(s) | Reason excluded |
|---------|----------------|
| 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18 | Used by camera interface |
| 38, 39, 40 | Used by SD_MMC |
| 0 | Boot-strap / USB boot pin |
| 3 | USB D- / serial input |
| 19, 20 | USB D+/D- (native USB CDC) |
| 26–32 | Flash / PSRAM / internal use |

GPIO 14 is:
- Not assigned to camera, SD_MMC, USB, or flash
- Supports input with internal pull-up (active-low button typical)
- Not a bootstrap pin that affects boot mode
- Tested to be available on Freenove ESP32-S3 WROOM headers

### Wiring

Connect a momentary push button between **GPIO14** and **GND**.

```
GPIO14 ── [button] ── GND
            │
        (internal pull-up enabled in firmware)
```

The firmware configures the pin with `INPUT_PULLUP`. Pressing the button
pulls GPIO14 LOW; the firmware treats LOW as "pressed" (`activeLow = true`).

### Electrical Limits

- Maximum GPIO source/sink current: **40 mA** (do not exceed)
- Internal pull-up resistance: ~45 kΩ
- A 100–470 Ω series resistor is recommended to protect the pin

### Debounce

Software debounce: 50 ms stable time required before a press is accepted.  
Minimum re-trigger interval: 3 000 ms.

## Camera Settings Defaults

| Parameter | Default | Notes |
|-----------|---------|-------|
| Frame size | VGA (640×480) | `FRAMESIZE_VGA` |
| JPEG quality | 12 | 4=max quality, 63=min quality |
| XCLK | 20 MHz | |
| Brightness | +1 | |
| Saturation | 0 | |
| Exposure compensation | -3 | |
| Mirror/flip | Depends on sensor PID | See `CameraService::applyDefaultOrientation()` |

## Known Hardware Notes

1. Frame buffers are allocated in PSRAM (`CAMERA_FB_IN_PSRAM`). The board
   must have working PSRAM or camera init will fail.

2. The `X-Framerate: 60` header in the MJPEG stream is advisory. Actual
   throughput at VGA resolution over a typical Wi-Fi connection is
   approximately 10–25 fps. iOS clients must use actual frame arrival
   timestamps when constructing video files.

3. Physical camera validation has not been performed in CI. All hardware
   behaviour in this documentation is derived from the Freenove board
   documentation and Espressif ESP-IDF camera driver.
