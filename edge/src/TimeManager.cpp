#include "TimeManager.h"

/**
 * Default constructor
 */
TimeManager::TimeManager() {}

bool TimeManager::init() {
    // Set NTP Configuration: 
    configTime(GMT_TIME_ZONE, DAYLIGHT_OFFSET, NTP_SERVER);
    vTaskDelay(700 / portTICK_PERIOD_MS); // wait 700ms
    
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) { // check for local time
        Serial.printf("Failed to config time.\r\n");
        return false;
    }
    Serial.printf("Time initialized at %s\r\n", toString(timeinfo));
    return true;
}

tm TimeManager::getTime() {
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)) {
        return timeinfo;
    } // else: set time manually
    timeinfo.tm_sec = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_hour = 0;
    timeinfo.tm_mday = 0;
    timeinfo.tm_mon = 0;
    timeinfo.tm_year = 0;
    timeinfo.tm_wday = 0;
    timeinfo.tm_yday = 0;
    timeinfo.tm_isdst = 0;
    return timeinfo;    
}

char* TimeManager::toString() {
    tm timeinfo = this->getTime();
    return toString(timeinfo);
}

char* TimeManager::toString(tm timeinfo) {
    char timeString[TIME_STRING_LENGTH];
    size_t bytes = strftime(timeString, TIME_STRING_LENGTH, TIME_STRING_FORMAT, &timeinfo);
    if(bytes == 0) {
        return NULL;
    }
    return timeString;
}

tm TimeManager::fromString(const char* timestring) {
    tm timeinfo;
    strptime(timestring, TIME_STRING_FORMAT, &timeinfo);
    return timeinfo;
}

TimeManager Time = TimeManager();
