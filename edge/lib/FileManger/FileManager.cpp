#include "FileManager.h"

#define MUTEX_TIMEOUT (1000/portTICK_PERIOD_MS) // 1000 ms

FileManager::FileManager(fs::FS& filesystem, const std::string& filename) : fs(filesystem), fn(filename) {
    this->semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use file semaphore.");
    }
}

/**
 * @brief Write the given buffer to the file.
 * @param buffer text to write to this file
 * @return true on success, false otherwise
 */
bool FileManager::write(const std::string& buffer) {
    return this->put(buffer, FILE_WRITE);
}

/**
 * @brief Write the given buffer to the end of the file 
 * @param buffer text to write to file
 * @return true on success, false otherwise
 */
bool FileManager::append(const std::string& buffer) {
    return this->put(buffer, FILE_APPEND);
}

/**
 * @brief Reads lines from the file into the buffer without whitespaces. Only entire lines are
 * read. A line is terminated with a LF character. Check 'lines.size()' afterwards to see how many
 * lines were actually read.
 * @param lines buffer to be filled. Needs to be allocated with reserve(), so 'lines.capacity()' works
 * @return true on success, false on failure
 */
bool FileManager::readLines(std::vector<std::string>& lines) {
    // Get Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    // Critical Section:
    bool success = false;
    do {
        // Open File:
        File file = this->fs.open(getPath(), FILE_READ);
        if(!file) {
            log_e("Could not open file %s", getPath());
            file.close();
            break;
        }

        // Read Bytes:
        std::string line = "";
        while(file.available() && lines.size() < lines.capacity()) {
            int byte = file.read();
            if(byte == -1) {
                log_e("Read on %s returned with error", getPath());
                file.close();
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

        // Result:
        success = true;
        file.close();
    } while(0);


    // Clean Up:
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
}

/**
 * @brief Strips the first 'num' lines of the file. This is done by copying lines, into
 * a new file and deleting the old one.
 * @param num number of lines to strip 
 * @return true on success, false otherwise
 */
bool FileManager::shrink(size_t num) {
    // Copy Data to Temporary File:
    if(!this->temp(num)) {
        log_e("Failed to copy data to temporary file");
        return false;
    }

    // Get mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    // Critical Section:
    bool success = true;
    do {
        // Remove Data File:
        if(!this->remove()) {
            log_e("Failed to delete old file");
            success = false;
            break;
        }

        // Rename Temporary File:
        std::string tempFileName = this->fn + ".temp";
        if(!this->fs.rename(tempFileName.c_str(), this->fn.c_str())) {
            log_e("Failed to rename temporary file to data file");
            success = false;
            break;
        }
    } while(0);

    // Clean Up:
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
        return false;
    }

    return success;
}

/**
 * Checks if the file exisits and tries to open it
 * @return true on success, false otherwise
 */
bool FileManager::check() {
    // Check File Existance:
    if(!this->fs.exists(getPath())) {
        return false; // file does not exist
    }

    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    // Critical Section:
    bool success = false;
    do { // open scope for critical section
        // Try Open File:
        File file = this->fs.open(getPath(), FILE_READ);
        if(!file) {
            log_e("Could not open existing file %s", getPath());
            break;
        }
        
        // Result:
        success = true;
        file.close(); // clean up after critical section
    } while(0); // exit critical section

    // Clean Up:
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
}

/**
 * @brief Create a new (empty) file. Deletes any content if the file already exists
 * @return true on success, false otherwise
 */
bool FileManager::reset() {
    // Delete Old File:
    this->remove();
    
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    // Critical Section:
    bool success = false;
    do {
        // Open File:
        File file = this->fs.open(getPath(), FILE_WRITE, true);
        if(!file) {
            log_e("Could not open file %s", getPath());
            break;
        }

        // Result:
        success = true;
        file.close();
    } while(0); // exit critical section

    // Clean Up:
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
}

/**
 * @brief Removes the file from storage
 * @return true on success, false otherwise
 */
bool FileManager::remove() {
    return this->fs.remove(getPath());
}

/**
 * @brief Get the file size
 * @return number of bytes
 */
size_t FileManager::size() {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return 0;
    }

    // Critical Section:
    size_t size = 0;
    do { // open scope for critical section
        // Open File:
        File file = this->fs.open(getPath(), FILE_READ);
        if(!file) {
            log_e("Could not open %s", getPath());
            break;
        }

        // Result:
        size = file.size();
        file.close();
    } while(0);

    // Clean Up:
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return size;
}

/**
 * @brief Count how many lines are in the file. Once there is an error on read, the line number
 * up until that point is returned
 * @return number of lines (zero in case of error)
 */
