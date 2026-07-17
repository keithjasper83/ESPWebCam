# ESPWebCam – API Reference

## Versioning

All current endpoints are under `/api/v1`.

Every successful response uses:

```json
{ "ok": true, "data": { ... } }
```

Every error response uses:

```json
{
  "ok": false,
  "error": {
    "code": "ERROR_CODE",
    "message": "Human-readable explanation"
  }
}
```

---

## Endpoints

### `GET /api/v1/health`

Lightweight liveness probe.

**Response `data`**

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | `"ok"` |
| `uptime` | number | Seconds since boot |
| `freeHeap` | number | Free heap bytes |
| `wifiConnected` | boolean | |
| `streamClients` | number | Active MJPEG stream connections |
| `firmwareVersion` | string | e.g. `"2.0.0"` |
| `apiVersion` | string | `"v1"` |

---

### `GET /api/v1/status`

Detailed device status.

**Response `data`**

| Field | Type | Description |
|-------|------|-------------|
| `deviceId` | string | Configured device identifier |
| `hostname` | string | mDNS hostname (without `.local`) |
| `firmwareVersion` | string | |
| `apiVersion` | string | |
| `uptime` | number | Seconds since boot |
| `freeHeap` | number | |
| `minFreeHeap` | number | Minimum seen since boot |
| `rssi` | number | Wi-Fi signal strength (dBm) |
| `ipAddress` | string | |
| `wifiConnected` | boolean | |
| `streamClients` | number | |

---

### `GET /api/v1/camera/settings`

Returns the current camera sensor settings as actually applied.

**Response `data`**

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `frameSize` | integer | 0–13 | `framesize_t` enum value |
| `frameSizeName` | string | | Human-readable name, e.g. `"VGA"` |
| `jpegQuality` | integer | 4–63 | Lower = higher quality |
| `brightness` | integer | -2–2 | |
| `contrast` | integer | -2–2 | |
| `saturation` | integer | -2–2 | |
| `aeLevel` | integer | -5–5 | Exposure compensation |
| `hmirror` | integer | 0–1 | Horizontal mirror |
| `vflip` | integer | 0–1 | Vertical flip |
| `sensorPid` | string | | Sensor product ID, e.g. `"0x2641"` |

---

### `PUT /api/v1/camera/settings`

Update one or more camera settings. Unspecified fields are unchanged.

**Request**

`Content-Type: application/json`

```json
{
  "frameSize": 8,
  "jpegQuality": 12,
  "brightness": 1,
  "contrast": 0,
  "saturation": 0,
  "aeLevel": -3,
  "hmirror": 1,
  "vflip": 0
}
```

**Response**: Same as `GET /api/v1/camera/settings` – returns the final
applied values, not just the requested values.

**Error codes**

| Code | Meaning |
|------|---------|
| `INVALID_JSON` | Body is not valid JSON |
| `INVALID_CONTENT_TYPE` | Content-Type is not `application/json` |
| `INVALID_CAMERA_SETTING` | A value is outside its valid range |
| `REQUEST_TOO_LARGE` | Body exceeds 512 bytes |
| `CAMERA_NOT_READY` | Camera failed to initialise |

---

### `GET /api/v1/camera/snapshot`

Returns a single JPEG image from the camera sensor.

**Response**: `image/jpeg` binary body.

Prefer this endpoint over capturing a frame from the MJPEG stream when the
highest available quality is needed for a screenshot.

---

### `GET /api/v1/camera/stream`

MJPEG multipart stream. Also available at `http://<host>:81/stream`.

**Content-Type**: `multipart/x-mixed-replace;boundary=123456789000000000000987654321`

Each part:
```
--123456789000000000000987654321
Content-Type: image/jpeg
Content-Length: <bytes>
X-Timestamp: <tv_sec>.<tv_usec>

<JPEG bytes>
```

The `X-Framerate: 60` header is advisory only. Actual frame rate depends on
sensor settings, resolution, and network conditions. **Do not assume 60 fps**
when constructing video timestamps; use arrival time or `X-Timestamp` values.

The stream server runs on **port 81** to isolate it from the API server.

---

### `GET /api/v1/doorbell/status`

Returns the most recent doorbell event.

**Response `data`**

| Field | Type | Description |
|-------|------|-------------|
| `totalEvents` | number | Cumulative press count since boot |
| `lastEvent` | object\|null | Most recent event, or `null` |
| `lastEvent.eventType` | string | `"doorbell.pressed"` |
| `lastEvent.eventId` | string | `"<deviceId>-<sequence>"` |
| `lastEvent.deviceId` | string | |
| `lastEvent.occurredAtMs` | number | `millis()` at press time |
| `lastEvent.sequence` | number | Monotonically increasing |

---

### `POST /api/v1/doorbell/test`

Triggers a synthetic doorbell event without requiring physical button press.
Useful for testing notification delivery.

**Response `data`**: `{ "triggered": true }`

---

## Deprecated Legacy Routes

The following routes from the original firmware are retained for backward
compatibility with existing browser clients. They are **deprecated** and may
be removed in a future firmware version. Use the `/api/v1` equivalents.

| Legacy route | Method | V1 equivalent |
|---|---|---|
| `/` | GET | (Web UI – same) |
| `/settings` | GET | `GET /api/v1/camera/settings` |
| `/settings` | POST | `PUT /api/v1/camera/settings` |
| `/status` | GET | `GET /api/v1/status` |
| `/stream` | GET | `GET /api/v1/camera/stream` (port 81) |
| `/button` | POST | **Removed** – SD-card capture is no longer supported |

The `/button` POST endpoint has been **removed**. It previously captured a
JPEG to the SD card, which was a confusing and misleading behaviour. All
screenshot functionality is now performed client-side by the iPhone app.
