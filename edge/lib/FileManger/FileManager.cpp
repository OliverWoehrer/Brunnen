#include "FileManager.h"

#define MUTEX_TIMEOUT (1000/portTICK_PERIOD_MS) // 1000 ms

FileManager::FileManager(fs::FS& fs) : fs(fs) {
    this->semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use file semaphore.");
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
    // Get Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    File file;
    bool success = false;
    do { // open scope for critical section

    // Open File:
    file = this->fs.open(path, FILE_READ);
    if(!file) {
        log_e("Could not open file %s", path);
        break;
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

    success = true;
    } while(0); // exit critical section

    // Clean Up:
    file.close();
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
}

/**
 * @brief Write the line to the file at the given path. Adds <CR><LF> at the end
 * @param path file path to write to
 * @param line line to write to this file
 * @return true on success, false otherwise
 */
bool FileManager::writeLine(const char* path, const std::string& line) {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
    }
    
    File file;
    bool success = false;
    do { // open scope for critical section

    // Open File:
    file = this->fs.open(path, FILE_WRITE);
    if(!file) {
        log_e("Could not open file %s", path);
        break;
    }
    size_t bytes = file.printf("%s\r\n", line.c_str());
    if(bytes == 0) {
        log_e("Could not write to file %s", path);
        break;
    }

    success = true;
    } while(0); // exit critical section

    // Clean Up:
    file.close();
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
}

/**
 * @brief Write the line to the end of the file at the given path 
 * @param path file path to write to
 * @param line line to write to file
 * @return true on success, false otherwise
 */
bool FileManager::appendLine(const char* path, const std::string& line) {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    File file;
    bool success = false;
    do { // open scope for critical section

    // Open File:
    file = this->fs.open(path, FILE_APPEND);
    if(!file) {
        log_e("Could not open file %s", path);
        break;
    }
    size_t bytes = file.printf("%s\r\n", line.c_str());
    if(bytes == 0) {
        log_e("Could not write to file %s", path);
        break;
    }

    success = true;
    } while(0); // exit critical section

    // Clean Up:
    file.close();
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
}

/**
 * @brief Create a new (empty) file at the given path
 * @param path file path to create (e.g. "/test.txt")
 * @return true on success, false otherwise
 */
bool FileManager::createFile(const char* path) {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    File file;
    bool success = false;
    do { // open scope for critical section

    // Open File:
    file = this->fs.open(path, FILE_WRITE);
    if(!file) {
        log_e("Could not open file %s", path);
        break;
    }

    success = true;
    } while(0); // exit critical section

    // Clean Up:
    file.close();
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
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
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    File srcFile;
    File destFile;
    bool success = true;
    do { // open scope for critical section

    // Open File:
    srcFile = this->fs.open(src, FILE_READ);
    if(!srcFile) {
        log_e("Could not open file %s", src);
        break;
    }

    // Set Curser to n-th Line (=lineNumber):
    size_t numBytes = 0;
    std::string line = "";
    int retries = 3; // maximum number of retreies
    while(srcFile.available() && startingLine > 0) {
        char byte = srcFile.read();
        if(byte == -1) {
            size_t position = numBytes + line.size(); // get curser poisiton (byte number it failed to read)
            log_w("Read on %s [byte %u] returned with error", src, position);
            if(retries == 0) { // no more retries
                log_e("Failed to read %s [byte %u] after multiple retries", src, position);
                success = false;
                break;
            }
            log_d("Resetting file curser on %s to %u before retry", src, numBytes);
            if(!srcFile.seek(numBytes)) {
                log_e("Failed to set file curser on %s to %u", src, numBytes);
                success = false;
                break;
            }
            retries--;
            continue; // skip rest of the loop and retry
        }
        line.append(1, byte);
        if(byte == '\n') {
            numBytes += line.size();
            line.clear();
            startingLine--;
        }
    }

    if(!success) {
        log_e("No success while setting the curser to the n-th line");
        break;
    }

    if(!srcFile.seek(numBytes)) {
        log_e("Failed to set file curser on %s to %u", src, numBytes);
        success = false;
        break;
    }

    // Open/Create Destination File:
    destFile = this->fs.open(dest, FILE_WRITE);
    if(!destFile) {
        log_e("Failed to open %s", dest);
        success = false;
        break;
    }

    // Copy Bytes:
    while(srcFile.available()) {
        uint8_t bytes[100] = ""; // copy in chunks of 100 bytes
        size_t num = srcFile.read(bytes, 100);
        if(num < 0) {
            log_e("Read on %s returned with error [%u bytes]", src, num);
            success = false;
            break;
        }
        
        size_t retries = 2;
        while(retries > 0) {
            size_t num2 = destFile.write(bytes, num);
            if(num != num2) { // check if all bytes from buffer were written
                log_w("Write on %s failed [%u/%u bytes]", dest, num2, num);
                retries--;
                log_d("There are %u retries left", retries);
                continue; // skip rest of the loop and retry
            }
            break;
        }
        if(retries == 0) { // use up all retries
            log_e("Failed to write %s after multiple retries", src);
            success = false;
            break;
        }
    }

    } while(0); // exit critical section


    // Clean Up:
    srcFile.close(); 
    destFile.close();
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
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
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    File file;
    size_t size;
    do { // open scope for critical section

    file = this->fs.open(path, FILE_READ);
    if(!file) {
        log_e("Could not open %s", path);
        size = 0;
        break;
    }
    size = file.size();
    
    } while(0); // exit critical section
    
    // Clean Up:
    file.close();
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return size;
}

/**
 * @brief Get how many lines are in the given file. Once there is an error on read, the line number
 * up until that point is returned
 * @param path file path to retrive the number from
 * @return number of lines
 */
size_t FileManager::lineCount(const char* path) {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    File file;
    size_t count = 0;
    do { // open scope for critical section

    // Open File:
    file = this->fs.open(path, FILE_READ);
    if(!file) {
        log_e("Could not open %s", path);
        break;
    }

    // Read Bytes:
    while(file.available()) {
        int byte = file.read();
        if(byte == -1) {
            log_e("Read on %s returned with error", path);
            break;
        }
        if(byte == '\n') {
            count++;
        }
    }
    
    } while(0); // exit critical section
    
    // Clean Up:
    file.close();
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return count;
}

/**
 * Checks if the file exisits.
 * @param path file path to check
 * @return true on success, false otherwise
 */
bool FileManager::checkFile(const char* path) {
    return this->fs.exists(path);
}
