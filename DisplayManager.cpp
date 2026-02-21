#include "DisplayManager.h"
#include "Config.h"

// Initialize the library with the numbers of the interface pins
// LCD Pinout: RS, EN, D4, D5, D6, D7
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

// LCD Pinout: RS, EN, D4, D5, D6, D7
// Ensure Config.h is included if needed for pins, but usually pins are passed to constructor or defined globally.
// Assuming global pins or fixed pins. The original code has hardcoded pins in constructor call.

void DisplayManager::init() {
    // Initialize LCD (16 columns, 2 rows)
    lcd.begin(16, 2);
    
    lcd.setCursor(0, 0);
    lcd.print("Ramzan Alarm    ");
    lcd.setCursor(0, 1);
    lcd.print("System Booting  ");
    
    _currentL1 = "Ramzan Alarm";
    _currentL2 = "System Booting";
    
    delay(2000);
}

void DisplayManager::update(String timeStr, String statusMsg) {
    if (isMessageActive()) return;
    showMessage(timeStr, statusMsg);
}

void DisplayManager::showMessage(String line1, String line2) {
    if (isMessageActive() && !line1.startsWith("WiFi")) return;

    // Optimization: Don't redraw if content is exactly the same
    if (line1 == _currentL1 && line2 == _currentL2) return;

    // Pad Line 1
    String p1 = line1;
    while(p1.length() < 16) p1 += " ";
    if (p1.length() > 16) p1 = p1.substring(0, 16);

    // Pad Line 2
    String p2 = line2;
    while(p2.length() < 16) p2 += " ";
    if (p2.length() > 16) p2 = p2.substring(0, 16);

    // Write if changed
    if (p1 != _currentL1) {
        lcd.setCursor(0, 0);
        lcd.print(p1);
        _currentL1 = p1;
    }
    
    if (p2 != _currentL2) {
        lcd.setCursor(0, 1);
        lcd.print(p2);
        _currentL2 = p2;
    }
}

void DisplayManager::setOverrideMessage(String l1, String l2, unsigned long durationMs) {
    _messageExpiry = millis() + durationMs;
    // Clear and Write immediately
    showMessage(l1, l2);
}

bool DisplayManager::isMessageActive() {
    return (millis() < _messageExpiry);
}
