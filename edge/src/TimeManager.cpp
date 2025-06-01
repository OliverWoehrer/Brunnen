#include "TimeManager.h"

tm getDefault() {
    struct tm timeinfo;
    timeinfo.tm_sec = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_min = 0;
    timeinfo.tm_hour = 0;
    timeinfo.tm_mday = 1;
    timeinfo.tm_mon = 0;
    timeinfo.tm_year = 70;
    timeinfo.tm_wday = 4;
    timeinfo.tm_yday = 0;
    timeinfo.tm_isdst = 0;
    return timeinfo; // 1970-01-01 00:00:00
}

/**
 * Default constructor
 */
TimeManager::TimeManager() {}

/**
 * Initalizes the time system time by connecting to the NTP server. WiFi needs to be enabled for
 * this.
 * @return true on success, false otherwise
 */
bool TimeManager::begin() {
    // Set NTP Configuration: 
    configTime(GMT_TIME_ZONE, DAYLIGHT_OFFSET, NTP_SERVER);
    vTaskDelay(700 / portTICK_PERIOD_MS); // wait 700ms
    
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) { // check for local time
        log_d("Failed to config time");
        return false;
    }
    log_d("Time initialized at %s", toString(timeinfo).c_str());
    return true;
}

/**
 * @brief Returns the timeinfo holding the system time
 * @return timeinfo of 'this'
 */
tm TimeManager::getTime() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)) {
        log_w("Getting default time. This should not happend in regular operation.");
        return getDefault();
    }
    return timeinfo;
}

/**
 * @brief Converts timeinfo of 'this' to ISO format string
 * @return string in the format YYYY-MM-DDTHH:MM:SS, empty on failure
 */
std::string TimeManager::toString() {
    tm timeinfo = this->getTime();
    return TimeManager::toString(timeinfo);
}

/**
 * @brief Converts the given timeinfo to a string of ISO format YYYY-MM-DD
 * @return string on success, empty string on failure
 */
std::string TimeManager::toDateString() {
    const tm timeinfo = this->getTime();
    return TimeManager::toDateString(timeinfo);
}

/**
 * @brief Converts the given timeinfo to a string of ISO format HH:MM:SS
 * @param timeinfo time struct to convert
 * @return string on success, empty string on failure
 */
std::string TimeManager::toTimeString() {
    const tm timeinfo = this->getTime();
    return TimeManager::toTimeString(timeinfo);
}

/**
 * @brief Parses the given string into a time struct
 * @param timestring string in the format HH:MM:SS
 * @param timeinfo parsed time struct (invalid on failure)
 * @return true on success, false otherwise
 */
bool TimeManager::fromTimeString(const char* timestring, tm &timeinfo) {
    char* endptr = strptime(timestring, TIME_STRING_FORMAT, &timeinfo);
    if(endptr == NULL) {
        log_w("Failed to parse time from '%s'", timestring);
        return false;
    }
    return true;
}

/**
 * @brief Parses the given string into a time struct
 * @param timestring string in the format YYYY-MM-DDTHH:MM:SS
 * @param timeinfo parsed time struct (invalid on failure)
 * @return true on success, false otherwise
 */
bool TimeManager::fromDateTimeString(const char* timestring, tm &timeinfo) {
    char* endptr = strptime(timestring, DATETIME_STRING_FORMAT, &timeinfo);
    if(endptr == NULL) {
        log_w("Failed to parse datetime from '%s'", timestring);
        return false;
    }
    return true;
}

/**
 * @brief Converts the given timeinfo to a string of ISO format YYYY-MM-DDTHH:MM:SS
 * @param timeinfo time struct to convert
 * @return string on success, empty string on failure
 */
std::string TimeManager::toString(tm timeinfo) {
    char buffer[TIME_STRING_LENGTH];
    size_t bytes = strftime(buffer, TIME_STRING_LENGTH, DATETIME_STRING_FORMAT, &timeinfo);
    if(bytes == 0) {
        return "";
    }
    return buffer;
}

/**
 * @brief Converts the given timeinfo to a string of ISO format YYYY-MM-DD
 * @param timeinfo time struct to convert
 * @return string on success, empty string on failure
 */
std::string TimeManager::toDateString(tm timeinfo) {
    char buffer[TIME_STRING_LENGTH];
    size_t bytes = strftime(buffer, TIME_STRING_LENGTH, DATE_STRING_FORMAT, &timeinfo);
    if(bytes == 0) {
        return "";
    }
    return buffer;
}

/**
 * @brief Converts the given timeinfo to a string of ISO format HH:MM:SS
 * @param timeinfo time struct to convert
 * @return string on success, empty string on failure
 */
std::string TimeManager::toTimeString(tm timeinfo) {
    char buffer[TIME_STRING_LENGTH];
    size_t bytes = strftime(buffer, TIME_STRING_LENGTH, TIME_STRING_FORMAT, &timeinfo);
    if(bytes == 0) {
        return "";
    }
    return buffer;
}

TimeManager Time = TimeManager();
