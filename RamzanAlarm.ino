#include <Arduino.h>
#include <esp_task_wdt.h> 
#include <ArduinoOTA.h> 
#include <Preferences.h> // Added for Persistent Settings
#include "Config.h"
#include "BuzzerEngine.h"
#include "InputManager.h"
#include "SystemState.h"
#include "NetworkManager.h"
#include "DisplayManager.h"
#include "AlarmScheduler.h"
#include "ButtonEngine.h"
#include "WebServerManager.h" 

// --- Global Objects ---
BuzzerEngine buzzerA(PIN_BUZZER_HOUSE_A);
BuzzerEngine buzzerB(PIN_BUZZER_HOUSE_B);

ButtonEngine btnHouseA(PIN_BUTTON_HOUSE_A);
ButtonEngine btnHouseB(PIN_BUTTON_HOUSE_B);
ButtonEngine btnNav(PIN_BUTTON_NAV);

RamzanNetworkManager networkManager;
DisplayManager displayManager;
AlarmScheduler alarmScheduler;
WebServerManager webServerManager; 
Preferences prefs; // Global Preferences for NVS

// --- System State ---
SystemState currentState = STATE_BOOT;
unsigned long bootTime = 0;
String bootTimeString = "Loading...";

// --- Switch Logic State ---
bool initialSwitchStatePreSehri = false;
bool initialSwitchStateA = false; 
bool initialSwitchStateB = false; 
bool preSehriActive = false; 

// --- Display State ---
int currentScreen = 0; 
unsigned long lastScreenAutoCycle = 0;
const int SCREEN_COUNT = 7; 

// --- Status Tracking ---
String lastActionDescription = "Boot Done";
bool wifiWasConnected = true;
unsigned long lastSleepCheck = 0;
bool sleepModeEnabled = true; // Hardcoded or from prefs? Let's check prefs below.

// --- Function Prototypes ---
void handleSerialCommands();
void checkStopConditions();
void handleDisplay();
void handleButtons();
void startPreSehriAlarm();
void startSehriAlarm();
void startIftarAlarm();
void startPrayerBeep();
void startTestMode();
void startSehriEndBeep();
void setupOTA();
void updatePrayerPattern(int count, int dur, int gap); 
void updateSehriPattern(int dur, int interval); 
void updatePreSehriOffset(int minutes); 

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n\n--- RAMZAN ALARM SYSTEM FINAL v3 (OTA + Prefs) ---");
    bootTime = millis();
    
    // Init Drivers
    buzzerA.init();
    buzzerB.init();
    
    btnHouseA.init();
    btnHouseB.init();
    btnNav.init();
    
    // Pin Modes
    pinMode(PIN_SWITCH_PRE_SEHRI, INPUT_PULLUP);
    pinMode(PIN_BUTTON_HOUSE_A, INPUT_PULLUP); 
    pinMode(PIN_BUTTON_HOUSE_B, INPUT_PULLUP);

    displayManager.init();
    displayManager.showMessage("Ramzan Alarm", "Starting...");
    
    networkManager.init();
    alarmScheduler.init();
    webServerManager.init(); 
    
    // Load Settings from NVS
    prefs.begin("ramzan", false); // Namespace "ramzan", ReadWrite
    int sOff = prefs.getInt("sOff", 0);
    int iOff = prefs.getInt("iOff", 0);
    
    // Prayer Beep Settings
    int pCount = prefs.getInt("pCount", 2);
    int pDur = prefs.getInt("pDur", 300);
    int pGap = prefs.getInt("pGap", 300);
    
    // Sehri Pattern Settings
    int sDur = prefs.getInt("sDur", 5000);
    int sInt = prefs.getInt("sInt", 10000);

    // Loading Pre-Sehri Offset
    int preOff = prefs.getInt("preOff", 60);

    Serial.print("Loaded Offsets -> Sehri: "); Serial.print(sOff);
    Serial.print(", Iftar: "); Serial.print(iOff);
    Serial.print(", Pre-Sehri: "); Serial.println(preOff);
    Serial.printf("Prayer Pattern -> Count: %d, Dur: %d, Gap: %d\n", pCount, pDur, pGap);
    Serial.printf("Sehri Pattern -> Dur: %d, Int: %d\n", sDur, sInt);

    alarmScheduler.setOffsets(sOff, iOff);
    alarmScheduler.setPreSehriOffset(preOff);
    sleepModeEnabled = prefs.getBool("sleep", false); // Default OFF for safety
    updatePrayerPattern(pCount, pDur, pGap);
    updateSehriPattern(sDur, sInt);
    
    prefs.end();
    
    // Setup OTA
    setupOTA();

    currentState = STATE_IDLE; 
    Serial.println("System Ready. Mode: IDLE");
    
    // WDT
    Serial.println("Initializing WDT (8s)...");
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 8000,
        .idle_core_mask = (1 << 0), 
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdt_config);
    esp_task_wdt_add(NULL);     

    lastActionDescription = "Boot Done";
}

