#include "LogFile.h"

#define MUTEX_TIMEOUT (2*1000)/portTICK_PERIOD_MS // 2000 ms

/**
 * @brief Constructor initalizes a log file at the file system and creates a semaphore to be ready
 * for multi process usage.
 * @param filename file name of the log file (e.g. "/log,txt")
 */
Log::Log(const char* filename) : FileManager(SPIFFS), led(LED_RED) {
    if(!SPIFFS.begin()) { // mount internal file system
        log_e("Unable to mount SPIFFS");
    }
    this->semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use data file semaphore");
    }
    this->filename = filename;
}

/**
 * @brief Write the given message with the current timestamp and a prefix according to the log mode
 * to the log file
 * @param mode mode of log (e.g. INFO, ERROR, etc.)
 * @param msg message without line ending
 * @return true on success, false otherwise
 */
bool Log::log(log_mode_t mode, std::string&& msg) {
    std::string prefix;
    switch (mode) {
    case INFO:
        prefix = "INFO";
        break;
    case WARNING:
        prefix = "WARNING";
        break;
    case ERROR:
        prefix = "ERROR";
        this->led.on();
        break;
    case DEBUG:
        prefix = "DEBUG";
        break;
    default:
        prefix = "";
        break;
    }

    Serial.printf("[%s] %s\r\n", prefix.c_str(), msg.c_str());
    std::string timestamp = Time.toString();
    if(timestamp.size()) { // write buffer to log file if timestamp is available
        xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
        File file = this->fs.open(this->filename.c_str(), FILE_APPEND);
        xSemaphoreGive(this->semaphore); // give back mutex semaphore
        if(!file) {
            log_e("Failed to open log file");
            return false;
        }
        size_t size = file.printf("%s [%s] %s\r\n", timestamp.c_str(), prefix.c_str(), msg.c_str());
        file.flush();
        file.close();
        if(size == 0) {
            log_e("Failed to printf log file");
        }
    }
    return true;
}

/**
 * @brief Reads lines from log file into the given vector. The lines are read withput line endings.
 * At most N lines are read, where N is the size of the vector
 * @param logs buffer to be filled. Needs to be allocated with reserve(), so 'logs.capacity()' works
 * @return true on success, false otherwise
 */
bool Log::exportLogs(std::vector<std::string>& logs) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    bool success = this->readLines(this->filename.c_str(), logs);
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return success;
}

/**
 * @brief Shrinks the log file by removing the first num-1 lines from the file. This is done by
 * copying bytes, starting at the given line number, to a new file and deleting the old one. This
 * operation is expensive and should be called with care. 
 * @param num line number of first line to keep 
 * @return true on success, false otherwise
 */
bool Log::shrinkLogs(size_t num) {
    bool success;
    if(this->createFile("/temp.txt")) {
        log_e("Failed to create temporary copy file");
        return false;
    }
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    success = this->copyFile(this->filename.c_str(), "/temp.txt", num);
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    if(!success) {
        log_e("Failed to copy content");
        return false;
    }
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    success = this->deleteFile(this->filename.c_str());
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    if(!success) {
        log_e("Failed to remove log file");
        return false;
    }
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    success = this->renameFile("/temp.txt", this->filename.c_str());
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    if(!success) {
        log_e("Failed to rename temporary file to log file");
        return false;
    }
    return true;
}

/**
 * @brief Deletes the contents of the log file and clears the error led
 * @return true on success, false otherwise
 */
bool Log::clear() {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    bool success = this->clearFile(this->filename.c_str());
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    this->led.off();
    return success;
}

/**
 * @brief Retreive the size of the log file
 * @return size in bytes
 */
size_t Log::size() {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    bool success = this->fileSize(this->filename.c_str());
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return success;
}

/**
 * @brief Call this function if you acknowledged an error log message and want to clear the error
 * led (= turn it off), without deleting the actual log messages.
 */
void Log::acknowledge() {
    this->led.off();
}

Log LogFile = Log("/log.txt");
