#include "TimeManager.h"

/**
 * Default constructor
 */
TimeManager::TimeManager() {}

/**
 * Initalizes the time system time by connecting to the NTP server. WiFi needs to be enabled for
 * this.
 * @return true on success, false otherwise
 */
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

/**
 * 
 * @return 
 */

/**
 * Returns the timeinfo holding the system time
 * @return timeinfo of 'this'
 */
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

/**
 * Converts timeinfo of 'this' to ISO format string
 * @return string in the format YYYY-MM-DDTHH:MM:SS
 */
std::string TimeManager::toString() {
    tm timeinfo = this->getTime();
    return toString(timeinfo);
}

/**
 * Converts the given timeinfo to a ISO format string
 * @param timeinfo time struct to convert
 * @return string in the format YYYY-MM-DDTHH:MM:SS
 */
std::string TimeManager::toString(tm timeinfo) {
    char buffer[TIME_STRING_LENGTH];
    size_t bytes = strftime(buffer, TIME_STRING_LENGTH, TIME_STRING_FORMAT, &timeinfo);
    return buffer;
}

/**
 * Parses the given string into a time struct
 * @param timestring string in the format YYYY-MM-DDTHH:MM:SS
 * @return time struct
 */
tm TimeManager::fromString(const char* timestring) {
    tm timeinfo;
    strptime(timestring, TIME_STRING_FORMAT, &timeinfo);
    return timeinfo;
}

TimeManager Time = TimeManager();
