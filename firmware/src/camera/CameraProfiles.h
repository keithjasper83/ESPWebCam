/**
 * CameraProfiles.h
 *
 * Pin profiles for supported camera boards.
 * Select exactly one CAMERA_MODEL_* before including this header.
 */
#pragma once

// Re-export the existing camera_pins.h from the legacy location.
// The camera_pins.h in firmware/src/ is the upstream Espressif file that
// covers all supported models; we simply include it here.
#include "../camera_pins.h"
