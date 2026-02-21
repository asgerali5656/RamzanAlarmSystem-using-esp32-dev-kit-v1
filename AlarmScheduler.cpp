#include "AlarmScheduler.h"
#include "Config.h"

void AlarmScheduler::init() {
    _currentDay = -1;
    _currentMonth = -1;
    _alarmsLoadedForToday = false;
    
    _todayAlarms.hasSehri = false;
    _todayAlarms.hasIftar = false;
    _todayAlarms.sehriTriggered = false;
    _todayAlarms.preSehriTriggered = false;
    _todayAlarms.sehriEndTriggered = false;
    _todayAlarms.iftarTriggered = false;
    _todayAlarms.fajrTriggered = false;
    _todayAlarms.zohrTriggered = false;
    _todayAlarms.asrTriggered = false;
    _todayAlarms.ishaTriggered = false;
    
    _alarmStartTime = 0;
    _lastAlarmDuration = 0;
}

void AlarmScheduler::startAlarmDurationTracking() {
    _alarmStartTime = millis();
}

void AlarmScheduler::stopAlarmDurationTracking() {
    if (_alarmStartTime > 0) {
        _lastAlarmDuration = (millis() - _alarmStartTime) / 1000;
        _alarmStartTime = 0;
    }
}

String AlarmScheduler::getLastAlarmDuration() {
    if (_lastAlarmDuration == 0) return "--";
    int m = _lastAlarmDuration / 60;
    int s = _lastAlarmDuration % 60;
    char buff[20];
    sprintf(buff, "%dm %ds", m, s);
    return String(buff);
}

void AlarmScheduler::update(RamzanNetworkManager* network) {
    if (!network->isTimeSynced()) return;

    // TODO: Add getDay() getMonth() to NetworkManager
    // For now, let's assume we add those methods or parse formatting
    // network->getTmStruct() would be better
    // Let's rely on adding parsing to NetworkManager or doing it here
    
    // TEMPORARY: Re-fetch time to parse locally
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return; 
    }
    
    int todayDay = timeinfo.tm_mday;
    int todayMonth = timeinfo.tm_mon + 1; // tm_mon is 0-11
    int h = timeinfo.tm_hour;
    int m = timeinfo.tm_min;
    
    // If new day, reload alarms
    if (todayDay != _currentDay || todayMonth != _currentMonth) {
        Serial.println("New Day Detected! Loading Alarms...");
        _currentDay = todayDay;
        _currentMonth = todayMonth;
        loadAlarmsForDate(_currentMonth, _currentDay);
        
        // Reset triggers for the new day
        _todayAlarms.sehriTriggered = false;
        _todayAlarms.preSehriTriggered = false;
        _todayAlarms.sehriEndTriggered = false;
        _todayAlarms.iftarTriggered = false;
        _todayAlarms.fajrTriggered = false;
        _todayAlarms.zohrTriggered = false;
        _todayAlarms.asrTriggered = false;
        _todayAlarms.ishaTriggered = false;
        _alarmsLoadedForToday = true;
    }

    // Auto-Mark Skipped/Completed Alarms based on current time
    // If NOW > AlarmTime, and not triggered, mark as triggered (skipped)
    // We add 1 minute buffer to be safe
    int currentMins = (h * 60) + m;

    if (_todayAlarms.hasSehri && !_todayAlarms.sehriTriggered) {
        // Marc exact sehri trigger
        if (currentMins > (_todayAlarms.sehriHour * 60 + _todayAlarms.sehriMin)) _todayAlarms.sehriTriggered = true;
        
        // Mark pre-sehri trigger if time passed
        int preMins = (_todayAlarms.sehriHour * 60 + _todayAlarms.sehriMin) - _preSehriOffsetMinutes;
        if (preMins < 0) preMins += 1440;
        if (currentMins > preMins && !_todayAlarms.preSehriTriggered) _todayAlarms.preSehriTriggered = true;
    }
    if (!_todayAlarms.fajrTriggered) {
        if (currentMins > (_todayAlarms.fajrHour * 60 + _todayAlarms.fajrMin)) _todayAlarms.fajrTriggered = true;
    }
    if (!_todayAlarms.zohrTriggered) {
        if (currentMins > (_todayAlarms.zohrHour * 60 + _todayAlarms.zohrMin)) _todayAlarms.zohrTriggered = true;
    }
    if (!_todayAlarms.asrTriggered) {
        if (currentMins > (_todayAlarms.asrHour * 60 + _todayAlarms.asrMin)) _todayAlarms.asrTriggered = true;
    }
    if (_todayAlarms.hasIftar && !_todayAlarms.iftarTriggered) {
        if (currentMins > (_todayAlarms.iftarHour * 60 + _todayAlarms.iftarMin)) _todayAlarms.iftarTriggered = true;
    }
    if (!_todayAlarms.ishaTriggered) {
        if (currentMins > (_todayAlarms.ishaHour * 60 + _todayAlarms.ishaMin)) _todayAlarms.ishaTriggered = true;
    }
}