void loop() {
    esp_task_wdt_reset();

    ArduinoOTA.handle(); 
    webServerManager.handleClient();
    networkManager.update(); 

    btnHouseA.update();
    btnHouseB.update();
    btnNav.update();
    buzzerA.update();
    buzzerB.update();
    alarmScheduler.update(&networkManager);
    
    static bool bootTimeCaptured = false;
    if (!bootTimeCaptured && networkManager.isTimeSynced()) {
        bootTimeString = networkManager.getFormattedTime();
        bootTimeCaptured = true;
    }

    handleButtons();
    checkStopConditions();
    
    if (currentState == STATE_IDLE && networkManager.isTimeSynced()) {
        int code = alarmScheduler.checkAlarmTriggers(&networkManager);
        if (code == 1) startSehriAlarm();
        else if (code == 2) startIftarAlarm();
        else if (code == 3) startPreSehriAlarm();
        else if (code == 4) startPrayerBeep(); 
        else if (code == 5) {
            Serial.println("AUTO-TRIGGER: Sehri Ends (3s Ring)");
            startSehriEndBeep(); 
        }

        // --- Deep Sleep Logic ---
        // Stay awake for at least 2 minutes after boot/reset for OTA/Settings
        const unsigned long GRACE_PERIOD = 120000; 
        if (sleepModeEnabled && currentState == STATE_IDLE && (millis() > GRACE_PERIOD) && (millis() - lastSleepCheck > 60000)) {
            lastSleepCheck = millis();
            long secToNext = alarmScheduler.getSecondsToNextAlarm();
            if (secToNext > (3 * 3600)) { // More than 3 hours
                long sleepSecs = secToNext - (30 * 60); // Wake 30 mins before
                if (sleepSecs > 300) { // Safety: at least 5 mins sleep
                    Serial.printf("DEEP SLEEP: Target in %ld s. Sleeping for %ld s.\n", secToNext, sleepSecs);
                    displayManager.showMessage("SLEEP MODE", "Press NAV to wake");
                    delay(3000);
                    // Add wakeup from Navigation Button (GPIO 4 / PIN_BUTTON_NAV)
                    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0); // 0 = Wake on LOW (Press)
                    esp_sleep_enable_timer_wakeup((uint64_t)sleepSecs * 1000000ULL);
                    esp_deep_sleep_start();
                }
            }
        }
    }

    // --- WiFi Warning Logic ---
    bool currentWifi = networkManager.isConnected();
    if (wifiWasConnected && !currentWifi) {
        // WiFi Just Lost
        wifiWasConnected = false;
        displayManager.setOverrideMessage("WiFi LOST", "Connect WiFi", 5000);
    } else if (!wifiWasConnected && currentWifi) {
        wifiWasConnected = true;
        displayManager.setOverrideMessage("WiFi RESTORED", networkManager.getFormattedTime(), 3000);
    }

    if (!currentWifi && millis() % 10000 < 2000) {
        displayManager.showMessage("WiFi ERROR", "Connect WiFi");
        
        // Periodic short beep if WiFi lost (Every 30 seconds)
        static unsigned long lastWifiChirp = 0;
        if (millis() - lastWifiChirp > 30000) {
            lastWifiChirp = millis();
            // buzzerA logic removed
        }
    } else {
        handleDisplay();
    }
    handleSerialCommands();
}

void setupOTA() {
    Serial.println("Configuring OTA...");
    ArduinoOTA.setHostname("RamzanAlarm-Device");
    
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("Start updating " + type);
        displayManager.showMessage("SYSTEM UPDATE", "Do Not Power Off");
        buzzerA.setBuzzer(true); delay(100); buzzerA.setBuzzer(false);
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
        displayManager.showMessage("UPDATE DONE", "Rebooting...");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        displayManager.showMessage("UPDATE FAILED", "Check Serial");
    });

    ArduinoOTA.begin();
    Serial.println("OTA Ready");
}

