// ButtonEngine.cpp
#include "ButtonEngine.h"

ButtonEngine::ButtonEngine(uint8_t pin) {
    _pin = pin;
    _currentState = HIGH; 
    _confidence = 0; 
    _lastSampleTime = 0;
}

void ButtonEngine::init() {
    pinMode(_pin, INPUT_PULLUP);
    _currentState = digitalRead(_pin);
    
    // Initialize confidence based on initial state
    if (_currentState == HIGH) {
        _confidence = CONFIDENCE_THRESHOLD; // Assume stable HIGH
    } else {
        _confidence = 0; // Assume stable LOW
    }
    
    _lastSampleTime = millis();
    _pressStartTime = 0;
    _longPressHandled = false;
    _stateChanged = false;
    _currentEvent = BUTTON_NO_EVENT;
}

void ButtonEngine::update() {
    // 1. Throttle Sampling (Run every 2ms)
    // This creates a consistent integration window regardless of loop speed
    if (millis() - _lastSampleTime < 2) return;
    _lastSampleTime = millis();

    _currentEvent = BUTTON_NO_EVENT; // Reset per sample? No, keep steady until consumed or next update?
    // Actually per-loop reset is tricky with throttled update. 
    // Ideally update() should be called every loop, and we set event only when state changes.
    // Let's rely on _stateChanged flag.
    
    if (_stateChanged) _stateChanged = false; // Clear previous change flag only on next process step
    // Wait, if we return early above, we don't clear flags.
    // So calling code must check flags. Ideally events are one-shot.
    // If we return, no new event.
    
    int reading = digitalRead(_pin);

    // 2. Integrate Reading (Counter Method)
    if (reading == HIGH) {
        if (_confidence < CONFIDENCE_THRESHOLD) _confidence++;
    } else {
        if (_confidence > 0) _confidence--;
    }

    // 3. Hysteresis / Threshold Logic
    int previousState = _currentState;
    
    // If confidence hits 0, it's definitely LOW
    if (_confidence == 0 && _currentState == HIGH) {
        _currentState = LOW;
        _stateChanged = true;
        
        // EVENT: PRESSED
        _pressStartTime = millis();
        _longPressHandled = false;
        _currentEvent = BUTTON_PRESSED; 
    }
    // If confidence hits MAX, it's definitely HIGH
    else if (_confidence == CONFIDENCE_THRESHOLD && _currentState == LOW) {
        _currentState = HIGH;
        _stateChanged = true;
        
        // EVENT: RELEASED
        _currentEvent = BUTTON_RELEASED;
    }
    else {
        // No State Change this cycle (or still integrating)
        // Reset event if it was set in a previous call that didn't get consumed? 
        // No, typically update() is called once per loop.
        // If we sample every 2ms, we might set event multiple times? No, state changes once.
    }
    
    // 4. Check for Long Press (only while held down)
    if (_currentState == LOW && !_longPressHandled) {
        if ((millis() - _pressStartTime) > LONG_PRESS_DELAY) { 
            _currentEvent = BUTTON_HELD;
            _longPressHandled = true;
            _stateChanged = true; // Signal change for handling
        }
    }
}

ButtonEvent ButtonEngine::getEvent() {
    ButtonEvent e = _currentEvent;
    _currentEvent = BUTTON_NO_EVENT; // Clear after reading
    return e;
}

bool ButtonEngine::isPressed() {
    return _currentState == LOW;
}

bool ButtonEngine::hasChanged() {
    return _stateChanged;
}

int ButtonEngine::getState() {
    return _currentState;
}