void AlarmScheduler::loadAlarmsForDate(int month, int day) {
    _todayAlarms.hasSehri = false;
    _todayAlarms.hasIftar = false;

    // Exhaustive search
    for (int i = 0; i < TIMETABLE_SIZE; i++) {
        if (ramzanTimetable[i].month == month && ramzanTimetable[i].day == day) {
            _todayAlarms.hasSehri = true;
            // Apply Sehri Offset
            int sTotal = (ramzanTimetable[i].sehriHour * 60) + ramzanTimetable[i].sehriMinute + _sehriOffset;
            if (sTotal < 0) sTotal += 1440;
            if (sTotal >= 1440) sTotal %= 1440;
            _todayAlarms.sehriHour = sTotal / 60;
            _todayAlarms.sehriMin = sTotal % 60;
            
            _todayAlarms.hasIftar = true;
            // Apply Iftar Offset
            int iTotal = (ramzanTimetable[i].iftarHour * 60) + ramzanTimetable[i].iftarMinute + _iftarOffset;
            if (iTotal < 0) iTotal += 1440;
            if (iTotal >= 1440) iTotal %= 1440;
            _todayAlarms.iftarHour = iTotal / 60;
            _todayAlarms.iftarMin = iTotal % 60;
            
            Serial.print("Alarms Loaded (Day "); Serial.print(day); Serial.println("):");
            Serial.print("Sehri: "); Serial.print(_todayAlarms.sehriHour); Serial.print(":"); Serial.print(_todayAlarms.sehriMin); 
            Serial.print(" (Off: "); Serial.print(_sehriOffset); Serial.println(")");
            
            Serial.print("Iftar: "); Serial.print(_todayAlarms.iftarHour); Serial.print(":"); Serial.print(_todayAlarms.iftarMin);
            Serial.print(" (Off: "); Serial.print(_iftarOffset); Serial.println(")");
            
            // Auto-calculate Prayer Times
            // Fajr: 06:00
            _todayAlarms.fajrHour = 6; _todayAlarms.fajrMin = 0;
            
            // Zohr: 1:00 PM (13:00)
            _todayAlarms.zohrHour = 13; _todayAlarms.zohrMin = 0;
            
            // Asr: 5:05 PM (17:05)
            _todayAlarms.asrHour = 17; _todayAlarms.asrMin = 5;
            
            // Isha: 8:00 PM (20:00)
            _todayAlarms.ishaHour = 20; _todayAlarms.ishaMin = 0;
            
            Serial.printf("Prayers: Fajr %02d:%02d, Zohr %02d:%02d, Asr %02d:%02d, Isha %02d:%02d\n", 
                _todayAlarms.fajrHour, _todayAlarms.fajrMin,
                _todayAlarms.zohrHour, _todayAlarms.zohrMin,
                _todayAlarms.asrHour, _todayAlarms.asrMin,
                _todayAlarms.ishaHour, _todayAlarms.ishaMin);
            return;
        }
    }
    Serial.println("No Alarms found in timetable for today.");
}

