#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Pin Definitions ---
// Updated based on your request

// Buzzers / Relays
#define PIN_BUZZER_HOUSE_A  25
#define PIN_BUZZER_HOUSE_B  26

// Set this to true if your relay triggers on LOW (Common for relay modules)
// Set to false if you are using a direct buzzer that triggers on HIGH
#define RELAY_ACTIVE_LOW true

// Switch for 1-hour-before-Sehri reminder
// You did not list this pin, so I assigned GPIO 13. Connect your switch here.
#define PIN_SWITCH_PRE_SEHRI 13 

// Navigation Button (For future use in Phase 3/Display)
#define PIN_BUTTON_NAV       4 

// House Buttons (Renamed for clarity if needed, ensuring match with .ino)
#define PIN_BUTTON_HOUSE_A   16
#define PIN_BUTTON_HOUSE_B   17

// --- LCD Pin Definitions (4-bit mode) ---
#define PIN_LCD_RS    21
#define PIN_LCD_EN    22
#define PIN_LCD_D4    23
#define PIN_LCD_D5    19
#define PIN_LCD_D6    18
#define PIN_LCD_D7    5

// --- Timing Constants ---
#define PRE_SEHRI_OFFSET_MINUTES 60
#define SEHRI_WAKE_OFFSET_MINUTES 45 // Wake up 45 mins before end
#define NTP_SYNC_INTERVAL_HOURS  12

// --- Buzzer Pattern Definitions (in ms) ---
#define TONE_SHORT_DURATION 300
#define TONE_LONG_DURATION  800
#define GAP_DURATION        300

#define SEHRI_CONTINUOUS_DURATION 5000
#define SEHRI_REPEAT_INTERVAL     10000

// --- WiFi Credentials ---
#define WIFI_SSID     "isko change mat karna"
#define WIFI_PASSWORD "passwordhai"

// --- Static IP Configuration ---
// Uncomment to use Static IP
// #define USE_STATIC_IP
// Replace with your desired IP settings
#define STATIC_IP_ADDR "192.168.1.200"
#define STATIC_GATEWAY "192.168.1.1"
#define STATIC_SUBNET  "255.255.255.0"
#define STATIC_DNS1    "8.8.8.8"
#define STATIC_DNS2    "8.8.4.4"

// --- Timezone Settings (NTP) ---
// Adjust for your location. Example: India is UTC +5:30 = 5.5 * 3600 = 19800
#define GMT_OFFSET_SEC      19800 
#define DAYLIGHT_OFFSET_SEC 0

#endif
