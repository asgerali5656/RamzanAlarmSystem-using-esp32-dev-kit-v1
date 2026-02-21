#ifndef BUTTON_ENGINE_H
#define BUTTON_ENGINE_H

#include <Arduino.h>

enum ButtonEvent {
    BUTTON_NO_EVENT,
    BUTTON_PRESSED,   // Short press
    BUTTON_HELD,      // Long press
    BUTTON_RELEASED
};

class ButtonEngine {
public:
    ButtonEngine(uint8_t pin);
    void init();
    void update();
    ButtonEvent getEvent();
    bool isPressed();
    bool hasChanged();
    int getState(); // Returns stable, debounced HIGH/LOW
    
private:
    uint8_t _pin;
    int _currentState;      // Stable state
    
    // New Debounce Members
    int _confidence;        // Integrity counter
    unsigned long _lastSampleTime;
    
    unsigned long _pressStartTime;
    bool _longPressHandled;
    bool _stateChanged;
    
    ButtonEvent _currentEvent;
    
    const int CONFIDENCE_THRESHOLD = 50; // 50 * 2ms = 100ms settling time
    const unsigned long LONG_PRESS_DELAY = 2000;
};

#endif
