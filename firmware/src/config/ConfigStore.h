/**
 * ConfigStore.h
 *
 * Persists and loads DeviceConfig via NVS (Non-Volatile Storage).
 * Falls back to defaults if no config has been written yet.
 */
#pragma once
#include "DeviceConfig.h"

class ConfigStore {
public:
    // Load config from NVS; returns defaults for any missing key
    static DeviceConfig load();

    // Persist the current config to NVS
    static bool save(const DeviceConfig &cfg);

    // Reset all stored keys to factory defaults
    static bool resetToDefaults();
};
