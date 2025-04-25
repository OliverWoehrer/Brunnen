#include "DataFile.h"

#define MUTEX_TIMEOUT (1*1000)/portTICK_PERIOD_MS // in milliseconds
#define MAX_CACHE_SIZE 120

/**
 * Constructor initalizes the file system (external SD card)
 */
DataFileClass::DataFileClass(const std::string& filename) : file(SD, filename) {
    this->semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use data file semaphore");
    }
}

/**
 * @brief Mounts the extern file system (external SD card) and initializes the file on disk
 * @return true on success, false otherwise
 */
bool DataFileClass::begin() {
    // Mount SD Card:
    if(!SD.begin(SPI_CD)) {
        log_e("Failed to mount SD card");
        return false;
    }

    // Initialize File:
    if(!this->file.check()) {
        log_w("Data file broken or not found");
        log_d("Resetting data file");
        if(!file.reset()) {
            log_e("Could not reset data file");
            return false;
        }
    }

    log_d("Reusing existing data file (data file passed check)");
    return true;
}

/**
 * Takes the given sensor data and writes it as a new item to this data file.
 * @param data sensor data to be stored to file
 * @return true on success, false otherwise
 */
bool DataFileClass::store(sensor_data_t data) {
    // Check If Cache Is Full:
    if(this->cache.size() >= MAX_CACHE_SIZE) {
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

    // [at this point we need to reallocate data from cache to file storage]
    log_d("cache is (nearly) full [size = %u], copy data to file", this->cache.size()); 

    // Make Local Copy of Cache:
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

    // Stringify Cache Data:
    std::string buffer;
    buffer.reserve(cacheCopy.size() * DATA_STRING_LENGTH);
    for(const sensor_data_t& data : cacheCopy) {
        char line[DATA_STRING_LENGTH]; // format: "TIME,FLOW,PRESSURE,LEVEL<CR><NL>"
        size_t bytes = snprintf(line, sizeof(line), "%s,%d,%d,%d\r\n", TimeManager::toString(data.timestamp).c_str(), data.flow, data.pressure, data.level);
        if(bytes >= sizeof(line)) {
            log_w("Formatted line exceeded buffer size (increase DATA_STRING_LENGTH)");
            return false;
        }
        if(bytes == 0) {
            log_e("Failed to build line from sensor data");
            return false;
        }
        buffer.append(line, bytes);
    }

    // Copy Buffer to File:
    if(!this->file.append(buffer)) {
        log_e("Failed to write buffer to data file");
        return false;
    }

    // Clear Moved Data From Cache:
    if(!shrinkCache(cacheCopy.size())) { // shrink by the number of items just written
        log_e("Failed to shrink cache");
        return false;
    }

    log_d("cache shrunk [size = %u]", this->cache.size());
    return true;
}

/**
 * Extracts sensor data from this file into the given buffer. Oldest data is exported first. At
 * most 'data.capacity()' items are exported.
 * @param data buffer to be filled. Needs to be allocated with reserve(), so 'data.capacity()'
 * works
 * @return number of items that could succesfully be parsed, -1 on error
 */
bool DataFileClass::exportData(std::vector<sensor_data_t>& data) {
    size_t fSize = this->file.size();
    if(fSize) { // check if file is not empty
        log_d("Export from file (file size = %u bytes)", fSize);

        // Read From File:
        std::vector<std::string> lines;
        lines.reserve(data.capacity()); // initialize buffer
        if(!this->file.readLines(lines)) {
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
    } else {
        log_d("Export from cache (cache size = %u elements)",this->cache.size());

        // Copy From Cache to Buffer:
        if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
            log_e("Could not take semaphore");
            return false;
        }
        auto iter = this->cache.begin(); // get cache iterator
        auto end = std::next(iter, std::min(this->cache.size(), data.capacity())); // advance by maximum of 'capacity' steps
        for (; iter != end; ++iter) { // iterate at most the available size of data buffer
            sensor_data_t element = (*iter);
            data.push_back(element);
        }
        if(!xSemaphoreGive(this->semaphore)) { // give mutex semaphore back
            log_d("Failed to give semaphore");
            return false;
        }
    }

    // Return Count of Actually Read Lines:
    log_d("Exported %d/%d lines", data.size(), data.capacity());
    return true;
}

/**
 * Strips the first 'num' items of this file. The first item after shrinking, will be index
 * 'num'. If there is no data on the disk file, the cache is cleared instead, following the same
 * principels. Because 'exportData()' only exports items either from cache or disk file, this
 * method does only shrink either cache or disk file. It is intended to be used in combination
 * with 'exportData()'
 * @param num line number of the first line to keep 
 * @return true on success, false otherwise
 * @note Use this method after you successfully exported items with 'exportData()'
 */
bool DataFileClass::shrink(size_t num) {
    log_d("shrink by %u lines", num);

    if(this->file.size()) { // check if file is not empty
        if(!this->file.shrink(num)) {
            log_e("Failed to shrink data file");
            return false;
        }
    } // else: file already empty, shrink cache instead

    if(!shrinkCache(num)) {
        log_e("Failed to shrink cache");
        return false;
    }

    return true;
}

/**
 * Clears this file so it is empty afterwards
 * @return true on success, false otherwise
 */
bool DataFileClass::clear() {
    // Clear Disk File:
    if(!this->file.reset()) {
        log_e("Could not reset disk file");
        return false;
    }

    // Clear Cache:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    this->cache.clear();
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }

    return true;
}

/**
 * @brief Retreive the number of items in the data file
 * @return item counter (items in cache and disk file combined)
 */
size_t DataFileClass::itemCount() {
    size_t counter = 0; // total count

    // Item Count of Disk File:
    counter += this->file.lineCount();


    // Item Count of Cache:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }
    counter += this->cache.size();
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }

    return counter;
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

DataFileClass DataFile = DataFileClass("/data.txt");