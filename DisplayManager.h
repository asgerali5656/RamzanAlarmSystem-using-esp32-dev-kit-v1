#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Arduino.h>
#include <LiquidCrystal.h>

class DisplayManager {
public:
    void init();
    void update(String timeStr, String statusMsg);
    void showMessage(String line1, String line2);
    
    // New methods for message override and live preview (PUBLIC)
    void setOverrideMessage(String l1, String l2, unsigned long durationMs = 5000);
    bool isMessageActive();
    
    // Current display content (for preview)
    String getCurrentLine1() { return _currentL1; }
    String getCurrentLine2() { return _currentL2; }

private:
    unsigned long _lastUpdate;
    unsigned long _messageExpiry = 0;
    String _currentL1;
    String _currentL2;
};

#endif
