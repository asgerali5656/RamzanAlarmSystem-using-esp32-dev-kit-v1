#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"

class RamzanNetworkManager {
public:
    void init();
    void update();
    bool isConnected();
    bool isTimeSynced();
    String getFormattedTime();
    String getFormattedDate(); // DD/MM
    int getCurrentHour();
    int getCurrentMinute();
    int getCurrentSecond();
    
private:
    bool _wifiConnected;
    bool _timeSynced;
    unsigned long _lastWifiCheck;
};

#endif
