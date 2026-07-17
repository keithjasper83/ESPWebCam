// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "driver/ledc.h"

#include "sdkconfig.h"

#include "Arduino.h"
#include "sd_read_write.h"

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define TAG ""
#else
#include "esp_log.h"
static const char *TAG = "camera_httpd";
#endif

typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static int button_state = 1;

// Settings structure
typedef struct {
    framesize_t frame_size;
    uint8_t jpeg_quality;
    uint32_t xclk_freq_hz;
} camera_settings_t;

static camera_settings_t cam_settings = {
    .frame_size = FRAMESIZE_VGA,
    .jpeg_quality = 12,
    .xclk_freq_hz = 20000000
};

typedef struct
{
    size_t size;  // number of values used for filtering
    size_t index; // current value index
    size_t count; // value count
    int sum;
    int *values; // array to be filled with values
} ra_filter_t;

static ra_filter_t ra_filter;

static ra_filter_t *ra_filter_init(ra_filter_t *filter, size_t sample_size)
{
    memset(filter, 0, sizeof(ra_filter_t));

    filter->values = (int *)malloc(sample_size * sizeof(int));
    if (!filter->values)
    {
        return NULL;
    }
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}

static int ra_filter_run(ra_filter_t *filter, int value)
{
    if (!filter->values)
    {
        return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size)
    {
        filter->count++;
    }
    return filter->sum / filter->count;
}

static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    struct timeval _timestamp;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[128];

    static int64_t last_frame = 0;
    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");

    while (true)
    {
        fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        
        _timestamp.tv_sec = fb->timestamp.tv_sec;
        _timestamp.tv_usec = fb->timestamp.tv_usec;
        
        if (fb->format != PIXFORMAT_JPEG)
        {
            bool jpeg_converted = frame2jpg(fb, 90, &_jpg_buf, &_jpg_buf_len);  // Higher compression quality
            esp_camera_fb_return(fb);
            fb = NULL;
            if (!jpeg_converted)
            {
                ESP_LOGE(TAG, "JPEG compression failed");
                res = ESP_FAIL;
                break;
            }
        }
        else
        {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }
        
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if (res == ESP_OK)
        {
            size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        
        // Cleanup
        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        
        if (res != ESP_OK)
        {
            ESP_LOGI(TAG, "Stream error res=%d, breaking stream", res);
            break;
        }
        
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
        
        ESP_LOGI(TAG, "MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)",
                (uint32_t)(_jpg_buf_len),
                (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time,
                avg_frame_time, 1000.0 / avg_frame_time);
    }
    ESP_LOGI(TAG, "Stream exit!");
    last_frame = 0;
    return res;
}

static esp_err_t parse_get(httpd_req_t *req, char **obuf)
{
    char *buf = NULL;
    size_t buf_len = 0;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            *obuf = buf;
            return ESP_OK;
        }
        free(buf);
    }
    httpd_resp_send_404(req);
    return ESP_FAIL;
}

static esp_err_t settings_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        // Return current settings as JSON
        char json_response[512];
        const char *frame_size_str = "VGA";
        switch(cam_settings.frame_size) {
            case FRAMESIZE_QVGA: frame_size_str = "QVGA"; break;
            case FRAMESIZE_HVGA: frame_size_str = "HVGA"; break;
            case FRAMESIZE_VGA: frame_size_str = "VGA"; break;
            case FRAMESIZE_SVGA: frame_size_str = "SVGA"; break;
            case FRAMESIZE_XGA: frame_size_str = "XGA"; break;
            default: frame_size_str = "VGA"; break;
        }
        snprintf(json_response, sizeof(json_response),
            "{\"frame_size\":\"%s\",\"jpeg_quality\":%d,\"xclk_freq_hz\":%d}",
            frame_size_str, cam_settings.jpeg_quality, cam_settings.xclk_freq_hz);
        
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, json_response, strlen(json_response));
    } 
    else if (req->method == HTTP_POST) {
        // Update settings from JSON
        char buf[256] = {0};
        if (httpd_req_recv(req, buf, sizeof(buf)) <= 0) {
            return ESP_FAIL;
        }
        
        sensor_t * s = esp_camera_sensor_get();
        if (!s) return ESP_FAIL;
        
        // Parse frame_size
        if (strstr(buf, "QVGA")) cam_settings.frame_size = FRAMESIZE_QVGA;
        else if (strstr(buf, "HVGA")) cam_settings.frame_size = FRAMESIZE_HVGA;
        else if (strstr(buf, "SVGA")) cam_settings.frame_size = FRAMESIZE_SVGA;
        else if (strstr(buf, "XGA")) cam_settings.frame_size = FRAMESIZE_XGA;
        else cam_settings.frame_size = FRAMESIZE_VGA;
        
        s->set_framesize(s, cam_settings.frame_size);
        
        // Parse jpeg_quality
        char *quality_ptr = strstr(buf, "\"jpeg_quality\":");
        if (quality_ptr) {
            cam_settings.jpeg_quality = atoi(quality_ptr + 15);
        }
        
        char json_response[] = "{\"status\":\"ok\"}";
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, json_response, strlen(json_response));
    }
    return ESP_FAIL;
}

