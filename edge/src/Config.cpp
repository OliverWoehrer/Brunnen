#include "Config.h"

#define MUTEX_TIMEOUT (2*1000)/portTICK_PERIOD_MS // 2000 ms

/**
 * Default constructor initalizes the preferences and mounts the flash memory
 */
ConfigClass::ConfigClass() : preferences() {
    this->semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use config semaphore.");
    }
}

/**
 * Writes the given interval at the given index into preferences
 * @param interval interval struct to be stored
 * @param index index of intervall
 */
void ConfigClass::storePumpInterval(interval_t interval, size_t index) {
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
void ConfigClass::storePumpIntervals(std::vector<interval_t>& intervals) {
    size_t i = 0;
    for(; i < intervals.size(); i++) {
        this->storePumpInterval(intervals[i], i);
    }
    // TODO: remove old pump intervals
}

/**
 * Read pump interval from memory from the given index
 * @param index index to read from
 * @return interval struct read from memory
 */
interval_t ConfigClass::loadPumpInterval(size_t index) {
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
 * @brief Loads all intervals from memory
 * @param intervals vector of intervals, to read into
 */
void ConfigClass::loadPumpIntervals(std::vector<interval_t>& intervals) {
    for(size_t i = 0; i < intervals.capacity(); i++) {
        intervals.push_back(this->loadPumpInterval(i));
    }
}

/**
 * @brief Removes the pump interval at the given index from memory. This will work if there is no
 * interval at the index
 * @param index number of interval to delete
 */
void ConfigClass::deletePumpInterval(size_t index) {
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

    // Delete From Memory:
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.remove(startHrString);
    this->preferences.remove(startMinString);
    this->preferences.remove(stopHrString);
    this->preferences.remove(stopMinString);
    this->preferences.remove(wdayString);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Write the number of jobs into preferences
 * @param jobLength number to store
 */
void ConfigClass::storeJobLength(size_t jobLength) {
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
size_t ConfigClass::loadJobLength() {
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
void ConfigClass::storeJob(const char* fileName, size_t index) {
    // Build Job Key:
    char jobKey[7]; // format: job_XX
    sprintf(jobKey, "job_%02d", index);

    // Write To Memory:
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putString(jobKey, fileName);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Read the job (=filename) from preferences at the given index
 * @param index index to read from
 * @return filename of datafile
 */
std::string ConfigClass::loadJob(size_t index) {
    // Build Job Key:
    char jobKey[7]; // format: job_XX;
    sprintf(jobKey,"job_%02d", index);

    // Read From Memory:
    char buffer[50]; // max. filename length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getString(jobKey, buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore

    // Return Filename:
    return buffer;
}

/**
 * Remove the job(=filename) from the preferences at the given index
 * @param index index to delete from
 */
void ConfigClass::deleteJob(size_t index) {
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
void ConfigClass::storeRainThresholdLevel(uint8_t level) {
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
uint8_t ConfigClass::loadRainThresholdLevel() {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    uint8_t threshold = (uint8_t)this->preferences.getUChar("threshold");
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return threshold;
}

/**
 * Takes a mail address and stores it into flash memory
 * @param addres address string
 */
void ConfigClass::storeMailAddress(const char* address) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putString("mail_address", address);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Loads the (e-mail) address from flash memory
 * @return address string from memory
 */
std::string ConfigClass::loadMailAddress() {
    // Read From Memory:
    char buffer[50]; // max. password length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getString("mail_address", buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    
    // Return Password:
    return buffer;
}

/**
 * Takes a (e-mail) password and stores it into flash memory
 * @param pw password string to store
 */
void ConfigClass::storeMailPassword(const char* pw) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putString("password", pw);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Loads the (e-mail) password from flash memory
 * @return password string from memory
 */
std::string ConfigClass::loadMailPassword() {
    // Read From Memory:
    char buffer[50]; // max. password length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getString("password", buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    
    // Return Password:
    return buffer;
}

void ConfigClass::storeAPIHost(const char* host) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putString("host", host);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

std::string ConfigClass::loadAPIHost() {
    // Read From Memory:
    char buffer[50]; // max. host length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getString("host", buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    
    // Return Host Name:
    return buffer;
}

void ConfigClass::storeAPIPort(size_t port) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putUInt("port", port);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

size_t ConfigClass::loadAPIPort() {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    size_t port = (size_t)this->preferences.getUInt("port", 80);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return port;
}

void ConfigClass::storeAPIPath(const char* path) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putString("path", path);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

std::string ConfigClass::loadAPIPath() {
    // Read From Memory:
    char buffer[100]; // max. path length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getString("path", buffer, 100);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    
    // Return Path:
    return buffer;
}

/**
 * Takes a authentication password and stores it into flash memory
 * @param username authentication username for api
 */
void ConfigClass::storeAPIUsername(const char* username) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putString("api_username", username);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Loads the authentication username for the api from flash memory
 * @return authentication password from memory
 */
std::string ConfigClass::loadAPIUsername() {
    // Read From Memory:
    char buffer[50]; // max. password length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getString("api_username", buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore

    // Return Key:
    return buffer;
}

/**
 * Takes a authentication password and stores it into flash memory
 * @param password authentication password for api
 */
void ConfigClass::storeAPIPassword(const char* password) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putString("api_password", password);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Loads the authentication password for the api from flash memory
 * @return authentication password from memory
 */
std::string ConfigClass::loadAPIPassword() {
    // Read From Memory:
    char buffer[50]; // max. password length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    this->preferences.getString("api_password", buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore

    // Return Key:
    return buffer;
}

/**
 * Takes the name of the firmware version and stores it into flash memory
 * @param version name of the firmware version
 */
void ConfigClass::storeFirmwareVersion(const char* version) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, false);
    this->preferences.putString("fw_version", version);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
}

/**
 * Loads the firmware version from flash memory
 * @return name of the current firmeware version
 */
std::string ConfigClass::loadFirmwareVersion() {
    // Read From Memory:
    char buffer[50]; // max. password length
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    this->preferences.begin(CONFIG_NAME, true);
    size_t len = this->preferences.getString("fw_version", buffer, 50);
    this->preferences.end();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore

    // Return Key:
    if(!len) {
        return "1970-01-01T00:00:00";
    }
    return buffer;
}

ConfigClass Config = ConfigClass();