void handleButtons() {
    ButtonEvent navEvent = btnNav.getEvent();
    
    if (navEvent == BUTTON_PRESSED) {
        currentScreen++;
        if (currentScreen >= SCREEN_COUNT) currentScreen = 0;
        lastScreenAutoCycle = millis(); 
        Serial.print("Screen changed to: "); Serial.println(currentScreen);
    }
    else if (navEvent == BUTTON_HELD) {
        Serial.println("Long Press: Test Mode");
        startTestMode();
    }

    if (currentState == STATE_IDLE) {
        if (btnHouseA.getEvent() == BUTTON_PRESSED || btnHouseA.getEvent() == BUTTON_RELEASED) {
             bool isOn = (digitalRead(PIN_BUTTON_HOUSE_A) == LOW);
             displayManager.showMessage("House A", isOn ? "ON (GND)" : "OFF (OPEN)");
             delay(500);
             lastScreenAutoCycle = millis();
        }
        if (btnHouseB.getEvent() == BUTTON_PRESSED || btnHouseB.getEvent() == BUTTON_RELEASED) {
             bool isOn = (digitalRead(PIN_BUTTON_HOUSE_B) == LOW);
             displayManager.showMessage("House B", isOn ? "ON (GND)" : "OFF (OPEN)");
             delay(500);
             lastScreenAutoCycle = millis();
        }
    }
}

void handleDisplay() {
    static unsigned long lastUpdate = 0;
    
    if (millis() - lastScreenAutoCycle > 5000) {
        currentScreen++;
        if (currentScreen >= SCREEN_COUNT) currentScreen = 0;
        lastScreenAutoCycle = millis();
    }

    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        String line1, line2;
        
        if (currentState != STATE_IDLE && currentState != STATE_PRAYER_BEEP) {
            line1 = "ALARM ACTIVE!";
            if (currentState == STATE_PRE_SEHRI_RINGING) line2 = "PRE-SEHRI (SW)";
            else if (currentState == STATE_SEHRI_RINGING) line2 = "SEHRI TIME";
            else if (currentState == STATE_IFTAR_RINGING) line2 = "IFTAR TIME";
            else line2 = "Check Device";
        } 
        else if (currentState == STATE_PRAYER_BEEP) {
             line1 = "  PRAYER TIME   ";
             line2 = "   (2 Beeps)    ";
        }
        else {
            switch(currentScreen) {
                case 0: // Main
                    line1 = networkManager.getFormattedTime();
                    // Show Uptime on Line 2
                    {
                        unsigned long upSec = (millis() - bootTime) / 1000;
                        int upH = upSec / 3600;
                        int upM = (upSec % 3600) / 60;
                        char upBuff[16];
                        sprintf(upBuff, "UP: %02dh %02dm", upH, upM);
                        line2 = upBuff;
                    }
                    break;
                case 1: // Alarm Info
                    {
                         bool swA = (digitalRead(PIN_BUTTON_HOUSE_A) == LOW); 
                         bool swB = (digitalRead(PIN_BUTTON_HOUSE_B) == LOW); 
                         line1 = "A:" + String(swA?"ON":"OFF") + " B:" + String(swB?"ON":"OFF");
                         line2 = "Next: " + alarmScheduler.getNextAlarmTime();
                    }
                    break;
                case 2: // WiFi
                    if (currentScreen == 2 && millis() % 6000 < 3000) { 
                         line1 = "IP: " + WiFi.localIP().toString();
                         line2 = "Web Port: 80";
                    } else {
                         line1 = String("SSID:") + String(WiFi.SSID().substring(0,11)); 
                         line2 = "Sig:" + String(WiFi.RSSI()) + "dBm " + (WiFi.status() == WL_CONNECTED ? "Con" : "Dis");
                    }
                    break;
                case 3: // Status
                    {
                        // Show Next Prayer Beep Schedule
                        line1 = "Beep: " + String(prefs.getInt("pCount", 2)) + "x" + String(prefs.getInt("pDur", 300)) + "ms";
                        line2 = "Gap: " + String(prefs.getInt("pGap", 300)) + "ms"; 
                    }
                    break;
                case 4: // Family - Scrolling
                    line1 = "Arham Yusuf Anik"; 
                    if (millis() % 4000 < 2000) line2 = "Hasnain Sadiya";
                    else line2 = "Ramzan Mubarak!";
                    break;
                case 5: // Tomorrow
                    line1 = "Tom S: " + alarmScheduler.getTomorrowSehriTime();
                    line2 = "Tom I: " + alarmScheduler.getTomorrowIftarTime();
                    break;
                case 6: // Today
                    line1 = "Td S: " + alarmScheduler.getTodaySehriTime();
                    line2 = "Td I: " + alarmScheduler.getTodayIftarTime();
                    break;
            }
        }
        displayManager.showMessage(line1, line2); 
    }
}

