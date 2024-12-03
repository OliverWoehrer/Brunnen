#include "FileManager.h"

FileManager::FileManager(fs::FS& fs) : fs(fs) {}

/**
 * Reads lines from the file at the given path into the buffer without whitespaces. Only entire
 * lines are read (ending with a LF character).
 * @param path name of the file to read (e.g "/log.txt")
 * @param lines buffer to read into
 * @return number of lines read into the buffer, -1 on failure, 0 if the buffer is to small or no
 * LF character was found
 */
bool FileManager::readLines(const char* path, std::vector<std::string>& lines) {
    // Open File:
    File file = this->fs.open(path);
    if(!file) {
        return false;
    }

    // Read Bytes:
    size_t idx = 0;
    std::string line = "";
    while(file.available() && idx < lines.size()) {
        char byte = file.read();
        if(byte == -1) { return false; }
        if(byte < 0x09 || 0x0D < byte) { // check if whitespace (ASCII code between '\t'=0x09 and '\r'=0x0D)
            line.append(1, byte); // only use if not a whitespace
        }
        if(byte == '\n') {
            lines[idx] = line;
            line.clear();
            idx++;
        }
    }

    // Close File:
    file.close();
    return true;
}

bool FileManager::writeLine(const char* path, const std::string& line) {
    File file = this->fs.open(path);
    if(!file) {
        return false;
    }
    size_t bytes = file.printf("%s\r\n", line);
    if(bytes == 0) {
        return false;
    }
    file.close();
    return true;
}

bool FileManager::appendLine(const char* path, const std::string& line) {
    File file = this->fs.open(path, FILE_APPEND);
    if(!file) {
        return false;
    }
    size_t bytes = file.printf("%s\r\n", line);
    if(bytes == 0) {
        return false;
    }
    file.close();
    return true;
}

bool FileManager::createFile(const char* path) {
    File file = this->fs.open(path, FILE_WRITE);
    if(!file) {
        return false;
    }
    file.close();
    return true;
}

bool FileManager::clearFile(const char* path) {
    return createFile(path);
}

bool FileManager::deleteFile(const char* path) {
    return this->fs.remove(path);
}

bool FileManager::copyFile(const char* src, const char* dest, size_t startingLine) {
    // Open Source File:
    File srcFile = this->fs.open(src, FILE_READ);
    if(!srcFile) {
        return false;
    }

    // Set Curser to n-th Line (=lineNumber):
    size_t numBytes = 0;
    std::string line = "";
    while(srcFile.available() && startingLine > 0) {
        char byte = srcFile.read();
        if(byte == -1) {
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
        return false;
    }

    // Create Destination File:
    if(!createFile(dest)) {
        return false;
    }

    // Open Destination File:
    File destFile = this->fs.open(dest, FILE_WRITE);
    if(!destFile) {
        return false;
    }

    // Copy Bytes:
    while(srcFile.available()) {
        uint8_t bytes[100] = ""; // copy in chunks of 100 bytes
        size_t num = srcFile.read(bytes, 100);
        if(num < 0) {
            srcFile.close();
            destFile.close();
            return false;
        }
        num = destFile.write(bytes, num);
        if(num == 0) {
            srcFile.close();
            destFile.close();
            return false;
        }
    }

    // Close Files:
    srcFile.close(); 
    destFile.close();
    return true;
}

bool FileManager::renameFile(const char* pathFrom, const char* pathTo) {
    return this->fs.rename(pathFrom, pathTo);
}

size_t FileManager::fileSize(const char* path) {
    File file = this->fs.open(path, FILE_READ);
    if(!file) {
        return 0;
    }
    size_t size = file.size();
    file.close();
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