int AlarmScheduler::checkAlarmTriggers(RamzanNetworkManager* network) {
    if (!_alarmsLoadedForToday || !network->isTimeSynced()) return 0;
    
    int h = network->getCurrentHour();
    int m = network->getCurrentMinute();
    int s = network->getCurrentSecond();
    

    // 1. Check Pre-Sehri
    if (_todayAlarms.hasSehri && !_todayAlarms.preSehriTriggered) {
        int sehriTotalMins = _todayAlarms.sehriHour * 60 + _todayAlarms.sehriMin;
        int preTotalMins = sehriTotalMins - _preSehriOffsetMinutes;
        if (preTotalMins < 0) preTotalMins += 1440;
        
        int preH = preTotalMins / 60;
        int preM = preTotalMins % 60;

        if (h == preH && m == preM && s < 5) {
             _todayAlarms.preSehriTriggered = true;
             return 3; // Pre-Sehri Code
        }
    }

    // 2. Check Actual Sehri End (Exact Time) -> 2 Beeps
    if (_todayAlarms.hasSehri && h == _todayAlarms.sehriHour && m == _todayAlarms.sehriMin && s < 5 && !_todayAlarms.sehriEndTriggered) {
         _todayAlarms.sehriEndTriggered = true;
         return 5; // Sehri End Beep code (maps to startPrayerBeep)
    }
    
    // 3. Check Iftar
    if (_todayAlarms.hasIftar) {
        if (h == _todayAlarms.iftarHour && m == _todayAlarms.iftarMin && s < 5 && !_todayAlarms.iftarTriggered) {
            _todayAlarms.iftarTriggered = true;
            // User Request: Iftar should ring for 1 or 2 beeps (Prayer Style)
            // We'll return code 4 which maps to startPrayerBeep()
            return 4; 
        }
    }
    
    // 4. Check Prayer Times
    if (h == _todayAlarms.fajrHour && m == _todayAlarms.fajrMin && s < 5 && !_todayAlarms.fajrTriggered) {
        _todayAlarms.fajrTriggered = true; return 4;
    }
    if (h == _todayAlarms.zohrHour && m == _todayAlarms.zohrMin && s < 5 && !_todayAlarms.zohrTriggered) {
        _todayAlarms.zohrTriggered = true; return 4;
    }
    if (h == _todayAlarms.asrHour && m == _todayAlarms.asrMin && s < 5 && !_todayAlarms.asrTriggered) {
        _todayAlarms.asrTriggered = true; return 4;
    }
    if (h == _todayAlarms.ishaHour && m == _todayAlarms.ishaMin && s < 5 && !_todayAlarms.ishaTriggered) {
        _todayAlarms.ishaTriggered = true; return 4;
    }

    return 0;
}

String AlarmScheduler::getNextAlarmTime() {
    if (!_alarmsLoadedForToday) return "--:--";
    
    char buff[10];
    
    // Check Pre-Sehri first
    if (_todayAlarms.hasSehri && !_todayAlarms.preSehriTriggered) {
        int sehriTotalMins = _todayAlarms.sehriHour * 60 + _todayAlarms.sehriMin;
        int preTotalMins = sehriTotalMins - _preSehriOffsetMinutes;
        if (preTotalMins < 0) preTotalMins += 1440;
        sprintf(buff, "%02d:%02d", preTotalMins / 60, preTotalMins % 60);
        return String(buff);
    }
    
    if (_todayAlarms.hasSehri && !_todayAlarms.sehriTriggered) {
        sprintf(buff, "%02d:%02d", _todayAlarms.sehriHour, _todayAlarms.sehriMin); return String(buff);
    }
    if (!_todayAlarms.fajrTriggered) {
        sprintf(buff, "%02d:%02d", _todayAlarms.fajrHour, _todayAlarms.fajrMin); return String(buff);
    }
    if (!_todayAlarms.zohrTriggered) {
        sprintf(buff, "%02d:%02d", _todayAlarms.zohrHour, _todayAlarms.zohrMin); return String(buff);
    }
    if (!_todayAlarms.asrTriggered) {
        sprintf(buff, "%02d:%02d", _todayAlarms.asrHour, _todayAlarms.asrMin); return String(buff);
    }
    if (_todayAlarms.hasIftar && !_todayAlarms.iftarTriggered) {
        sprintf(buff, "%02d:%02d", _todayAlarms.iftarHour, _todayAlarms.iftarMin); return String(buff);
    }
    if (!_todayAlarms.ishaTriggered) {
        sprintf(buff, "%02d:%02d", _todayAlarms.ishaHour, _todayAlarms.ishaMin); return String(buff);
    }
    
    // If all done today, show Tomorrow Sehri (Wait, tomorrow pre-sehri is better if we have the offset logic)
    // For simplicity, let's keep tomorrow sehri for now.
    return getTomorrowSehriTime(); 
}