static esp_err_t status_handler(httpd_req_t *req)
{
    sensor_t * s = esp_camera_sensor_get();
    if (!s) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    char json_response[512];
    snprintf(json_response, sizeof(json_response),
        "{\"status\":\"ok\",\"model\":\"ESP32-S3\",\"firmware_version\":\"1.0\"}");
    
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, strlen(json_response));
}
const char index_web[]=R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Camera</title>
    <style>
      * { margin: 0; padding: 0; box-sizing: border-box; }
      body { 
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
        background: #f0f0f0;
        padding: 10px;
      }
      .container {
        max-width: 100%;
        margin: 0 auto;
      }
      .header {
        text-align: center;
        padding: 15px;
        background: #333;
        color: white;
        border-radius: 8px;
        margin-bottom: 15px;
      }
      .header h1 { font-size: 24px; margin-bottom: 5px; }
      .stream-container {
        width: 100%;
        background: #000;
        border-radius: 8px;
        overflow: hidden;
        margin-bottom: 15px;
      }
      #stream {
        width: 100%;
        display: block;
      }
      .controls {
        background: white;
        padding: 15px;
        border-radius: 8px;
        margin-bottom: 15px;
        box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      }
      .control-group {
        margin-bottom: 15px;
      }
      .control-group label {
        display: block;
        font-weight: 600;
        margin-bottom: 8px;
        color: #333;
        font-size: 14px;
      }
      .control-group select,
      .control-group input {
        width: 100%;
        padding: 10px;
        border: 1px solid #ddd;
        border-radius: 6px;
        font-size: 16px;
      }
      .control-group input[type="range"] {
        padding: 0;
        height: 6px;
      }
      .button-group {
        display: flex;
        gap: 10px;
        flex-wrap: wrap;
      }
      button {
        flex: 1;
        min-width: 100px;
        padding: 12px;
        border: none;
        border-radius: 6px;
        font-size: 16px;
        font-weight: 600;
        cursor: pointer;
        transition: background 0.2s;
      }
      .btn-primary {
        background: #007AFF;
        color: white;
      }
      .btn-primary:active {
        background: #0051D5;
      }
      .btn-secondary {
        background: #f0f0f0;
        color: #333;
      }
      .btn-secondary:active {
        background: #e0e0e0;
      }
      .status {
        font-size: 12px;
        color: #666;
        margin-top: 10px;
        padding: 10px;
        background: #f9f9f9;
        border-radius: 4px;
      }
      .quality-display {
        font-size: 12px;
        color: #666;
        margin-top: 5px;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <div class="header">
        <h1>ESP32 Camera</h1>
        <p id="ip-addr">Loading...</p>
      </div>

      <div class="stream-container">
        <img id="stream" src="" alt="Stream">
      </div>

      <div class="controls">
        <div class="control-group">
          <label for="resolution">Resolution:</label>
          <select id="resolution">
            <option value="QVGA">QVGA (320×240)</option>
            <option value="HVGA">HVGA (480×320)</option>
            <option value="VGA" selected>VGA (640×480)</option>
            <option value="SVGA">SVGA (800×600)</option>
            <option value="XGA">XGA (1024×768)</option>
          </select>
        </div>

        <div class="control-group">
          <label for="quality">JPEG Quality: <span id="quality-value">12</span></label>
          <input type="range" id="quality" min="1" max="63" value="12">
          <div class="quality-display">1 (lowest) to 63 (highest)</div>
        </div>

        <div class="button-group">
          <button class="btn-primary" onclick="updateSettings()">Apply Settings</button>
          <button class="btn-secondary" onclick="captureFrame()">Capture</button>
        </div>

        <div class="status">
          <div id="status-text">Ready</div>
        </div>
      </div>
    </div>

    <script>
      // Load current settings on page load
      async function loadSettings() {
        try {
          const response = await fetch('/settings');
          const data = await response.json();
          document.getElementById('resolution').value = data.frame_size;
          document.getElementById('quality').value = data.jpeg_quality;
          document.getElementById('quality-value').textContent = data.jpeg_quality;
        } catch (e) {
          console.error('Error loading settings:', e);
        }
      }

      // Update settings
      async function updateSettings() {
        const resolution = document.getElementById('resolution').value;
        const quality = parseInt(document.getElementById('quality').value);
        
        try {
          const response = await fetch('/settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              frame_size: resolution,
              jpeg_quality: quality
            })
          });
          
          if (response.ok) {
            document.getElementById('status-text').textContent = 'Settings applied! Stream may briefly disconnect.';
            setTimeout(() => {
              startStream();
            }, 1000);
          }
        } catch (e) {
          document.getElementById('status-text').textContent = 'Error updating settings';
          console.error(e);
        }
      }

      // Capture frame to SD
      async function captureFrame() {
        try {
          await fetch('/button', {method: 'POST'});
          document.getElementById('status-text').textContent = 'Frame captured to SD card!';
          setTimeout(() => {
            document.getElementById('status-text').textContent = 'Ready';
          }, 2000);
        } catch (e) {
          document.getElementById('status-text').textContent = 'Error capturing frame';
        }
      }

      // Start video stream
      function startStream() {
        var port = window.location.port === '' ? (window.location.protocol === 'https:' ? 443 : 80) : window.location.port;
        var streamPort = parseInt(port) + 1;
        var streamUrl = window.location.protocol + '//' + window.location.hostname + ':' + streamPort + '/stream';
        document.getElementById('stream').src = streamUrl;
      }

      // Update quality display in real-time
      document.getElementById('quality').addEventListener('input', function() {
        document.getElementById('quality-value').textContent = this.value;
      });

      // Initialize on load
      window.addEventListener('DOMContentLoaded', function () {
        loadSettings();
        startStream();
      });
    </script>
  </body>
