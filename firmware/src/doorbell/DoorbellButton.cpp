/**
 * DoorbellButton.cpp
 */
#include "DoorbellButton.h"
#include <Arduino.h>
#include "../diagnostics/Logger.h"

static const char *TAG = "DoorbellButton";

void IRAM_ATTR DoorbellButton::isrHandler(void *arg) {
    DoorbellButton *self = static_cast<DoorbellButton *>(arg);
    // Read the raw GPIO level and map to logical "pressed" state
    int level = digitalRead(self->_gpio);
    self->_rawPressed = self->_activeLow ? (level == LOW) : (level == HIGH);
}

bool DoorbellButton::begin(int gpio, bool activeLow, uint32_t debounceMs,
                           uint32_t minIntervalMs, PressCallback cb) {
    _gpio          = gpio;
    _activeLow     = activeLow;
    _debounceMs    = debounceMs;
    _minIntervalMs = minIntervalMs;
    _callback      = cb;

    int pullMode = activeLow ? INPUT_PULLUP : INPUT_PULLDOWN;
    pinMode(_gpio, pullMode);

    // Read initial state before attaching ISR to avoid spurious event
    int level = digitalRead(_gpio);
    _rawPressed = activeLow ? (level == LOW) : (level == HIGH);
    _lastRaw    = _rawPressed;

    attachInterruptArg(digitalPinToInterrupt(_gpio), isrHandler,
                       this, CHANGE);

    LOG_I(TAG, "Doorbell button on GPIO %d (active-%s, debounce %u ms, "
               "min-interval %u ms)",
          _gpio, activeLow ? "low" : "high", _debounceMs, _minIntervalMs);
    return true;
}

void DoorbellButton::loop() {
    bool raw = _rawPressed;  // single atomic read
    uint32_t now = millis();

    if (raw != _lastRaw) {
        // State changed – start debounce timer
        _lastRaw    = raw;
        _rawChangeMs = now;
    }

    // Debounce: state must be stable for _debounceMs
    if ((now - _rawChangeMs) >= _debounceMs) {
        bool stable = raw;
        if (stable && !_confirmed) {
            // Confirmed press – check minimum interval
            _confirmed = true;
            if ((now - _lastFireMs) >= _minIntervalMs) {
                _lastFireMs = now;
                _sequence++;
                DoorbellPressEvent evt;
                evt.sequenceNumber = _sequence;
                evt.timestamp      = now;
                LOG_I(TAG, "Doorbell pressed – seq=%u", _sequence);
                if (_callback) _callback(evt);
            } else {
                LOG_D(TAG, "Doorbell press ignored – min interval not elapsed");
            }
        } else if (!stable) {
            _confirmed = false;
        }
    }
}

bool DoorbellButton::isPressed() const {
    return _rawPressed;
}