String AlarmScheduler::getNextAlarmName() {
    if (!_alarmsLoadedForToday) return "Loading";
    
    if (_todayAlarms.hasSehri && !_todayAlarms.preSehriTriggered) return "Pre-Sehri";
    if (_todayAlarms.hasSehri && !_todayAlarms.sehriTriggered) return "Sehri";
    if (!_todayAlarms.fajrTriggered) return "Fajr";
    if (!_todayAlarms.zohrTriggered) return "Zohr";
    if (!_todayAlarms.asrTriggered) return "Asr";
    if (_todayAlarms.hasIftar && !_todayAlarms.iftarTriggered) return "Iftar";
    if (!_todayAlarms.ishaTriggered) return "Isha";
    
    return "Sehri (Tom)"; 
}

String AlarmScheduler::getTodaySehriTime() {
    if (!_alarmsLoadedForToday || !_todayAlarms.hasSehri) return "--:--";
    char buff[10];
    sprintf(buff, "%02d:%02d", _todayAlarms.sehriHour, _todayAlarms.sehriMin);
    return String(buff);
}

String AlarmScheduler::getTodayIftarTime() {
    if (!_alarmsLoadedForToday || !_todayAlarms.hasIftar) return "--:--";
    char buff[10];
    sprintf(buff, "%02d:%02d", _todayAlarms.iftarHour, _todayAlarms.iftarMin);
    return String(buff);
}

String AlarmScheduler::getTomorrowSehriTime() {
    // Find current day index
    for (int i = 0; i < TIMETABLE_SIZE; i++) {
        if (ramzanTimetable[i].month == _currentMonth && ramzanTimetable[i].day == _currentDay) {
            // Found Today. Tomorrow is i+1?
            if (i + 1 < TIMETABLE_SIZE) {
                char buff[10];
                sprintf(buff, "%02d:%02d", ramzanTimetable[i+1].sehriHour, ramzanTimetable[i+1].sehriMinute);
                return String(buff);
            }
        }
    }
    return "--:--";
}

String AlarmScheduler::getTomorrowIftarTime() {
    for (int i = 0; i < TIMETABLE_SIZE; i++) {
        if (ramzanTimetable[i].month == _currentMonth && ramzanTimetable[i].day == _currentDay) {
            if (i + 1 < TIMETABLE_SIZE) {
                char buff[10];
                sprintf(buff, "%02d:%02d", ramzanTimetable[i+1].iftarHour, ramzanTimetable[i+1].iftarMinute);
                return String(buff);
            }
        }
    }
    return "--:--";
}

String AlarmScheduler::getPrayerWarningDuration() {
    if (!_alarmsLoadedForToday || !_todayAlarms.hasSehri || _todayAlarms.sehriTriggered) return "--";

    // Warning is 1 hour before Sehri
    // Return formatted countdown or text
    return "1 Hr Before"; 
}

