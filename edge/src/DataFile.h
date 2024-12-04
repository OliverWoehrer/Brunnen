#ifndef DATA_FILE_H
#define DATA_FILE_H

#include "FileManager.h"
#include "SD.h"
#include "Sensors.h"
#include "TimeManager.h"
// #include "time.h"

// SD Card:
#define SD_CARD
#define SPI_CD 5
#define SPI_MOSI 23
#define SPI_CLK 18
#define SPI_MISO 19

// Time String:
#define DATA_STRING_LENGTH 40

class DataFileClass : FileManager {
public:
    DataFileClass();
    bool init(const char* filename);
    bool store(sensor_data_t data);
    int exportData(std::vector<sensor_data_t>& data);
    bool shrinkData(size_t numLines);
    bool clear();
    char* getFilename();
private:
    char* filename;
    
    bool parseCSVLine(const char* line, sensor_data_t& data);
};

extern DataFileClass DataFile;

#endif /* DATA_FILE_H */