</html>)rawliteral";



static esp_err_t index_handler(httpd_req_t *req)
{
  esp_err_t err;
  err = httpd_resp_set_type(req, "text/html");
  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL)
  {
      err = httpd_resp_send(req, (const char *)index_web, sizeof(index_web));
  }
  else
  {
      ESP_LOGE(TAG, "Camera sensor not found");
      err = httpd_resp_send_500(req);
  }
  return err;
}

static esp_err_t button_handler(httpd_req_t *req)
{
  esp_err_t err;
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb)
  {
      ESP_LOGE(TAG, "Camera capture failed");
      err = ESP_FAIL;
  }
  else
  {
    String video = "/video";
    int jpgCount=readFileNum(SD_MMC, video.c_str());
    String path = video + "/" + String(jpgCount) +".jpg";
    writejpg(SD_MMC, path.c_str(), fb->buf, fb->len);
    esp_camera_fb_return(fb);
    fb = NULL;
    err=ESP_OK;
  }
  return err;
}

void startCameraServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL};

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL}; 
    
    httpd_uri_t button_uri = {
        .uri = "/button",
        .method = HTTP_POST,
        .handler = button_handler,
        .user_ctx = NULL};
    
    httpd_uri_t settings_get_uri = {
        .uri = "/settings",
        .method = HTTP_GET,
        .handler = settings_handler,
        .user_ctx = NULL};
    
    httpd_uri_t settings_post_uri = {
        .uri = "/settings",
        .method = HTTP_POST,
        .handler = settings_handler,
        .user_ctx = NULL};
    
    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL};

    ra_filter_init(&ra_filter, 20);

    ESP_LOGI(TAG, "Starting web server on port: '%d'", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(camera_httpd, &index_uri);        
        httpd_register_uri_handler(camera_httpd, &button_uri);
        httpd_register_uri_handler(camera_httpd, &settings_get_uri);
        httpd_register_uri_handler(camera_httpd, &settings_post_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        // httpd_register_uri_handler(camera_httpd, &stream_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    ESP_LOGI(TAG, "Starting stream server on port: '%d'", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}















