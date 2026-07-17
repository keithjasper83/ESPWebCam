/**
 * MdnsService.cpp
 */
#include "MdnsService.h"
#include <ESPmDNS.h>
#include "../diagnostics/Logger.h"

static const char *TAG = "MdnsService";

bool MdnsService::begin(const char *hostname, const char *deviceId,
                        uint16_t httpPort, uint16_t streamPort) {
    if (!MDNS.begin(hostname)) {
        LOG_E(TAG, "Failed to start mDNS responder");
        return false;
    }

    // Advertise the HTTP API/web server
    MDNS.addService("http", "tcp", httpPort);
    MDNS.addServiceTxt("http", "tcp", "device", deviceId);
    MDNS.addServiceTxt("http", "tcp", "firmware", "espwebcam");
    MDNS.addServiceTxt("http", "tcp", "api",      "/api/v1");

    // Advertise the MJPEG stream on its own service type so iOS can discover it
    MDNS.addService("espwebcam-stream", "tcp", streamPort);
    MDNS.addServiceTxt("espwebcam-stream", "tcp", "device", deviceId);
    MDNS.addServiceTxt("espwebcam-stream", "tcp", "path",   "/stream");

    LOG_I(TAG, "mDNS started – %s.local (http:%d, stream:%d)",
          hostname, httpPort, streamPort);
    return true;
}

void MdnsService::end() {
    MDNS.end();
}
