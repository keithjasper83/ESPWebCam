/**
 * ConfigStore.cpp
 *
 * Uses Arduino Preferences library (wraps ESP-IDF NVS) to persist settings.
 */
#include "ConfigStore.h"
#include <Preferences.h>
#include "../diagnostics/Logger.h"

static const char *NVS_NS = "espwebcam";

DeviceConfig ConfigStore::load() {
    DeviceConfig cfg = defaultConfig();
    Preferences prefs;
    if (!prefs.begin(NVS_NS, /*readOnly=*/true)) {
        LOG_W("ConfigStore", "NVS namespace not found – using defaults");
        return cfg;
    }
    if (prefs.isKey("hostname"))  prefs.getString("hostname",  cfg.hostname,  sizeof(cfg.hostname));
    if (prefs.isKey("deviceId"))  prefs.getString("deviceId",  cfg.deviceId,  sizeof(cfg.deviceId));
    if (prefs.isKey("frameSize")) cfg.frameSize    = (framesize_t)prefs.getUInt("frameSize", cfg.frameSize);
    if (prefs.isKey("jpegQual"))  cfg.jpegQuality  = (uint8_t)prefs.getUInt("jpegQual", cfg.jpegQuality);
    if (prefs.isKey("bright"))    cfg.brightness   = (int8_t)prefs.getInt("bright",   cfg.brightness);
    if (prefs.isKey("contrast"))  cfg.contrast     = (int8_t)prefs.getInt("contrast", cfg.contrast);
    if (prefs.isKey("sat"))       cfg.saturation   = (int8_t)prefs.getInt("sat",      cfg.saturation);
    if (prefs.isKey("aeLevel"))   cfg.aeLevel      = (int8_t)prefs.getInt("aeLevel",  cfg.aeLevel);
    if (prefs.isKey("xclkHz"))    cfg.xclkFreqHz   = prefs.getUInt("xclkHz",  cfg.xclkFreqHz);
    if (prefs.isKey("dbGpio"))    cfg.doorbellGpio = prefs.getInt("dbGpio",   cfg.doorbellGpio);
    if (prefs.isKey("dbActLow"))  cfg.doorbellActiveLow = prefs.getBool("dbActLow", cfg.doorbellActiveLow);
    if (prefs.isKey("notifUrl"))  prefs.getString("notifUrl", cfg.notificationEndpoint, sizeof(cfg.notificationEndpoint));
    if (prefs.isKey("notifTok"))  prefs.getString("notifTok", cfg.notificationToken,    sizeof(cfg.notificationToken));
    prefs.end();
    LOG_I("ConfigStore", "Config loaded from NVS");
    return cfg;
}

bool ConfigStore::save(const DeviceConfig &cfg) {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, /*readOnly=*/false)) {
        LOG_E("ConfigStore", "Failed to open NVS for writing");
        return false;
    }
    prefs.putString("hostname",  cfg.hostname);
    prefs.putString("deviceId",  cfg.deviceId);
    prefs.putUInt("frameSize",   (uint32_t)cfg.frameSize);
    prefs.putUInt("jpegQual",    cfg.jpegQuality);
    prefs.putInt("bright",       cfg.brightness);
    prefs.putInt("contrast",     cfg.contrast);
    prefs.putInt("sat",          cfg.saturation);
    prefs.putInt("aeLevel",      cfg.aeLevel);
    prefs.putUInt("xclkHz",      cfg.xclkFreqHz);
    prefs.putInt("dbGpio",       cfg.doorbellGpio);
    prefs.putBool("dbActLow",    cfg.doorbellActiveLow);
    prefs.putString("notifUrl",  cfg.notificationEndpoint);
    prefs.putString("notifTok",  cfg.notificationToken);
    prefs.end();
    LOG_I("ConfigStore", "Config saved to NVS");
    return true;
}

bool ConfigStore::resetToDefaults() {
    Preferences prefs;
    if (!prefs.begin(NVS_NS, /*readOnly=*/false)) return false;
    prefs.clear();
    prefs.end();
    LOG_I("ConfigStore", "Config reset to defaults");
    return true;
}
