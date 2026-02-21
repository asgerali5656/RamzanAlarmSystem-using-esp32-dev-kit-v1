#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

class InputManager {
public:
    void init();
    void update(); // Call frequently for debouncing if needed

    // Direct reads (debounced optional, for now raw is okay given "edge detection" logic but debounce is safer)
    bool readSwitchPreSehri();
    bool readButtonA();
    bool readButtonB();
    bool readButtonNav();

    // Helper to check if state changed from a reference
    bool hasStateChanged(int pin, bool referenceState);

private:
    // Simple debounce could happen here if needed
};

#endif
