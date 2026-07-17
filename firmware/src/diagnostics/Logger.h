/**
 * Logger.h
 *
 * Thin logging macros wrapping ESP-IDF / Arduino logging.
 * Never log Wi-Fi passwords or notification tokens.
 */
#pragma once

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_E(tag, fmt, ...) log_e("[%s] " fmt, tag, ##__VA_ARGS__)
  #define LOG_W(tag, fmt, ...) log_w("[%s] " fmt, tag, ##__VA_ARGS__)
  #define LOG_I(tag, fmt, ...) log_i("[%s] " fmt, tag, ##__VA_ARGS__)
  #define LOG_D(tag, fmt, ...) log_d("[%s] " fmt, tag, ##__VA_ARGS__)
#else
  #include "esp_log.h"
  #define LOG_E(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
  #define LOG_W(tag, fmt, ...) ESP_LOGW(tag, fmt, ##__VA_ARGS__)
  #define LOG_I(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
  #define LOG_D(tag, fmt, ...) ESP_LOGD(tag, fmt, ##__VA_ARGS__)
#endif
