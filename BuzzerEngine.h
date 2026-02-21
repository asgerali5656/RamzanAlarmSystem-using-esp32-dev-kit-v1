#ifndef BUZZER_ENGINE_H
#define BUZZER_ENGINE_H

#include <Arduino.h>

enum PatternType {
    PATTERN_NONE,
    PATTERN_PRE_SEHRI,   // Short Short Long Short Short Long...
    PATTERN_SEHRI_IFTAR, // Continuous 5s... wait 10s... repeat
    PATTERN_PRAYER       // 2 Short beeps... stop
};

class BuzzerEngine {
public:
    BuzzerEngine(uint8_t pin);
    void init();
    void update();
    void startPattern(PatternType pattern);
    void stop();
    bool isRinging() const;
    void setBuzzer(bool state);
    void configurePrayerPattern(int count, int duration, int gap);
    void configureSehriPattern(int duration, int interval);
    bool getBuzzerState() const { return _buzzerState; }

private:
    uint8_t _pin;
    PatternType _currentPattern;
    unsigned long _lastStateChangeTime;
    int _stepIndex;
    bool _buzzerState; // true = ON (HIGH), false = OFF (LOW)

    // Configurable Prayer Pattern
    int _prayerBeepCount = 2;
    int _prayerBeepDuration = 300;
    int _prayerBeepGap = 300;
    
    // Configurable Sehri Pattern
    int _sehriDuration = 5000;
    int _sehriRepeatInterval = 10000;

    void handlePreSehri();
    void handleSehriIftar();
    void handlePrayer();
    void handleAutoOffPattern();
};

#endif
