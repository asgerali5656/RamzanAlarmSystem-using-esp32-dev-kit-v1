#ifndef ALARM_SCHEDULER_H
#define ALARM_SCHEDULER_H

#include <Arduino.h>
#include "RamzanTimetable.h"
#include "NetworkManager.h"

// Forward declaration to avoid circular dependency if needed
class RamzanNetworkManager;

struct DailyAlarms {
    bool hasSehri;
    int sehriHour;
    int sehriMin;
    bool sehriTriggered;
    bool preSehriTriggered;
    bool sehriEndTriggered;
    
    bool hasIftar;
    int iftarHour;
    int iftarMin;
    bool iftarTriggered;
    
    // Prayer Times (Standard)
    int fajrHour, fajrMin; bool fajrTriggered;
    int zohrHour, zohrMin; bool zohrTriggered;
    int asrHour, asrMin;   bool asrTriggered;
    int ishaHour, ishaMin; bool ishaTriggered;
};

class AlarmScheduler {
public:
    void init();
    void update(RamzanNetworkManager* network);
    
    // Checks if we need to trigger an alarm NOW
    // Returns 0 for None, 1 for Sehri, 2 for Iftar, 3 for Pre-Sehri
    int checkAlarmTriggers(RamzanNetworkManager* network);
    
    // Getters for display
    String getNextAlarmTime();
    String getNextAlarmName();
    
    // New: Look ahead
    String getTodaySehriTime();
    String getTodayIftarTime();
    String getTomorrowSehriTime();
    String getTomorrowIftarTime();
    
    // Global Configuration
    void setOffsets(int sehri, int iftar) { _sehriOffset = sehri; _iftarOffset = iftar; }
    void setPreSehriOffset(int minutes) { _preSehriOffsetMinutes = minutes; }
    int getSehriOffset() { return _sehriOffset; }
    int getIftarOffset() { return _iftarOffset; }
    int getPreSehriOffset() { return _preSehriOffsetMinutes; }
    
    String getPrayerWarningDuration();
    String getUpcomingScheduleJson(); // New method for Web UI

    // Duration Tracking
    void startAlarmDurationTracking();
    void stopAlarmDurationTracking();
    String getLastAlarmDuration();
    
    // Sleep Mode Helper
    long getSecondsToNextAlarm();
    
private:
    DailyAlarms _todayAlarms;
    int _currentDay;
    int _currentMonth;
    bool _alarmsLoadedForToday;
    
    int _sehriOffset = 0;
    int _iftarOffset = 0;
    int _preSehriOffsetMinutes = 60; // Default 1 hour before
    
    unsigned long _alarmStartTime;
    unsigned long _lastAlarmDuration; // in seconds
    
    void loadAlarmsForDate(int month, int day);
};

#endif