size_t FileManager::lineCount() {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return 0;
    }

    // Critical Section:
    size_t count = 0;
    do {
        // Open File:
        File file = this->fs.open(getPath(), FILE_READ);
        if(!file) {
            log_e("Could not open %s", getPath());
            break;
        }

        // Read Bytes:
        while(file.available()) {
            int byte = file.read();
            if(byte == -1) {
                log_e("Read on %s returned with error", getPath());
                file.close(); // clean up
                break;
            }
            if(byte == '\n') {
                count++;
            }
        }

        // Close:
        file.close();
    } while(0); // exit critical section
    
    // Clean Up:
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return count;
}


inline const char* FileManager::getPath() {
    return this->fn.c_str();
}

/**
 * @brief Write the given buffer to the file with the given mode
 * @param buffer text to write to file
 * @param mode write mode (write or append)
 * @return true on success, false otherwise
 */
bool FileManager::put(const std::string& buffer, const char* mode) {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
    }

    // Critical Section:
    bool success = false;
    do {
        // Open File:
        File file = this->fs.open(getPath(), mode);
        if(!file) {
            log_e("Could not open file %s", getPath());
            file.close(); // clean up
            break;
        }

        // Write Bytes:
        size_t bytes = file.printf(buffer.c_str());
        if(bytes == 0) {
            log_e("Could not write to file %s", getPath());
            file.close(); // clean up
            break;
        }

        // Result:
        success = true;
    } while(0);

    // Clean Up:
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
}

/**
 * @brief Copy lines from the file to a temporary file (this->fn + ".temp"), starting at the given line number.
 * @param src file path of source file
 * @param dest file path of destination file
 * @param startingLine line number of the first line to copy, i.e. "0" means to copy entire file
 * @return true on success, false otherwise
 */
bool FileManager::temp(size_t startingLine) {
    // Take Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        log_e("Could not take semaphore");
        return false;
    }

    // Critical Section:
    bool success = true;
    do {
        // Open Source File:
        File srcFile = this->fs.open(getPath(), FILE_READ);
        if(!srcFile) {
            log_e("Could not open file %s", getPath());
            break;
        }

        // Set Curser to n-th Line (=num):
        size_t numBytes = 0;
        std::string line = "";
        while(srcFile.available() && startingLine > 0) {
            char byte = srcFile.read();
            if(byte == -1) {
                size_t position = numBytes + line.size(); // get curser position (byte it failed to read)
                log_w("Read on %s [byte %u] returned with error", getPath(), position);
                success = false;
                break;
            }
            line.append(1, byte);
            if(byte == '\n') {
                numBytes += line.size();
                line.clear();
                startingLine--;
            }
        }
        if(!success) {
            log_e("Error while iterating until the n-th line");
            srcFile.close();
            break;
        }
        if(!srcFile.seek(numBytes)) {
            log_e("Failed to set file curser on %s to %u", getPath(), numBytes);
            srcFile.close();
            success = false;
            break;
        }

        // Create Temporary File:
        std::string tempFileName = this->fn + ".temp";
        log_d("Creating temporary file '%s'", tempFileName.c_str());
        File tempFile =  this->fs.open(tempFileName.c_str(), FILE_WRITE);
        if(!tempFile) {
            log_e("Failed to create temporary copy file");
            srcFile.close(); // clean up
            success = false;
            break;
        }

        // Copy Bytes:
        while(srcFile.available()) {
            // Read Data Chunk:
            uint8_t bytes[100] = ""; // copy in chunks of 100 bytes
            size_t num = srcFile.read(bytes, 100);
            if(num < 0) {
                log_e("Read on %s returned with error [%u bytes]", getPath(), num);
                success = false;
                break;
            }

            // Write Data Chunk:
            size_t retries = 2;
            while(retries > 0) {
                size_t num2 = tempFile.write(bytes, num);
                if(num != num2) { // check if all bytes from buffer were written
                    log_w("Write on temporary file failed [%u/%u bytes]", tempFile.name(), num2, num);
                    retries--;
                    log_d("There are %u retries left", retries);
                    continue; // skip rest of the loop and retry
                }
                break;
            }
            if(retries == 0) { // used up all retries
                log_e("Failed to write temporary file after multiple retries");
                success = false;
                break;
            }
        }

        // Close Files:
        if(srcFile) {
            srcFile.close();
        }
        if(tempFile) {
            tempFile.close();
        }
    } while(0); // exit critical section


    // Clean Up:
    if(!xSemaphoreGive(this->semaphore)) { // give back mutex semaphore
        log_d("Failed to give semaphore");
    }
    return success;
}
