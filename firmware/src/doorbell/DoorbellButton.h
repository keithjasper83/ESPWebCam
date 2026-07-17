/**
 * DoorbellButton.h
 *
 * Handles a physical momentary push button with software debounce and
 * edge detection. Never blocks for more than a microsecond in its ISR.
 */
#pragma once
#include <stdint.h>
#include <functional>

struct DoorbellPressEvent {
    uint32_t sequenceNumber;    // monotonically increasing per power cycle
    uint32_t timestamp;         // millis() at time of confirmed press
};

class DoorbellButton {
public:
    using PressCallback = std::function<void(const DoorbellPressEvent &)>;

    /**
     * Initialise the button on the given GPIO.
     * @param gpio          GPIO number
     * @param activeLow     true if pressing pulls the pin LOW (typical when
     *                      button connects GPIO to GND and pull-up is enabled)
     * @param debounceMs    minimum stable time in ms before a press is accepted
     * @param minIntervalMs minimum ms between consecutive registered presses
     * @param cb            called (from loop(), not ISR) on each confirmed press
     */
    bool begin(int gpio, bool activeLow, uint32_t debounceMs,
               uint32_t minIntervalMs, PressCallback cb);

    // Call frequently from the main application loop – NOT from an ISR.
    void loop();

    uint32_t sequenceNumber() const { return _sequence; }
    bool isPressed() const;

private:
    int      _gpio          = -1;
    bool     _activeLow     = true;
    uint32_t _debounceMs    = 50;
    uint32_t _minIntervalMs = 3000;
    PressCallback _callback;

    volatile bool _rawPressed  = false;     // updated by ISR
    bool          _lastRaw     = false;
    uint32_t      _rawChangeMs = 0;
    bool          _confirmed   = false;
    uint32_t      _lastFireMs  = 0;
    uint32_t      _sequence    = 0;

    static void IRAM_ATTR isrHandler(void *arg);
};
