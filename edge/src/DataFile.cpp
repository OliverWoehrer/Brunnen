#include "DataFile.h"

/**
 * Constructor initalizes the file system (external SD card)
 */
DataFileClass::DataFileClass() : FileManager(SD) {
    if(!SD.begin(SPI_CD)) {
        Serial.printf("Failed to mount SD card!\r\n");
    }
}

/**
 * Sets a new active data file. This essentially sets a (new) file path, so all future file
 * operations are executed on the given file.
 * @param filename 
 * @return 
 */
bool DataFileClass::init(const char* filename) {
    sprintf(this->filename, "%s", filename);
    if(!checkFile(filename)) { // check if file exisits
        writeLine(filename, "Timestamp,Flow,Pressure,Level\r\n"); // write header on new file
    }
}

/**
 * Takes the given sensor data and writes it as a new line in this data file. The data file itself
 * uses CSV format. 
 * @param data sensor data to be stored to file
 * @return true on success, false otherwise
 */
bool DataFileClass::store(sensor_data_t data) {
    char buffer[DATA_STRING_LENGTH]; // TIME,FLOW,PRESSURE,LEVEL<CR><LF>
    size_t bytes = sprintf(buffer, "%s,%d,%d,%d", TimeManager::toString(data.timestamp), data.flow, data.pressure, data.level);
    if(bytes == 0) {
        return false;
    }
    return writeLine(this->filename, buffer);
}

/**
 * Extracts sensor data from the beginning of the data file and writes it into the given data
 * buffer. Depending on the size of 'data' it tries to read that many lines and parse them.
 * @param data buffer to be filled. Needs to have a pre-allocated size, so 'data.size()' works
 * @return number of lines, that were read from file and could succesfully be parsed
 */
int DataFileClass::exportData(std::vector<sensor_data_t>& data) {
    // Read Data File:
    std::vector<std::string> lines(data.size());
    if(!this->readLines(this->filename, lines)) {
        return -1;
    }
    /*std::vector<std::string> lines = {
        "Timestamp,Flow,Pressure,Level",
        "2024-03-31T14:32:19,0,920,2541",
        "2024-03-31T14:32:20,0,930,2539",
        "2024-03-31T14:32:21,0,926,2539",
        "2024-03-31T14:32:22,0,926,2575",
        "2024-03-31T14:32:23,0,923,2544",
        "2024-03-31T14:32:24,0,926,2549",
        "2024-03-31T14:32:25,0,929,2545"
    };*/

    // Parse CSV Lines:
    int lineCount = 0;
    for(std::string line : lines) {
        sensor_data_t d;
        if(parseCSVLine(line.data(), d)) {
            data[lineCount] = d;
            lineCount++;
        }
    }

    // Return Count of Actually Read Lines:
    return lineCount;
}

/**
 * Strips the first numLines-1 lines of the active data file. This is done by copying lines,
 * into a new file and deleting the old one. The first line after shrinking, will be line number
 * 'numLines'
 * @param numLines line number of the first line to keep 
 * @return true on success, false otherwise
 */
bool DataFileClass::shrinkData(size_t numLines) {
    if(this->createFile("/temp.txt")) {
        Serial.printf("Failed to create temporary copy file\r\n");
        return false;
    }
    if(this->copyFile(this->filename, "/temp.txt", numLines)) {
        Serial.printf("Failed to copy content\r\n");
        return false;
    }
    if(this->deleteFile(this->filename)) {
        Serial.printf("Failed to remove log file.\r\n");
        return false;
    }
    if(this->renameFile("/temp.txt", this->filename)) {
        Serial.printf("Failed to rename temporary file to log file.\r\n");
        return false;
    }
    return true;
}

/**
 * Clears the active data file so it is empty afterwards
 * @return true on success, false otherwise
 */
bool DataFileClass::clear() {
    return this->clearFile(this->filename);
}

/**
 * Return the filename of the active data file
 * @return string of the file path
 */
char* DataFileClass::getFilename() {
    return this->filename;
}

/**
 * Tries to parse sensor data from the given line (CSV format) into the sensor data.
 * @param line string holding the CSV line in format TIME,FLOW,PRESSURE,LEVEL
 * @param data sensor data struct to be filled with parsed values
 * @return true on success, false if one of values failed to parse
 */
bool DataFileClass::parseCSVLine(const char* line, sensor_data_t& data) {
    // Copy Into Local String Buffer:
    char buffer[DATA_STRING_LENGTH];
    strncpy(buffer, line, DATA_STRING_LENGTH);

    // Parse Time String:
    char* token = strtok(buffer,",");
    if(token == NULL) {
        return false;
    }
    data.timestamp = TimeManager::fromString(token);

    // Parse Flow Value:
    token = strtok(NULL, ",");
    if(token == NULL) {
        return false;
    }
    data.flow = atoi(token);

    // Parse Pressure Value:
    token = strtok(NULL, ",");
    if(token == NULL) {
        return false;
    }
    data.pressure = atoi(token);

    // Parse Level Value:
    token = strtok(NULL, ",");
    if(token == NULL) {
        return false;
    }
    data.level = atoi(token);

    // Return Success:
    return true;
}

DataFileClass DataFile = DataFileClass();