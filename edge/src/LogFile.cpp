#include "LogFile.h"

Log::Log(const char* filename) : FileManager(SPIFFS) {
    this->filename = filename;
    if(!SPIFFS.begin()) { // mount internal file system
        Serial.printf("Unable to mount SPIFFS.\r\n");
    }
}

bool Log::log(log_mode_t mode, const std::string& msg) {
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
        break;
    case DEBUG:
        prefix = "DEBUG";
        break;
    default:
        prefix = "";
        break;
    }
    Serial.printf("[%s] %s\r\n", prefix, msg);
    char* timestamp = Time.toString();
    if (timestamp != NULL) { // write buffer to log file if timestamp is available
        File file = this->fs.open(this->filename, FILE_APPEND);
        size_t size = file.printf("%s [%s] %s\r\n", timestamp, prefix, msg);
        file.flush();
        file.close();
    }
    return true;
}

bool Log::exportLogs(std::vector<std::string>& logs) {
    return this->readLines(this->filename, logs);
}

bool Log::shrinkLogs(size_t num) {
    if(this->createFile("/temp.txt")) {
        Serial.printf("Failed to create temporary copy file\r\n");
        return false;
    }
    if(this->copyFile(this->filename, "/temp.txt", num)) {
        Serial.printf("Failed to copy content\r\n");
        return false;
    }
    if(this->deleteFile(this->filename)) {
        Serial.printf("Failed to remove log file.\r\n");
        return false;
    }
    if(this->renameFile("/temp.txt", this->filename)) {
        Serial.printf("Failed to rename temporary file to log file.\r\n");
        return false;
    }
    return true;
}

bool Log::clear(void) {
    return this->clearFile(this->filename);
}

size_t Log::size(void) {
    return this->fileSize(this->filename);
}
