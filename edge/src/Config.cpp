#include "Config.h"

#define MUTEX_TIMEOUT (2*1000)/portTICK_PERIOD_MS // 2000 ms

/**
 * Default constructor initalizes the preferences and mounts the flash memory
 */
Config::Config() : preferences() {
    this->semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use data file semaphore.");
    }
    bool ret = this->preferences.begin(CONFIG_NAME, false);
    this->preferences.end();
    Serial.printf("Config();\r\n");
}

/**
 * Writes the given interval at the given index into preferences
 * @param interval interval struct to be stored
 * @param index index of intervall
 */
void Config::storePumpInterval(interval_t interval, size_t index) {
    // Build Start Time Key:
    char startHrString[14]; // format: start_hour_XX
    char startMinString[13]; // format: start_min_XX
    sprintf(startHrString, "start_hour_%02d", index);
    sprintf(startMinString, "start_min_%02d", index);

    // Build Stop Time Key:
    char stopHrString[13]; // format: stop_hour_XX
    char stopMinString[12]; // format: stop_min_XX
    sprintf(stopHrString, "stop_hour_%02d", index);
    sprintf(stopMinString, "stop_min_%02d", index);

    // Build Weekday Key:
    char wdayString[8]; // format: "wday_XX"
    sprintf(wdayString, "wday_%02d", index);

    // Write to Memory:
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putUChar(startHrString, interval.start.tm_hour);
    this->preferences.putUChar(startMinString, interval.start.tm_min);
    this->preferences.putUChar(stopHrString, interval.stop.tm_hour);
    this->preferences.putUChar(stopMinString, interval.stop.tm_min);
    this->preferences.putUChar(wdayString, interval.wday);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Writes all intervals in the given vector into preferences. The first interval will be stored
 * at index 0.
 * @param intervals vector of intervals, to be stored
 */
void Config::storePumpIntervals(std::vector<interval_t>& intervals) {
    for(size_t i = 0; i < intervals.size(); i++) {
        this->storePumpInterval(intervals[i], i);
    }
}

/**
 * Read pump interval from memory from the given index
 * @param index index to read from
 * @return interval struct read from memory
 */
interval_t Config::loadPumpInterval(size_t index) {
    // Build Start Time Key:
    char startHrString[14]; // format: start_hour_XX
    char startMinString[13]; // format: start_min_XX
    sprintf(startHrString, "start_hour_%02d", index);
    sprintf(startMinString, "start_min_%02d", index);

    // Build Stop Time Key:
    char stopHrString[13]; // format: stop_hour_XX
    char stopMinString[12]; // format: stop_min_XX
    sprintf(stopHrString, "stop_hour_%02d", index);
    sprintf(stopMinString, "stop_min_%02d", index);

    // Build Weekday Key:
    char wdayString[8]; // format: "wday_XX"
    sprintf(wdayString, "wday_%02d", index);

    // Read From Memory:
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);    
    tm start;
    start.tm_hour = (int)this->preferences.getUChar(startHrString, 0);
    start.tm_min = (int)this->preferences.getUChar(startMinString, 0);
    start.tm_sec = 0;
    tm stop;
    stop.tm_hour = (int)this->preferences.getUChar(stopHrString, 0);
    stop.tm_min = (int)this->preferences.getUChar(stopMinString, 0);
    stop.tm_sec = 0;
    uint8_t weekday = (uint8_t)this->preferences.getUChar(wdayString, 0);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore

    // Return Interval:
    interval_t inter = { .start = start, .stop = stop, .wday = weekday };
    return inter;
}

/**
 * Write the number of jobs into preferences
 * @param jobLength number to store
 */
void Config::storeJobLength(size_t jobLength) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putUChar("jobLength", jobLength);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Read the number of jobs from preferences
 * @return number of jobs available
 */
size_t Config::loadJobLength() {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    size_t jl = (size_t)this->preferences.getUChar("jobLength", 0);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return jl;
}

/**
 * Writes the given job (=filename) into preferences at the given index
 * @param fileName filename of the datafile to store
 * @param index index to write at preferences
 */
void Config::storeJob(const char* fileName, size_t index) {
    // Build Job Key:
    char jobKey[7]; // format: job_XX
    sprintf(jobKey, "job_%02d", index);

    // Write To Memory:
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putBytes(jobKey, fileName, strlen(fileName));
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Read the job (=filename) from preferences at the given index
 * @param index index to read from
 * @return filename of datafile
 */
std::string Config::loadJob(size_t index) {
    // Build Job Key:
    char jobKey[7]; // format: job_XX;
    sprintf(jobKey,"job_%02d", index);

    // Read From Memory:
    char buffer[50]; // max. filename length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getBytes(jobKey, buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore

    // Return Filename:
    return buffer;
}

/**
 * Remove the job(=filename) from the preferences at the given index
 * @param index index to delete from
 */
void Config::deleteJob(size_t index) {
    // Build Job Key:
    char jobKey[7]; // format: job_XX
    sprintf(jobKey, "job_%02d", index);

    // Delete From Memory:
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.remove(jobKey);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Write the given threshold into preferences memory
 * @param level threshold values to store
 */
void Config::storeRainThresholdLevel(uint8_t level) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putUChar("threshold", level);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Read the threshold from preferences memory
 * @return threshold value from memory
 */
uint8_t Config::loadRainThresholdLevel() {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    uint8_t threshold = (uint8_t)this->preferences.getUChar("threshold");
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return threshold;
}

/**
 * Takes a (e-mail) password and stores it into flash memory
 * @param pw password string to store
 */
void Config::storePassword(const char* pw) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putBytes("password", pw, strlen(pw));
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Loads the (e-mail) password from flash memory
 * @return password string from memory
 */
std::string Config::loadPassword() {
    // Read From Memory:
    char buffer[50]; // max. password length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getBytes("password", buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    
    // Return Password:
    return buffer;
}

/**
 * Takes a authentication key and stores it into flash memory
 * @param key authentication key to store
 */
void Config::storeAuthKey(const char* key) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putBytes("auth_key", key, strlen(key));
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Loads the authentication key from flash memory
 * @return authentication key from memory
 */
std::string Config::loadAuthKey() {
    // Read From Memory:
    char buffer[50]; // max. password length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getBytes("auth_key", buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore

    // Return Key:
    return buffer;
}
