#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <vector>
#include "FS.h"

class FileManager {
public:
    FileManager(fs::FS& fs);
protected:
    fs::FS& fs;
    bool readLines(const char* path, std::vector<std::string>& lines);
    bool writeLine(const char* path, const std::string& line);
    bool appendLine(const char* path, const std::string& line);
    bool createFile(const char* path);
    bool clearFile(const char* path);
    bool deleteFile(const char* path);
    bool copyFile(const char* src, const char* dest, size_t startingLine);
    bool renameFile(const char* pathFrom, const char* pathTo);
    size_t fileSize(const char* path);
    size_t lineCount(const char* path);
    bool checkFile(const char* path);
private:
    SemaphoreHandle_t semaphore;
};

#endif /* FILE_MANAGER_H */