void checkStopConditions() {
    if (currentState == STATE_PRE_SEHRI_RINGING || 
        currentState == STATE_SEHRI_RINGING || 
        currentState == STATE_IFTAR_RINGING || 
        currentState == STATE_PRAYER_BEEP) {
        
        if (!buzzerA.isRinging() && !buzzerB.isRinging()) {
            currentState = STATE_IDLE;
            alarmScheduler.stopAlarmDurationTracking();
        }
    }
}

void startPreSehriAlarm() {
    if (currentState != STATE_IDLE) return;
    lastActionDescription = "Pre-Sehri";
    initialSwitchStatePreSehri = digitalRead(PIN_SWITCH_PRE_SEHRI);
    buzzerA.startPattern(PATTERN_PRE_SEHRI);
    buzzerB.startPattern(PATTERN_PRE_SEHRI);
    currentState = STATE_PRE_SEHRI_RINGING;
}

void startSehriAlarm() { 
    lastActionDescription = "Sehri";
    // Capture Debounced States
    initialSwitchStateA = btnHouseA.getState();
    initialSwitchStateB = btnHouseB.getState();
    buzzerA.startPattern(PATTERN_SEHRI_IFTAR);
    buzzerB.startPattern(PATTERN_SEHRI_IFTAR);
    currentState = STATE_SEHRI_RINGING;
    alarmScheduler.startAlarmDurationTracking();
}

void startIftarAlarm() {
    lastActionDescription = "Iftar";
    initialSwitchStateA = btnHouseA.getState();
    initialSwitchStateB = btnHouseB.getState();
    buzzerA.startPattern(PATTERN_IFTAR);
    buzzerB.startPattern(PATTERN_IFTAR);
    currentState = STATE_IFTAR_RINGING;
    alarmScheduler.startAlarmDurationTracking();
}

void startPrayerBeep() { 
    lastActionDescription = "Prayer";
    initialSwitchStateA = btnHouseA.getState();
    initialSwitchStateB = btnHouseB.getState();
    
    // Use the user-configured prayer pattern (Beep Count, etc)
    int pCount = prefs.getInt("pCount", 2);
    int pDur = prefs.getInt("pDur", 300);
    int pGap = prefs.getInt("pGap", 300);
    buzzerA.configurePrayerPattern(pCount, pDur, pGap);
    buzzerB.configurePrayerPattern(pCount, pDur, pGap);
    
    buzzerA.startPattern(PATTERN_PRAYER);
    buzzerB.startPattern(PATTERN_PRAYER);
    currentState = STATE_PRAYER_BEEP;
}

void startSehriEndBeep() { 
    lastActionDescription = "SehriEnd";
    initialSwitchStateA = btnHouseA.getState();
    initialSwitchStateB = btnHouseB.getState();
    
    // Configure for a single 3-second beep
    buzzerA.configurePrayerPattern(1, 3000, 100);
    buzzerB.configurePrayerPattern(1, 3000, 100);
    
    buzzerA.startPattern(PATTERN_PRAYER);
    buzzerB.startPattern(PATTERN_PRAYER);
    currentState = STATE_PRAYER_BEEP;
}

void startTestMode() {
    Serial.println("TEST MODE: Simulating Sehri Alarm!");
    displayManager.showMessage("TEST MODE", "Simulating Sehri");
    delay(1000);
    startSehriAlarm(); 
}

void handleSerialCommands() {
    if (Serial.available()) {
        char c = Serial.read();
        if (c == '1') startPreSehriAlarm();
        else if (c == '2') startSehriAlarm();
        else if (c == '3') startIftarAlarm();
        else if (c == 't') startTestMode();
        else if (c == 'r') ESP.restart(); 
    }
}

void updatePrayerPattern(int count, int dur, int gap) {
    if (count < 1) count = 1;
    if (dur < 50) dur = 50;
    if (gap < 50) gap = 50;
    
    buzzerA.configurePrayerPattern(count, dur, gap);
    buzzerB.configurePrayerPattern(count, dur, gap);
}

void updateSehriPattern(int dur, int interval) {
    if (dur < 100) dur = 100;
    if (interval < 100) interval = 100;
    buzzerA.configureSehriPattern(dur, interval);
    buzzerB.configureSehriPattern(dur, interval);
}

void updatePreSehriOffset(int minutes) {
    alarmScheduler.setPreSehriOffset(minutes);
}
