#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <vector>
#include "FS.h"

class FileManager {
public:
    FileManager(fs::FS& fs, const std::string& path);
    bool write(const std::string& buffer);
    bool append(const std::string& buffer);
    bool readLines(std::vector<std::string>& lines);
    bool shrink(size_t num);
    bool check();
    bool reset();
    bool remove();
    size_t size();
    size_t lineCount();
private:
    fs::FS& fs; // file system
    std::string fn; // file name "/data_YYYY-MM-DD.txt"
    SemaphoreHandle_t semaphore;
    inline const char* getPath();
    bool put(const std::string& buffer, const char* mode);
    bool temp(size_t startingLine);
};

#endif /* FILE_MANAGER_H */