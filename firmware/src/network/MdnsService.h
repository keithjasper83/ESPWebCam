/**
 * MdnsService.h
 *
 * mDNS/Bonjour service advertisement so iOS clients can discover the camera
 * via espwebcam.local instead of relying on a fixed IP address.
 */
#pragma once
#include <stdint.h>

class MdnsService {
public:
    // Call after Wi-Fi is connected.
    // hostname – used as <hostname>.local
    // deviceId – advertised as instance name
    bool begin(const char *hostname, const char *deviceId,
               uint16_t httpPort, uint16_t streamPort);

    void end();
};
