#include "FileManager.h"

#define MUTEX_TIMEOUT (1000/portTICK_PERIOD_MS) // 1000 ms

FileManager::FileManager(fs::FS& fs) : fs(fs) {
    this->semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use data file semaphore.");
    }
}

/**
 * @brief Reads lines from the file at the given path into the buffer without whitespaces. Only entire
 * lines are read (ending with a LF character).
 * @param path name of the file to read (e.g "/log.txt")
 * @param lines buffer to be filled. Needs to be allocated with reserve(), so 'lines.capacity()' works
 * @return number of lines read into the buffer, -1 on failure, 0 if the buffer is to small or no
 * LF character was found
 */
bool FileManager::readLines(const char* path, std::vector<std::string>& lines) {
    // Open File:
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    File file = this->fs.open(path);
    if(!file) {
        log_e("Could not open file %s", path);
        return false;
    }

    // Read Bytes:
    std::string line = "";
    while(file.available() && lines.size() < lines.capacity()) {
        int byte = file.read();
        if(byte == -1) {
            log_e("Read on %s returned with error", path);
            break;
        }
        if(byte < 0x09 || 0x0D < byte) { // check if whitespace (ASCII code between '\t'=0x09 and '\r'=0x0D)
            line.append(1, byte); // only use if not a whitespace
        }
        if(byte == '\n') {
            lines.push_back(line);
            line.clear();
        }
    }

    // Close File:
    file.close();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return true;
}

/**
 * @brief Write the line to the file at the given path. Adds <CR><LF> at the end
 * @param path file path to write to
 * @param line line to write to this file
 * @return true on success, false otherwise
 */
bool FileManager::writeLine(const char* path, const std::string& line) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    File file = this->fs.open(path);
    if(!file) {
        log_e("Could not open file %s", path);
        return false;
    }
    size_t bytes = file.printf("%s\r\n", line.c_str());
    if(bytes == 0) {
        log_e("Could not write to file %s", path);
        return false;
    }
    file.close();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return true;
}

/**
 * @brief Write the line to the end of the file at the given path 
 * @param path file path to write to
 * @param line line to write to file
 * @return true on success, false otherwise
 */
bool FileManager::appendLine(const char* path, const std::string& line) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    File file = this->fs.open(path, FILE_APPEND);
    if(!file) {
        log_e("Could not open file %s", path);
        return false;
    }
    size_t bytes = file.printf("%s\r\n", line.c_str());
    if(bytes == 0) {
        log_e("Could not write to file %s", path);
        return false;
    }
    file.close();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return true;
}

/**
 * @brief Create a new (empty) file at the given path
 * @param path file path to create (e.g. "/test.txt")
 * @return true on success, false otherwise
 */
bool FileManager::createFile(const char* path) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    File file = this->fs.open(path, FILE_WRITE);
    if(!file) {
        log_e("Could not open file %s", path);
        return false;
    }
    file.close();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return true;
}

/**
 * @brief Deletes any content from the file at the given path
 * @param path file path to clear at
 * @return true on success, false otherwise
 */
bool FileManager::clearFile(const char* path) {
    return createFile(path);
}

/**
 * @brief Removes the file at the given file path
 * @param path file path to remove
 * @return true on success, false otherwise
 */
bool FileManager::deleteFile(const char* path) {
    return this->fs.remove(path);
}

/**
 * @brief Copy the content from the source file to the destination file, starting at the given file
 * number. Destination file is created if it does not exist.
 * @param src file path of source file
 * @param dest file path of destination file
 * @param startingLine line number of the first line to copy, i.e. "0" means to copy entire file
 * @return true on success, false otherwise
 */
bool FileManager::copyFile(const char* src, const char* dest, size_t startingLine) {    
    // Open Source File:
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    File srcFile = this->fs.open(src, FILE_READ);
    if(!srcFile) {
        log_e("Could not open file %s", src);
        return false;
    }

    // Set Curser to n-th Line (=lineNumber):
    size_t numBytes = 0;
    std::string line = "";
    while(srcFile.available() && startingLine > 0) {
        char byte = srcFile.read();
        if(byte == -1) {
            log_e("Read on %s returned with error", src);
            srcFile.close();
            return false;
        }
        line.append(1, byte);
        if(byte == '\n') {
            numBytes += line.size();
            line.clear();
        }
    }
    if(!srcFile.seek(numBytes)) {
        log_e("Failed to set file curser on %s to %u", src, numBytes);
        return false;
    }

    // Create Destination File:
    if(!createFile(dest)) {
        log_e("Failed to create %s", dest);
        return false;
    }

    // Open Destination File:
    File destFile = this->fs.open(dest, FILE_WRITE);
    if(!destFile) {
        log_e("Failed to open %s", dest);
        return false;
    }

    // Copy Bytes:
    while(srcFile.available()) {
        uint8_t bytes[100] = ""; // copy in chunks of 100 bytes
        size_t num = srcFile.read(bytes, 100);
        if(num < 0) {
            log_e("Read on %s returned with error", src);
            srcFile.close();
            destFile.close();
            return false;
        }
        num = destFile.write(bytes, num);
        if(num == 0) {
            log_e("Write on %s returned with error", dest);
            srcFile.close();
            destFile.close();
            return false;
        }
    }

    // Close Files:
    srcFile.close(); 
    destFile.close();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return true;
}

/**
 * @brief Rename the file at the given path
 * @param pathFrom original file path
 * @param pathTo new file path
 * @return true on success, false otherwise
 */
bool FileManager::renameFile(const char* pathFrom, const char* pathTo) {
    return this->fs.rename(pathFrom, pathTo);
}

/**
 * @brief Get the file size at the given file path
 * @param path file path to retrive the size from
 * @return number of bytes
 */
size_t FileManager::fileSize(const char* path) {
    xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT); // blocking wait
    File file = this->fs.open(path, FILE_READ);
    if(!file) {
        log_e("Could not open %s", path);
        return 0;
    }
    size_t size = file.size();
    file.close();
    xSemaphoreGive(this->semaphore); // give back mutex semaphore
    return size;
}

/**
 * Checks if the file exisits.
 * @param path file path to check
 * @return true on success, false otherwise
 */
bool FileManager::checkFile(const char* path) {
    return this->fs.exists(path);
}