String AlarmScheduler::getUpcomingScheduleJson() {
    String json = "[";
    
    // Today
    if(_alarmsLoadedForToday) {
        char buff[10];
        
        // Sehri
        json += "{\"day\":\"Today\", \"name\":\"Sehri\", \"time\":\"" + getTodaySehriTime() + "\"},";
        
        // Fajr
        sprintf(buff, "%02d:%02d", _todayAlarms.fajrHour, _todayAlarms.fajrMin);
        json += "{\"day\":\"Today\", \"name\":\"Fajr\", \"time\":\"" + String(buff) + "\"},";
        
        // Zohr
        sprintf(buff, "%02d:%02d", _todayAlarms.zohrHour, _todayAlarms.zohrMin);
        json += "{\"day\":\"Today\", \"name\":\"Zohr\", \"time\":\"" + String(buff) + "\"},";
        
        // Asr
        sprintf(buff, "%02d:%02d", _todayAlarms.asrHour, _todayAlarms.asrMin);
        json += "{\"day\":\"Today\", \"name\":\"Asr\", \"time\":\"" + String(buff) + "\"},";
        
        // Iftar
        json += "{\"day\":\"Today\", \"name\":\"Iftar\", \"time\":\"" + getTodayIftarTime() + "\"},";
        
        // Isha
        sprintf(buff, "%02d:%02d", _todayAlarms.ishaHour, _todayAlarms.ishaMin);
        json += "{\"day\":\"Today\", \"name\":\"Isha\", \"time\":\"" + String(buff) + "\"},";
    }
    
    // Tomorrow (Just Sehri/Iftar for brevity, or full if needed)
    json += "{\"day\":\"Tom\", \"name\":\"Sehri\", \"time\":\"" + getTomorrowSehriTime() + "\"},";
    json += "{\"day\":\"Tom\", \"name\":\"Iftar\", \"time\":\"" + getTomorrowIftarTime() + "\"}";
    
    json += "]";
    return json;
}

long AlarmScheduler::getSecondsToNextAlarm() {
    if (!_alarmsLoadedForToday) return -1;
    
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) return -1;
    
    int nowSecs = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    
    // Helper to get secs from H, M
    auto getSecs = [](int h, int m) { return h * 3600 + m * 60; };

    long nextTriggerSecs = -1;

    // 1. Check Today's remaining alarms
    if (_todayAlarms.hasSehri && !_todayAlarms.preSehriTriggered) {
        int sehriTotalMins = _todayAlarms.sehriHour * 60 + _todayAlarms.sehriMin;
        int preTotalMins = sehriTotalMins - _preSehriOffsetMinutes;
        if (preTotalMins < 0) preTotalMins += 1440;
        nextTriggerSecs = preTotalMins * 60;
    }
    else if (_todayAlarms.hasSehri && !_todayAlarms.sehriTriggered) nextTriggerSecs = getSecs(_todayAlarms.sehriHour, _todayAlarms.sehriMin);
    else if (!_todayAlarms.fajrTriggered) nextTriggerSecs = getSecs(_todayAlarms.fajrHour, _todayAlarms.fajrMin);
    else if (!_todayAlarms.zohrTriggered) nextTriggerSecs = getSecs(_todayAlarms.zohrHour, _todayAlarms.zohrMin);
    else if (!_todayAlarms.asrTriggered) nextTriggerSecs = getSecs(_todayAlarms.asrHour, _todayAlarms.asrMin);
    else if (_todayAlarms.hasIftar && !_todayAlarms.iftarTriggered) nextTriggerSecs = getSecs(_todayAlarms.iftarHour, _todayAlarms.iftarMin);
    else if (!_todayAlarms.ishaTriggered) nextTriggerSecs = getSecs(_todayAlarms.ishaHour, _todayAlarms.ishaMin);
    
    // 2. If no more today, look for Tomorrow's first alarm (Pre-Sehri)
    if (nextTriggerSecs == -1) {
        for (int i = 0; i < TIMETABLE_SIZE; i++) {
            if (ramzanTimetable[i].month == _currentMonth && ramzanTimetable[i].day == _currentDay) {
                if (i + 1 < TIMETABLE_SIZE) {
                    int tomSehriMins = (ramzanTimetable[i+1].sehriHour * 60) + ramzanTimetable[i+1].sehriMinute + _sehriOffset;
                    int tomPreSehriMins = tomSehriMins - _preSehriOffsetMinutes;
                    if (tomPreSehriMins < 0) tomPreSehriMins += 1440;
                    
                    nextTriggerSecs = (long)tomPreSehriMins * 60;
                    // For tomorrow, we add 24 hours to the calculation
                    long diff = (86400 - nowSecs) + nextTriggerSecs;
                    return diff;
                }
            }
        }
    }
    
    if (nextTriggerSecs == -1) return -1;
    
    long diff = nextTriggerSecs - nowSecs;
    if (diff < 0) diff += 86400; // Wraparound
    return diff;
}

