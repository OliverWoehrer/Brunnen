#ifndef DATA_FILE_H
#define DATA_FILE_H

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

class DataFileClass : FileManager {
public:
    DataFileClass();
    bool begin();
    bool init(std::string filename);
    bool store(sensor_data_t data);
    bool exportData(std::vector<sensor_data_t>& data);
    bool shrinkData(size_t numLines);
    bool clear();
    bool remove();
    std::string getFilename();
private:
    std::string filename; // "/data_YYYY-MM-DD.txt"
    SemaphoreHandle_t semaphore;
    bool parseCSVLine(const char line[], sensor_data_t& data);
};

extern DataFileClass DataFile;

#endif /* DATA_FILE_H */