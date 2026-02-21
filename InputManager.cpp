#include "InputManager.h"
#include "Config.h"

// Simple debounce implementation
struct Debouncer {
    uint8_t pin;
    bool stableState;
    bool lastState;
    unsigned long lastDebounceTime;
    const unsigned long debounceDelay = 50; 

    void init(uint8_t p) {
        pin = p;
        pinMode(pin, INPUT_PULLUP); // Assuming pull-up, user can change if needed
        stableState = digitalRead(pin);
        lastState = stableState;
        lastDebounceTime = 0;
    }

    void update() {
        bool reading = digitalRead(pin);
        if (reading != lastState) {
            lastDebounceTime = millis();
        }

        if ((millis() - lastDebounceTime) > debounceDelay) {
            if (reading != stableState) {
                stableState = reading;
            }
        }
        lastState = reading;
    }
};

Debouncer switchPreSehri;
Debouncer buttonA;
Debouncer buttonB;
Debouncer buttonNav;

void InputManager::init() {
    switchPreSehri.init(PIN_SWITCH_PRE_SEHRI);
    buttonA.init(PIN_BUTTON_HOUSE_A);
    buttonB.init(PIN_BUTTON_HOUSE_B);
    buttonNav.init(PIN_BUTTON_NAV);
}

void InputManager::update() {
    switchPreSehri.update();
    buttonA.update();
    buttonB.update();
    buttonNav.update();
}

bool InputManager::readSwitchPreSehri() {
    return switchPreSehri.stableState;
}

bool InputManager::readButtonA() {
    return buttonA.stableState;
}

bool InputManager::readButtonB() {
    return buttonB.stableState;
}

bool InputManager::readButtonNav() {
    return buttonNav.stableState;
}

bool InputManager::hasStateChanged(int pin, bool referenceState) {
    if (pin == PIN_SWITCH_PRE_SEHRI) return switchPreSehri.stableState != referenceState;
    if (pin == PIN_BUTTON_HOUSE_A) return buttonA.stableState != referenceState;
    if (pin == PIN_BUTTON_HOUSE_B) return buttonB.stableState != referenceState;
    if (pin == PIN_BUTTON_NAV) return buttonNav.stableState != referenceState;
    return false;
}
