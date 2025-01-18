#include "DataFile.h"

#define MUTEX_TIMEOUT (2*1000)/portTICK_PERIOD_MS // 2000 ms

/**
 * Constructor initalizes the file system (external SD card)
 */
DataFileClass::DataFileClass() : FileManager(SD) {
    this->semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use data file semaphore");
    }
}

/**
 * @brief Mounts the extern file system (external SD card)
 * @return true on success, false otherwise
 */
bool DataFileClass::begin() {
    if(!SD.begin(SPI_CD)) {
        log_e("Failed to mount SD card");
        return false;
    }
    return true;
}

/**
 * Sets a new active data file. This essentially sets a (new) file path, so all future file
 * operations are executed on the given file.
 * @param filename 
 * @return 
 */
bool DataFileClass::init(std::string filename) {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    this->filename = filename;
    bool success = true;
    if(!checkFile(this->filename.c_str())) { // check if file exisits
        success = writeLine(this->filename.c_str(), "Timestamp,Flow,Pressure,Level"); // write header on new file
    }
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }
    return success;
}

/**
 * Takes the given sensor data and writes it as a new line in this data file. The data file itself
 * uses CSV format. 
 * @param data sensor data to be stored to file
 * @return true on success, false otherwise
 */
bool DataFileClass::store(sensor_data_t data) {
    char buffer[DATA_STRING_LENGTH]; // format: "TIME,FLOW,PRESSURE,LEVEL<CR><LF>"
    size_t bytes = sprintf(buffer, "%s,%d,%d,%d", TimeManager::toString(data.timestamp).c_str(), data.flow, data.pressure, data.level);
    if(bytes == 0) {
        log_e("Failed to build line from sensor data");
        return false;
    }
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    bool success = appendLine(this->filename.c_str(), buffer);
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }
    return success;
}

/**
 * Extracts sensor data from the beginning of the data file and writes it into the given data
 * buffer. Depending on the size of 'data' it tries to read that many lines and parse them.
 * @param data buffer to be filled. Needs to be allocated with reserve(), so 'data.capacity()' works
 * @return number of lines that could succesfully be parsed, -1 on error
 */
bool DataFileClass::exportData(std::vector<sensor_data_t>& data) {
    // Read Data File:
    std::vector<std::string> lines;
    lines.reserve(data.capacity());
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    bool success = this->readLines(this->filename.c_str(), lines);
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }
    if(!success) {
        log_e("Failed to read lines from file");
        return false;
    }

    /*std::vector<std::string> lines = {
        "Timestamp,Flow,Pressure,Level",
        "2024-03-31T14:32:19,0,920,2541",
        "2024-03-31T14:32:20,0,930,2539",
        "2024-03-31T14:32:21,0,926,2539",
        "2024-03-31T14:32:22,0,926,2575",
        "2024-03-31T14:32:23,0,923,2544",
        "2024-03-31T14:32:24,0,926,2549",
        "2024-03-31T14:32:25,0,929,2545"
    };*/

    // Parse CSV Lines:
    for(std::string line : lines) {
        sensor_data_t d;
        if(parseCSVLine(line.c_str(), d)) {
            data.push_back(d);
        }
    }

    // Return Count of Actually Read Lines:
    log_d("Exported %d/%d lines", data.size(), data.capacity());
    return true;
}

/**
 * Strips the first numLines-1 lines of the active data file. This is done by copying lines,
 * into a new file and deleting the old one. The first line after shrinking, will be line number
 * 'numLines'
 * @param numLines line number of the first line to keep 
 * @return true on success, false otherwise
 */
bool DataFileClass::shrinkData(size_t numLines) {    
    bool success;
    if(!this->createFile("/temp.txt")) {
        log_e("Failed to create temporary copy file");
        return false;
    }
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    success = this->copyFile(this->filename.c_str(), "/temp.txt", numLines);
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }
    if(!success) {
        log_e("Failed to copy content");
        return false;
    }
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    success = this->deleteFile(this->filename.c_str());
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }
    if(!success) {
        log_e("Failed to remove log file");
        return false;
    }
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    success = this->renameFile("/temp.txt", this->filename.c_str());
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }
    if(!success) {
        log_e("Failed to rename temporary file to data file");
        return false;
    }
    return true;
}

/**
 * Clears the active data file so it is empty afterwards
 * @return true on success, false otherwise
 */
bool DataFileClass::clear() {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    bool success = this->clearFile(this->filename.c_str());
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }
    return success;
}

/**
 * @brief Deletes the data file and sets the filename to "" (emtpy string). To re-use the data file
 * again use init() to create a new document.
 * @return true on success, false otherwise
 */
bool DataFileClass::remove() {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    bool success = this->deleteFile(this->filename.c_str());
    this->filename = "";
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }
    return success;
}

/**
 * Return the filename of the active data file
 * @return string of the file path
 */
std::string DataFileClass::getFilename() {
    return this->filename;
}

/**
 * @brief Retreive the number of lines in the data file
 * @return line counter
 */
size_t DataFileClass::lineCounter() {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    size_t count = this->lineCount(this->filename.c_str());
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }
    return count;
}

/**
 * Tries to parse sensor data from the given line (CSV format) into the sensor data.
 * @param line string holding the CSV line in format TIME,FLOW,PRESSURE,LEVEL
 * @param data sensor data struct to be filled with parsed values
 * @return true on success, false if one of values failed to parse
 */
bool DataFileClass::parseCSVLine(const char line[], sensor_data_t& data) {
    // Copy Into Local String Buffer:
    size_t len = strlen(line);
    char buffer[len+1];
    strncpy(buffer, line, len+1);

    // Parse Time String:
    char* token = strtok(buffer,",");
    if(token == NULL) {
        return false;
    }
    if(!TimeManager::fromDateTimeString(token, data.timestamp)) {
        return false;
    }

    // Parse Flow Value:
    token = strtok(NULL, ",");
    if(token == NULL) {
        return false;
    }
    data.flow = atoi(token);

    // Parse Pressure Value:
    token = strtok(NULL, ",");
    if(token == NULL) {
        return false;
    }
    data.pressure = atoi(token);

    // Parse Level Value:
    token = strtok(NULL, ",");
    if(token == NULL) {
        return false;
    }
    data.level = atoi(token);

    // Return Success:
    return true;
}

DataFileClass DataFile = DataFileClass();