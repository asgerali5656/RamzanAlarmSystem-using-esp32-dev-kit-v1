#include "BuzzerEngine.h"
#include "Config.h"

BuzzerEngine::BuzzerEngine(uint8_t pin) {
    _pin = pin;
    _currentPattern = PATTERN_NONE;
    _stepIndex = 0;
    _buzzerState = false;
    _lastStateChangeTime = 0;
    _lastStateChangeTime = 0;
    
    // Defaults matching original pattern
    _prayerBeepCount = 2;
    _prayerBeepDuration = 300;
    _prayerBeepGap = 300;
    
    _sehriDuration = SEHRI_CONTINUOUS_DURATION;
    _sehriRepeatInterval = SEHRI_REPEAT_INTERVAL;
}

void BuzzerEngine::configurePrayerPattern(int count, int duration, int gap) {
    if (count < 1) count = 1;
    _prayerBeepCount = count;
    _prayerBeepDuration = duration;
    _prayerBeepGap = gap;
}

void BuzzerEngine::configureSehriPattern(int duration, int interval) {
    if (duration < 100) duration = 100;
    if (interval < 100) interval = 100;
    _sehriDuration = duration;
    _sehriRepeatInterval = interval;
}

void BuzzerEngine::init() {
    pinMode(_pin, OUTPUT);
    // Initialize to OFF state immediately
    // If Active LOW, OFF is HIGH. If Active HIGH, OFF is LOW.
    digitalWrite(_pin, RELAY_ACTIVE_LOW ? HIGH : LOW);
}

void BuzzerEngine::setBuzzer(bool state) {
    // state == true means Turn ON (Make Sound)
    // state == false means Turn OFF (Silence)
    
    int pinLevel;
    if (RELAY_ACTIVE_LOW) {
        // Active Low: LOW is ON, HIGH is OFF
        pinLevel = state ? LOW : HIGH;
    } else {
        // Active High: HIGH is ON, LOW is OFF
        pinLevel = state ? HIGH : LOW;
    }
    
    digitalWrite(_pin, pinLevel);
    _buzzerState = state;
}

void BuzzerEngine::stop() {
    _currentPattern = PATTERN_NONE;
    setBuzzer(false);
    _stepIndex = 0;
}

void BuzzerEngine::startPattern(PatternType pattern) {
    if (_currentPattern == pattern) return; // Already running logic may reset if not careful
    _currentPattern = pattern;
    _stepIndex = 0;
    _lastStateChangeTime = millis();
    
    // Initial state setup based on pattern
    // For all patterns, we start immediately with ON usually
    // But update() handles transitions. Let's set initial state here for clarity.
    if (pattern != PATTERN_NONE) {
        setBuzzer(true); 
    } else {
        setBuzzer(false);
    }
}

bool BuzzerEngine::isRinging() const {
    return _currentPattern != PATTERN_NONE;
}

void BuzzerEngine::update() {
    if (_currentPattern == PATTERN_NONE) {
        return;
    }

    unsigned long now = millis();
    unsigned long elapsed = now - _lastStateChangeTime;

    switch (_currentPattern) {
        case PATTERN_PRE_SEHRI:
            handlePreSehri();
            break;
        case PATTERN_SEHRI_IFTAR:
            handleSehriIftar();
            break;
        case PATTERN_PRAYER:
            handlePrayer();
            break;
        default:
            stop();
            break;
    }
}

// Auto-off Pattern: 5s ON, 5x Short Rings (200ms ON, 200ms OFF), then off.
void BuzzerEngine::handlePreSehri() {
    handleAutoOffPattern();
}

void BuzzerEngine::handleSehriIftar() {
    handleAutoOffPattern();
}

void BuzzerEngine::handleAutoOffPattern() {
    unsigned long now = millis();
    unsigned long elapsed = now - _lastStateChangeTime;
    
    // Steps:
    // 0: ON 5000ms
    // 1: OFF 500ms
    // 2-11: 5 short rings (ON 200ms, OFF 200ms)
    
    unsigned long currentDuration = 0;
    bool nextState = false;

    switch (_stepIndex) {
        case 0: currentDuration = 5000; nextState = false; break;
        case 1: currentDuration = 500;  nextState = true;  break;
        case 2: currentDuration = 200;  nextState = false; break;
        case 3: currentDuration = 200;  nextState = true;  break;
        case 4: currentDuration = 200;  nextState = false; break;
        case 5: currentDuration = 200;  nextState = true;  break;
        case 6: currentDuration = 200;  nextState = false; break;
        case 7: currentDuration = 200;  nextState = true;  break;
        case 8: currentDuration = 200;  nextState = false; break;
        case 9: currentDuration = 200;  nextState = true;  break;
        case 10: currentDuration = 200; nextState = false; break;
    }

    if (elapsed >= currentDuration) {
        _stepIndex++;
        if (_stepIndex > 10) {
            stop();
        } else {
            setBuzzer(nextState);
            _lastStateChangeTime = now;
        }
    }
}

// Configurable Prayer Pattern
void BuzzerEngine::handlePrayer() {
    unsigned long now = millis();
    unsigned long elapsed = now - _lastStateChangeTime;

    // Determine current step duration
    // Even steps (0, 2, 4...) are ON -> Duration
    // Odd steps (1, 3, 5...) are OFF -> Gap
    
    unsigned long duration = ((_stepIndex % 2) == 0) ? _prayerBeepDuration : _prayerBeepGap;

    if (elapsed >= duration) {
        _stepIndex++;
        
        // Total steps = (2 * Count) - 1
        // e.g. Count=2 -> Steps 0(ON), 1(OFF), 2(ON). Done after 2 finishes.
        int totalSteps = (_prayerBeepCount * 2) - 1;
        
        if (_stepIndex >= totalSteps) {
            stop();
        } else {
            // If next step is even -> ON, odd -> OFF
            bool nextState = ((_stepIndex % 2) == 0);
            setBuzzer(nextState);
            _lastStateChangeTime = now;
        }
    }
}
