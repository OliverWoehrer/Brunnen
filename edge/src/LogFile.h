#ifndef LOG_FILE_H
#define LOG_FILE_H

#include "FileManager.h"
#include "SPIFFS.h"
#include "Output.h"
#include "TimeManager.h"

// Pin Definitions:
#define LED_RED 4

#define MAX_LOG_LENGTH 100

typedef enum {INFO, WARNING, ERROR, DEBUG} log_mode_t;

class Log : FileManager {
public:
    Log(const char* filename);
    bool log(log_mode_t mode, std::string&& msg);
    bool exportLogs(std::vector<std::string>& logs);
    bool shrinkLogs(size_t numLines);
    bool clear(void);
    size_t size(void);
    void acknowledge();
private:
    Output::Digital led;
    std::string filename;
    SemaphoreHandle_t semaphore;
};

extern Log LogFile;

#endif /* LOG_FILE_H */