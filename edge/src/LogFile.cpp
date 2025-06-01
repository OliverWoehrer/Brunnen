#include "LogFile.h"

#define MUTEX_TIMEOUT (2*1000)/portTICK_PERIOD_MS // 2000 ms

std::string stringToTag(std::string tagString) {
    if(tagString == "[INFO]") {
        return "info";
    } else if(tagString == "[WARNING]") {
        return "warning";
    } else if(tagString == "[ERROR]") {
        return "error";
    } else {
        return "debug";
    }
}

/**
 * @brief Constructor initalizes a log file at the file system and creates a semaphore to be ready
 * for multi process usage.
 * @param filename file name of the log file (e.g. "/log,txt")
 */
Log::Log(const std::string& filename) : file(SPIFFS, filename), led(LED_RED) {
    this->semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use log file semaphore");
    }
}

/**
 * @brief Mounts the internal file system SPIFFS and creates the log file (if it does not exist)
 * @return true on success, false otherwise
 */
bool Log::begin() {
    // Mount Internal Filesystem:
    if(!SPIFFS.begin(true)) {
        log_e("Unable to mount SPIFFS");
        return false;
    }

    // Initialize File:
    if(!this->file.check()) {
        log_w("Log file broken or not found");
        log_d("Resetting log file");
        if(!file.reset()) {
            log_e("Could not reset log file");
            return false;
        }
    }

    log_d("Reusing existing log file (file passed check)");
    return true;
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

    // Remove Control Characters:
    auto isControlCharacter = [](unsigned char c){ return std::isspace(c) && c != ' '; }; // is whitespace but not space character
    auto newEnd = std::remove_if(msg.begin(), msg.end(), isControlCharacter); // rearanges control chracters to the end of string
    msg.erase(newEnd, msg.end());

    // Build String:
    std::string timestamp = Time.toString();
    std::string buffer = timestamp+" ["+prefix+"] "+msg+"\r\n";
    
    // Print to Serial:
    Serial.printf(buffer.c_str());
    if(timestamp.size() == 0) { // invalid timestamp, only print to serial
        return false;
    }

    // Check Storage:
    if(SPIFFS.totalBytes() - SPIFFS.usedBytes() < 500) { // less then 500 bytes free
        log_e("Cannot write log file because onboard filesystem is (nearly) full");
        return false;
    }

    // Write to File:
    if(!this->file.append(buffer)) {
        log_w("Could not append log message");
        if(!this->file.check()) {
            log_w("The log file failed the check");
            if(!this->file.reset()) {
                log_e("Could not reset the log file as a fix");
                return false;
            }
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
bool Log::exportLogs(std::vector<log_message_t>& logs) {
    // Read Log File:
    std::vector<std::string> lines;
    lines.reserve(logs.capacity());
    if(!this->file.readLines(lines)) {
        log_e("Failed to read lines from file");
        return false;
    }
    
    /*std::vector<std::string> lines = {
        "17-07-2023T00:14:11 [DEBUG] Largest region currently free in heap at 87876 bytes.",
        "17-07-2023T00:14:11 [INFO] Requesting weather data from OpenMeteo API."
        "17-07-2023T00:14:14 [INFO] Rain today is 0 mm.",
        "17-07-2023T00:14:14 [INFO] Too little rain, resume pump operation.",
        "17-07-2023T00:14:14 [INFO] Sending Mail."
    };*/

    // Parse CSV Lines:
    for(std::string line : lines) {
        log_message_t l;
        if(parseLogLine(line.c_str(), l)) {
            logs.push_back(l);
        }
    }

    // Return Count of Actually Read Lines:
    log_d("Exported %d/%d lines", logs.size(), logs.capacity());
    return true;
}

/**
 * @brief Strips the first 'num' lines of this file. 
 * @param num line number of first line to keep 
 * @return true on success, false otherwise
 * @note This operation is expensive and should be called with care.
 */
bool Log::shrink(size_t num) {
    if(!this->file.shrink(num)) {
        log_e("Failed to shrink file");
        return false;
    }
    return true;
}

/**
 * @brief Clears the contents of the log file and clears the error led
 * @return true on success, false otherwise
 */
bool Log::clear() {
    if(!this->file.reset()) {
        log_e("Failed to reset file");
        return false;
    }
    this->led.off();
    return true;
}

/**
 * @brief Call this function if you acknowledged an error log message and want to clear the error
 * led (= turn it off), without deleting the actual log messages.
 */
void Log::acknowledge() {
    this->led.off();
}

/**
 * Tries to parse sensor data from the given line (CSV format) into the sensor data.
 * @param line string holding the CSV line in format TIME,FLOW,PRESSURE,LEVEL
 * @param data sensor data struct to be filled with parsed values
 * @return true on success, false if one of values failed to parse
 */
bool Log::parseLogLine(const char line[], log_message_t& log) {
    // Copy Into Local String Buffer:
    size_t len = strlen(line);
    char buffer[len+1];
    strncpy(buffer, line, len+1);

    // Parse Time String:
    char* token = strtok(buffer," ");
    if(token == NULL) {
        return false;
    }
    if(!TimeManager::fromDateTimeString(token, log.timestamp)) {
        return false;
    }

    // Parse Log Tag:
    token = strtok(NULL, " ");
    if(token == NULL) {
        return false;
    }
    log.tag = stringToTag(token);

    // Parse Message:
    token = strtok(NULL, "'");
    if(token == NULL) {
        return false;
    }
    log.message = std::string(token);

    // Return Success:
    return true;
}


Log LogFile = Log("/log.txt");
