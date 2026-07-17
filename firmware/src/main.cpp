/**
 * main.cpp
 *
 * ESPWebCam firmware – entry point.
 *
 * This file is the composition root only.  All business logic lives in
 * Application and the service modules under src/app, src/camera, src/network,
 * src/http, src/doorbell, src/notification, src/config, and src/diagnostics.
 *
 * SECURITY: Wi-Fi credentials are read from include/secrets.h which is
 * .gitignored and must never be committed.  Copy include/secrets.h.example
 * to include/secrets.h and fill in your credentials before building.
 *
 * The credentials previously committed to this repository (SSID "KJ iPhone",
 * password "hello123") must be treated as COMPROMISED and changed immediately.
 */
#include "app/Application.h"

// Camera model selection – must match the physical board.
// Freenove ESP32-S3 WROOM uses the ESP32S3_EYE pin profile.
#define CAMERA_MODEL_ESP32S3_EYE
#include "camera_pins.h"

// Pull Wi-Fi credentials from the uncommitted secrets file.
// See firmware/include/secrets.h.example for the template.
#include "secrets.h"

static Application app;

void setup() {
    app.setup(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
    app.loop();
    // Small yield to allow background tasks (Wi-Fi, TCP/IP stack) to run.
    delay(10);
}
