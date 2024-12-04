#ifndef LOG_FILE_H
#define LOG_FILE_H

#include "FileManager.h"
#include "SPIFFS.h"
#include "TimeManager.h"

#define MAX_LOG_LENGTH 100

typedef enum {INFO, WARNING, ERROR, DEBUG} log_mode_t;

class Log : FileManager {
public:
    Log(const char* filename);
    bool log(log_mode_t mode, const std::string& msg);
    bool exportLogs(std::vector<std::string>& logs);
    bool shrinkLogs(size_t numLines);
    bool clear(void);
    size_t size(void);
private:
    const char* filename;
};

#endif /* LOG_FILE_H */