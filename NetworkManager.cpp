#include "NetworkManager.h"
#include "Config.h"

const char* ntpServer = "pool.ntp.org";

void RamzanNetworkManager::init() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);
    
    #ifdef USE_STATIC_IP
    IPAddress local_IP, gateway, subnet, primaryDNS, secondaryDNS;
    // Just parse strings directly
    local_IP.fromString(STATIC_IP_ADDR);
    gateway.fromString(STATIC_GATEWAY);
    subnet.fromString(STATIC_SUBNET);
    primaryDNS.fromString(STATIC_DNS1);
    secondaryDNS.fromString(STATIC_DNS2);
        
    Serial.print("Using Static IP: "); Serial.println(STATIC_IP_ADDR);
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
            Serial.println("STA Failed to configure");
    }
    #endif

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    _lastWifiCheck = 0;
    _wifiConnected = false;
    _timeSynced = false;
    
    // Initialize NTP
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, ntpServer);
}

void RamzanNetworkManager::update() {
    // Check connection status
    if (WiFi.status() == WL_CONNECTED) {
        if (!_wifiConnected) {
            _wifiConnected = true;
            Serial.println("\nWiFi Connected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
        }
        
        // Check for time sync if not yet synced
        if (!_timeSynced) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo, 0)) { 
                _timeSynced = true;
                Serial.println("NTP Time Synced!");
            }
        }
    } else {
        // DISCONNECTED LOGIC
        if (_wifiConnected) {
            _wifiConnected = false;
            Serial.println("\nWiFi Lost! Will attempt reconnect...");
        }

        // Retry every 10 seconds
        static unsigned long lastReconnectAttempt = 0;
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 10000) {
            lastReconnectAttempt = now;
            Serial.println("Reconnecting to WiFi...");
            WiFi.disconnect();
            WiFi.reconnect();
        }
    }
}

bool RamzanNetworkManager::isConnected() {
    return _wifiConnected;
}

bool RamzanNetworkManager::isTimeSynced() {
    return _timeSynced;
}

String RamzanNetworkManager::getFormattedTime() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return "00:00:00";
    }
    char timeStringBuff[10];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
    return String(timeStringBuff);
}

String RamzanNetworkManager::getFormattedDate() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return "--/--";
    }
    char dateStringBuff[10];
    strftime(dateStringBuff, sizeof(dateStringBuff), "%d/%m", &timeinfo);
    return String(dateStringBuff);
}

int RamzanNetworkManager::getCurrentHour() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return -1;
    return timeinfo.tm_hour;
}

int RamzanNetworkManager::getCurrentMinute() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return -1;
    return timeinfo.tm_min;
}

int RamzanNetworkManager::getCurrentSecond() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return -1;
    return timeinfo.tm_sec;
}
