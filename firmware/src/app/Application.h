/**
 * Application.h
 *
 * Composition root. Owns and wires all services together.
 * main.cpp creates one Application instance and calls setup()/loop().
 */
#pragma once
#include "../config/DeviceConfig.h"
#include "../config/ConfigStore.h"
#include "../camera/CameraService.h"
#include "../network/WiFiService.h"
#include "../network/MdnsService.h"
#include "../http/HttpServer.h"
#include "../doorbell/DoorbellButton.h"
#include "../doorbell/DoorbellEventService.h"
#include "../notification/NotificationClient.h"

class Application {
public:
    void setup(const char *ssid, const char *password);
    void loop();

private:
    DeviceConfig          _cfg;
    CameraService         _camera;
    WiFiService           _wifi;
    MdnsService           _mdns;
    HttpServer            _httpServer;
    DoorbellButton        _doorbellButton;
    DoorbellEventService  _doorbellEvents;
    NotificationClient    _notifications;

    bool _mdnsStarted    = false;
    bool _httpStarted    = false;
};
