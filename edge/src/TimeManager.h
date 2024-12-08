#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include "time.h"
#include "Arduino.h"

// Time String:
#define TIME_STRING_LENGTH 20
#define TIME_STRING_FORMAT "%Y-%M-%dT%H:%M:%S" // YYYY-MM-DDTHH:MM:SS

// NTP Configuration:
#define NTP_SERVER "pool.ntp.org"
#define GMT_TIME_ZONE 3600
#define DAYLIGHT_OFFSET 3600

class TimeManager {
public:
    TimeManager();
    bool init();
    tm getTime();
    std::string toString();
    static std::string toString(tm timeinfo);
    static tm fromString(const char* infostring);
    static std::string toTime(tm timeinfo);
    static tm fromTime(const char* timestring);
};

extern TimeManager Time;

#endif /* TIME_MANAGER_H */