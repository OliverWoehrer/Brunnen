#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include "time.h"
#include "Arduino.h"

// Time String:
#define TIME_STRING_LENGTH 21
#define DATE_STRING_FORMAT "%Y-%m-%d" // YYYY-MM-DD
#define TIME_STRING_FORMAT "%H:%M:%S" // HH:MM:SS
#define DATETIME_STRING_FORMAT "%Y-%m-%dT%H:%M:%S" // YYYY-MM-DDTHH:MM:SS

// NTP Configuration:
#define NTP_SERVER "pool.ntp.org"
#define GMT_TIME_ZONE 3600
#define DAYLIGHT_OFFSET 3600

class TimeManager {
public:
    TimeManager();
    bool begin();
    tm getTime();
    std::string toString();
    std::string toDateString();
    std::string toTimeString();
    static bool fromTimeString(const char* timestring, tm &timeinfo);
    static bool fromDateTimeString(const char* timestring, tm &timeinfo);
    static std::string toString(tm timeinfo);
    static std::string toDateString(tm timeinfo);
    static std::string toTimeString(tm timeinfo);
};

extern TimeManager Time;

#endif /* TIME_MANAGER_H */