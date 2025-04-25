#ifndef DATA_FILE_H
#define DATA_FILE_H

#include <deque>
#include "FileManager.h"
#include "SD.h"
#include "Sensors.h"
#include "TimeManager.h"

// SD Card:
#define SD_CARD
#define SPI_CD 5
#define SPI_MOSI 23
#define SPI_CLK 18
#define SPI_MISO 19

// String Lenghts:
#define FILE_NAME_LENGTH 25
#define DATA_STRING_LENGTH 40

class DataFileClass {
public:
    DataFileClass(const std::string& filename);
    bool begin();
    bool store(sensor_data_t data);
    bool exportData(std::vector<sensor_data_t>& data);
    bool shrink(size_t num);
    bool clear();
    size_t itemCount();
private:
    FileManager file;
    std::deque<sensor_data_t> cache;
    SemaphoreHandle_t semaphore;
    bool parseCSVLine(const char line[], sensor_data_t& data);
    bool shrinkCache(size_t num);
};

extern DataFileClass DataFile;

#endif /* DATA_FILE_H */