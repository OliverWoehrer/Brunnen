#include "DataFile.h"

#define MUTEX_TIMEOUT (1*1000)/portTICK_PERIOD_MS // in milliseconds
#define MAX_CACHE_SIZE 120

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
    
    // Write Header To File:
    this->filename = filename;
    bool success = false;
    do { // open scope for critical section
        if(!checkFile(this->filename.c_str())) { // check if file works; try to re-create file, if broken
            log_w("Data file broken or not found");
            deleteFile(this->filename.c_str());
            if(!createFile(this->filename.c_str())) {
                log_e("Could not create new data file");
                break;
            }
            success = writeLine(this->filename.c_str(), "Timestamp,Flow,Pressure,Level"); // write header on new file
        }
        success = true;
    } while (0); // exit critical section

    // Give Mutex Semaphore Back:
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
    if(this->cache.size() >= MAX_CACHE_SIZE) { // check if cache is full
        log_e("Failed to store sensor data because cache is full");
        return false;
    }

    // Store Sensor Data:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    this->cache.push_back(data);
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }

    // Check Cache Size:
    if(this->cache.size() < MAX_CACHE_SIZE - 2) { // check if cache needs to be flushed
        return true; // no data reallocation needed, return early   
    }

    log_d("cache is (nearly) full [size = %u], copy data to file", this->cache.size()); 

    // Copy Data From Cache to Local Buffer:
    std::vector<sensor_data_t> cacheCopy; // local copy of cache for critical section
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    cacheCopy.assign(this->cache.begin(), this->cache.end());
    if (!xSemaphoreGive(this->semaphore)) {
        log_d("Failed to give semaphore (copy)");
        return false;
    }

    // Copy Data From Cache to File:
    bool success = true;
    for(const sensor_data_t& data : cacheCopy) {
        char buffer[DATA_STRING_LENGTH]; // format: "TIME,FLOW,PRESSURE,LEVEL<CR><LF>"
        size_t bytes = sprintf(buffer, "%s,%d,%d,%d", TimeManager::toString(data.timestamp).c_str(), data.flow, data.pressure, data.level);
        if(bytes == 0) {
            log_e("Failed to build line from sensor data");
            success = false;
            break; // exit early on error
        }
        if(!appendLine(this->filename.c_str(), buffer)) {
            log_e("Failed to append line to data file");
            success = false;
            break; // exit early on error
        }
    }

    if(!success) {
        log_e("Failed to copy data from cache to file");
        return false;
    }

    log_d("data copied to file, shrink cache");

    // Clear Moved Data From Cache:
    if(!shrinkCache(cacheCopy.size())) { // shrink by the number of items just written
        log_e("Failed to shrink cache");
        return false;
    }

    log_d("cache shrunk [size = %u]", this->cache.size());
    return true;
}

/**
 * Extracts sensor data from the beginning of the data file and writes it into the given data
 * buffer. Depending on the size of 'data' it tries to read that many lines and parse them.
 * @param data buffer to be filled. Needs to be allocated with reserve(), so 'data.capacity()' works
 * @return number of lines that could succesfully be parsed, -1 on error
 */
bool DataFileClass::exportData(std::vector<sensor_data_t>& data) {
    size_t fSize = this->fileSize(this->filename.c_str());
    if(fSize < 5) { // check if file is empty
        log_d("Export from cache (cache size = %u elements)",this->cache.size());

        // Read From Cache:
        auto iter = this->cache.begin(); // get cache iterator
        auto end = std::next(iter, std::min(this->cache.size(), data.capacity())); // advance by maximum of 'capacity' steps

        // Copy From Cache to Buffer:
        if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
            log_e("Could not take semaphore");
            return false;
        }
        for (; iter != end; ++iter) { // iterate at most the available size of data buffer
            sensor_data_t element = (*iter);
            data.push_back(element);
        }
        if(!xSemaphoreGive(this->semaphore)) { // give mutex semaphore back
            log_d("Failed to give semaphore");
            return false;
        }
    } else {
        log_d("Export from file (file size = %u bytes)", fSize);

        // Read From File:
        std::vector<std::string> lines;
        lines.reserve(data.capacity());
        
        // Read Data File:
        if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
            log_e("Could not take semaphore");
            return false;
        }
        bool success = this->readLines(this->filename.c_str(), lines);
        if(!xSemaphoreGive(this->semaphore)) { // give mutex semaphore back
            log_d("Failed to give semaphore");
            return false;
        }
        if(!success) {
            log_e("Failed to read lines from file");
            return false;
        }

        // Parse CSV Lines:
        for(std::string line : lines) {
            sensor_data_t d;
            if(parseCSVLine(line.c_str(), d)) {
                data.push_back(d);
            }
        }
    }

    // Return Count of Actually Read Lines:
    log_d("Exported %d/%d lines", data.size(), data.capacity());
    return true;
}

/**
 * Strips the first 'numLines' lines of the active data file. This is done by copying lines,
 * into a new file and deleting the old one. The first line after shrinking, will be line number
 * 'numLines'. If there is no data in the file, the cache is cleared instead, following the same
 * principels.
 * @param numLines line number of the first line to keep 
 * @return true on success, false otherwise
 */
bool DataFileClass::shrinkData(size_t numLines) {
    log_d("shrink by %u lines", numLines);

    if(this->fileSize(this->filename.c_str()) < 5) { // check if file is empty
        return shrinkCache(numLines); // file already empty, shrink cache instead
    } // else: shrink non-empty file 

    // Create Temporary File:
    bool success;
    if(!this->createFile("/temp.txt")) {
        log_e("Failed to create temporary copy file");
        return false;
    }

    // Copy Data to Temporary File:
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

    // Remove Data File:
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

    // Rename Temporary File:
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
    size_t count = 0; // total count = line count from file + cache size
    count += this->lineCount(this->filename.c_str());
    count += this->cache.size();
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

/**
 * @brief Removes the given numbers of elements from the front of the cache.
 * @param num number of elements to remove
 * @return 
 */
bool DataFileClass::shrinkCache(size_t num) {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    bool success = true;
    for(size_t i = 0; i < num; i++) {
        if(this->cache.empty()) {
            log_e("Unexpected state: Cache already empty after shrinking element %u/%u", i, num);
            success = false;
            break;
        }
        this->cache.pop_front();
    }

    // Give Mutex Semaphore Back:
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }

    return success;
}

DataFileClass DataFile = DataFileClass();