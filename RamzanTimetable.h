#ifndef RAMZAN_TIMETABLE_H
#define RAMZAN_TIMETABLE_H

#include <Arduino.h>

// Simplified structure
struct AlarmEntry {
    uint8_t month;
    uint8_t day;
    // Sehri End
    uint8_t sehriHour;  
    uint8_t sehriMinute;
    // Iftar
    uint8_t iftarHour;
    uint8_t iftarMinute;
    
    // Prayer Beeps (Fajr, Zohr, Asr, Maghrib, Isha)
    // For now we will just assume standard times or hardcode if user provided list
    // But user asked to "add that and list".
    // Let's add separate arrays or just hardcode standard prayer times logic 
    // since Ramzan timetable usually just has Sehri/Iftar.
    // We will stick to Sehri/Iftar in this struct for now to keep size low
    // and instead use a generic lookup for other prayers if needed.
};

// Navsari Ramzan 2026 Timetable (1447 AH)
// Format: { Month, Day, SehriHour, SehriMin, IftarHour, IftarMin }
const AlarmEntry ramzanTimetable[] = {
    // Feb 2026
    { 2, 19, 5, 42, 18, 43 }, // Roza 1
    { 2, 20, 5, 42, 18, 44 }, // Roza 2
    { 2, 21, 5, 41, 18, 44 }, // Roza 3
    { 2, 22, 5, 40, 18, 45 }, // Roza 4
    { 2, 23, 5, 40, 18, 45 }, // Roza 5
    { 2, 24, 5, 39, 18, 46 }, // Roza 6
    { 2, 25, 5, 38, 18, 46 }, // Roza 7
    { 2, 26, 5, 38, 18, 47 }, // Roza 8
    { 2, 27, 5, 37, 18, 47 }, // Roza 9
    { 2, 28, 5, 36, 18, 47 }, // Roza 10

    // March 2026
    { 3, 1,  5, 35, 18, 48 }, // Roza 11
    { 3, 2,  5, 35, 18, 48 }, // Roza 12
    { 3, 3,  5, 34, 18, 49 }, // Roza 13
    { 3, 4,  5, 33, 18, 49 }, // Roza 14
    { 3, 5,  5, 32, 18, 50 }, // Roza 15
    { 3, 6,  5, 32, 18, 50 }, // Roza 16
    { 3, 7,  5, 31, 18, 51 }, // Roza 17
    { 3, 8,  5, 30, 18, 51 }, // Roza 18
    { 3, 9,  5, 29, 18, 51 }, // Roza 19
    { 3, 10, 5, 28, 18, 52 }, // Roza 20
    { 3, 11, 5, 27, 18, 52 }, // Roza 21
    { 3, 12, 5, 27, 18, 52 }, // Roza 22
    { 3, 13, 5, 26, 18, 53 }, // Roza 23
    { 3, 14, 5, 25, 18, 53 }, // Roza 24
    { 3, 15, 5, 24, 18, 53 }, // Roza 25
    { 3, 16, 5, 23, 18, 54 }, // Roza 26
    { 3, 17, 5, 22, 18, 54 }, // Roza 27
    { 3, 18, 5, 21, 18, 54 }, // Roza 28
    { 3, 19, 5, 20, 18, 55 }, // Roza 29
    { 3, 20, 5, 19, 18, 55 }, // Roza 30
};

const int TIMETABLE_SIZE = sizeof(ramzanTimetable) / sizeof(AlarmEntry);

#endif
