/**
 * @author Oliver Woehrer
 * @date 17.08.2022
 * @file dt.cpp
 * This modul [Data and Time] provides functions to handle the wifi connection. It initialies and
 * connections to the network given via the network credentials macros in dt.h. This allows the 
 * local time to be initialized via a NTP server over the web. Furthermore the file system of the
 * sd card attached over the sd card shield can be used. It contains basic read- and write-functions
 * as well as directory management.
 */
#include <Preferences.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>
// #include <SPIFFS.h>
// #include "FS.h"
// #include "SD.h"
// #include "SPI.h"
#include "Arduino.h"
#include "dt.h"


namespace DataTime {

//===============================================================================================
// WLAN
//===============================================================================================
namespace Wlan {
    /**
     * @brief Initalizes the Wlan module by setting the mode to STA (=station mode: the ESP32
     * connects to an access point). Connects afterwards to the WiFi network. And to prevent any
     * unexpected failures the device is disconnted (in case it was before).
     * @return SUCCESS (=0)
     */
    int init() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        return SUCCESS;
    }

    /**
     * @brief Does the actual linking to the WiFi by the provided credentials and
     * prints out the IP-adress on the network. If a failure occures during login the process is
     * repeated/retried 3 times. This function is called by connect().
     * @return SUCCESS if the login succeded and FAILURE if something went wrong
     */
    int login(const char* ssid, const char* pw) {
        unsigned char retries = 3; // number of tries to login
        while(retries > 0) {
            retries--;
            WiFi.begin(ssid, pw);
            unsigned long now = millis();
            while (WiFi.status() != WL_CONNECTED) { // wait for max. 100 sec for WiFi to connect
                Serial.printf(".");
                delay(500);
                if (millis() > now+100000) { // connection attempt timed out
                    Serial.printf("Unable to connect to WiFi\n");
                    break;
                }
            }

            if (WiFi.status() == WL_CONNECTED) { // print local IP address if connected
                // delay(500);
                Serial.printf("\nWifi connected at ");Serial.println(WiFi.localIP());
                return SUCCESS;
            }

            Serial.printf("Failed to connect WiFi\r\n");     
        }

        return FAILURE;
    }

    /**
     * @brief Scans the available wifi networks and connects to the predefined WiFi network(s) by
     * trying to log in.
     * @return SUCCESS if the login process was succesfull. FAILURE otherwise
     */
    int connect() {
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; ++i) {
            if (String(WiFi.SSID(i)).equals(WIFI_SSID_HOME)) {
                return login(WIFI_SSID_HOME, WIFI_PASSWORD_HOME);
            }
            if (String(WiFi.SSID(i)).equals(WIFI_SSID_FIELD)) {
                return login(WIFI_SSID_FIELD, WIFI_PASSWORD_FIELD);
            } // else: continue search
        }
        return FAILURE;
    } 

    /**
     * @brief Disconnects the Wifi connection if there is any connection alive
     * @return SUCCESS
     */
    int disconnect() {
        WiFi.disconnect(true);
        return SUCCESS;
    }

    /**
     * @brief Wrapper function to check the current status of the WiFi connection
     * @return true if the network is connected, false otherwise
     */
    bool isConnected() {
        return WiFi.status() == WL_CONNECTED;
    }
}

//===============================================================================================
// TIME
//===============================================================================================
namespace Time {
    char timeString[TIME_STRING_LENGTH]; // format[20]: "DD-MM-YYYY HH:MM:SS"

    /**
     * @brief Checks if the onboard(local) time system is valid and returns the time struct
     * @return time struct holding current time if valid or default time (00:00:00 00/00/0000) if not 
     */
    tm getTimeinfo() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) { // set time manually
            Serial.printf("Failed to obtain time\n");
            timeinfo.tm_sec = 0;
            timeinfo.tm_min = 0;
            timeinfo.tm_min = 0;
            timeinfo.tm_hour = 0;
            timeinfo.tm_mday = 0;
            timeinfo.tm_mon = 0;
            timeinfo.tm_year = 0;
            timeinfo.tm_wday = 0;
            timeinfo.tm_yday = 0;
            timeinfo.tm_isdst = 0;
        }
        return timeinfo;
    }

    /**
     * @brief Coneverts the current time into a string with format[20]: "DD-MM-YYYY HH:MM:SS"
     * @return char string of current time
     */
    char* toString() {
        struct tm timeinfo = getTimeinfo();
        strftime(timeString, TIME_STRING_LENGTH, "%d-%m-%Y %H:%M:%S", &timeinfo);
        return timeString;
    }

    /**
     * @brief Initalizes the onboard(local) time system by connecting to the defined NTP server
     * @return SUCCESS
     */
    int init() {
        configTime(GMT_TIME_ZONE, DAYLIGHT_OFFSET, NTP_SERVER);
        delay(700);
        Serial.printf("time initialized at %s\n", toString());
        return SUCCESS;
    }
}

//===============================================================================================
// LOG SYSTEM
//===============================================================================================
namespace Log {
    /**
     * @brief Mounts the internal file system and initializes the logfile (/log.txt) with a welcome
     * message. If there is already a log file in place the new content is appended at the end.
     * @return SUCCESS if the welcome message was written successfully
     */
    int init(const char* timestamp) {
        //Mount internal file system:
        if(!SPIFFS.begin(true)) {
            Serial.printf("Unable to mount SPIFFS.\n");
            return FAILURE;
        }

        //Open File to Write:
        File fileToWrite = SPIFFS.open("/log.txt", FILE_APPEND);
        if(!fileToWrite){
            Serial.printf("Unable to open log file for writing.\n");
            return FAILURE;
        }
        
        /* write first line to file:
        char logMsg[TIME_STRING_LENGTH+24] = "";
        sprintf(logMsg,"%s [INFO] System booted!\n",timestamp);
        if(!fileToWrite.printf(logMsg)) {
            Serial.printf("Failed to write to log file.\n");
            return FAILURE;
        }*/
        fileToWrite.close();
        return SUCCESS;
    }

    /**
     * @brief Concatenates a log message in following form: [TIMESTAMP] [HEADER] [msg] [LF] and prints
     * it to serial. If there is a timeString given the message is also written to log file ("/log.txt").
     * The message itself followed by a linefeed ("\n").
     * @param mode type of message (e.g INFO, ERROR, ...), matches the HEADER field
     * @param timeString is the given TIMESTAMP, can be NULL
     * @param msg actual message itself
     * @return SUCCESS
     */
    int msg(log_mode_t mode, const char* timeString, const char* msg) {
        char buffer[TIME_STRING_LENGTH+9+strlen(msg)]; // LENGTH+9 because prefix " [ERROR] "
        switch (mode) {
        case INFO:
            sprintf(buffer, "%s [INFO] %s\n",timeString,msg);
            break;
        case WARNING:
            sprintf(buffer, "%s  > %s\n",timeString,msg);
            break;
        case ERROR:
            sprintf(buffer, "%s [ERROR] %s\n",timeString,msg);
            break;
        case DEBUG:
            sprintf(buffer, "%s ## %s\n",timeString,msg);
            break;
        default:
            sprintf(buffer, "%s %s\n",timeString,msg);
            break;
        }
        Serial.printf("%s",buffer);

        if (timeString != NULL) { // write buffer to log file if timestamp is available
            if(!SPIFFS.begin(false)) { // mount internal file system
                Serial.printf("Unable to mount SPIFFS.\n");
                return FAILURE;
            }
            File logFile = SPIFFS.open("/log.txt", FILE_APPEND); //FILE_APPEND
            logFile.printf(buffer);
            logFile.flush();
            logFile.close();
        }
        
        return SUCCESS;
    }

    /**
     * @brief Reads the content of the log file (/log.txt) and returns it
     * @return content of the log file
     */
    const char* readFile() {
        if(!SPIFFS.begin(false)) { // mount internal file system
            Serial.printf("Unable to mount SPIFFS.\n");
            return "";
        }
        File file = SPIFFS.open("/log.txt", FILE_READ);
        if (!file) {
            Serial.print("Failed to open log file\n");
            return "";
        }
        String logString = "";
        while(file.available()){
            logString = logString + String(file.readString()) + "\r\n";
        }
        file.close();
        return logString.c_str();      
        // int fileSize = file.size();
        // char logString[fileSize];
        // file.readBytes(logString,fileSize);
        // file.close();
    }

    /*int readFile() {
        File file = SPIFFS.open("/log.txt", FILE_READ);
        if (!file) {
            Serial.print("Failed to open log file\n");
            return FAILURE;
        }
        Serial.print(" <<< LOG FILE /log.txt >>>\n");
        while(file.available()){
            Serial.write(file.read());
        }
        Serial.print(" >>> END OF LOG FILE <<<\n");
        file.close();
        return SUCCESS;
    }*/

    /**
     * @brief Looks for the log file (/log.txt) and returns it size
     * @return size of log file in bytes
     */
    int getFileSize() {
        if(!SPIFFS.begin(false)) { // mount internal file system
            Serial.printf("Unable to mount SPIFFS.\n");
            return 0;
        }
        File file = SPIFFS.open("/log.txt", FILE_READ);
        if (!file) {
            Serial.print("Failed to open log file\n");
            return 0;
        }
        int fileSize = file.size();
        file.close();
        return fileSize;
    }

    /**
     * @brief Clears all content of the log file (/log.txt) by creats a new log file instead
     * @return SUCCESS if the new (empty/cleared) file was created successfully, FAILURE otherwise
     */
    int clearFile() {
        int ret = SUCCESS;
        if(!SPIFFS.begin(false)) { // mount internal file system
            Serial.printf("Unable to mount SPIFFS.\n");
            return FAILURE;
        }
        File file = SPIFFS.open("/log.txt", FILE_WRITE);
        if (!file) {
            Serial.print("Failed to open log file\n");
            ret = FAILURE;
        }
        return ret;
    }
}

//===============================================================================================
// FILE SYSTEM
//===============================================================================================
namespace FileSystem {
    char fileNameBuffer[FILE_NAME_LENGTH]; // format[21]: "/data_YYYY-MM-DD.txt"

    /**
     * @brief Uses the serial interface to print the contents of the given directory
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param dirname name of the directory (e.g "/")
     * @param levels the number of hierachy levels to print
     */
    void listDirectory(fs::FS &fs, const char *dirname, uint8_t levels) {
        Serial.printf("Listing directory: %s\n", dirname);
        File root = fs.open(dirname);
        if (!root) {
            Serial.println("Failed to open directory");
            return;
        }
        if (!root.isDirectory()) {
            Serial.println("Not a directory");
            return;
        }
        File file = root.openNextFile();
        while (file) {
            if (file.isDirectory()) {
                Serial.print("  DIR : ");
                Serial.println(file.name());
                if (levels) listDirectory(fs, file.name(), levels - 1);
            } else {
                Serial.print("  FILE: ");
                Serial.print(file.name());
                Serial.print("  SIZE: ");
                Serial.println(file.size());
            }
            file = root.openNextFile();
        }
    }

    /**
     * @brief Creats a new directory (=sub-folder) in the given directory
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     */
    void createDir(fs::FS &fs, const char *path) {
        Serial.printf("Creating Dir: %s\n", path);
        if (fs.mkdir(path)) Serial.println("Dir created");
        else Serial.println("mkdir failed");
    }

    /**
     * @brief Removes the directory in the given oath
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     */
    void removeDir(fs::FS &fs, const char *path) {
        Serial.printf("Removing Dir: %s\n", path);
        if (fs.rmdir(path)) Serial.println("Dir removed");
        else Serial.println("rmdir failed");
    }

    /**
     * @brief Reads the content of the file at the given path
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     */
    void readFile(fs::FS &fs, const char *path) {
        Serial.printf("Reading file: %s\n", path);
        File file = fs.open(path);
        if (!file) {
            Serial.println("Failed to open file for reading");
            return;
        }
        Serial.print("Read from file: ");
        while (file.available()) Serial.write(file.read());
        file.close();
    }

    /**
     * @brief Writes the given message to the beginning of the file at the given path
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     * @param message text to be written into file
     */
    void writeFile(fs::FS &fs, const char *path, const char *message) {
        Serial.printf("Writing file: %s\n", path);
        File file = fs.open(path, FILE_WRITE);
        if (!file) {
            Serial.println("Failed to open file for writing");
            return;
        }
        if (file.print(message)) Serial.println("File written");
        else Serial.println("Write failed");
        file.close();
    }

    /**
     * @brief Writes the given message to the end of the file at the given path and 
     * therefor appending the given data to the existing file
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     * @param message text to be appended into file
     */
    void appendFile(fs::FS &fs, const char *path, const char *message) {
        File file = fs.open(path, FILE_APPEND);
        if (!file) {
            Serial.println("Failed to open file for appending");
            return;
        }
        if (!file.print(message)) Serial.println("Append failed");
        file.close();
    }

    /**
     * @brief Delets the file at the given path
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     */
    void deleteFile(fs::FS &fs, const char *path) {
        Serial.printf("Deleting file: %s\n", path);
        if (fs.remove(path)) Serial.println("File deleted");
        else Serial.println("Delete failed");
    }

    /**
     * @brief Mounts the SD card and looks for a file named "fileName". This is the file currently
     * used to store the data. If there is non found, a new file is created with the headers in
     * place. This way multiple initalizations/reboots of the system do not lead to multiple files.
     * Last the free storage space on the SD card is checked.
     * @return SUCCESS is the file was found or created, FAILURE otherwise
     */
    int init(const char* fName) {
        if (!SD.begin(SPI_CD)) {
            Serial.printf("Card Mount Failed");
            return FAILURE; // error code: no card shield found
        }

        //Create a file on the SD card if does not exist:
        File file = SD.open(fName);
        if(!file) {
            writeFile(SD, fName, "Timestamp,Flow,Pressure,Level\r\n");
        }
        file.close();

        //Check SD card size:
        unsigned long long usedBytes = SD.usedBytes() / (1024 * 1024);
        Serial.printf("Mounted SD card with %llu MB used.\n", usedBytes);
        strncpy(fileNameBuffer, fName, FILE_NAME_LENGTH-1); // set file name currently used
        return SUCCESS;

        /*strncpy(fileNameBuffer, fName, FILE_NAME_LENGTH-1); // set file name currently used
        return SUCCESS;*/
    }

    /**
     * @brief Returns the name of the file currently used.
     * @return file name in format[21]: "/data_YYYY-MM-DD.txt"
     */
    char* getFileName() {
        return fileNameBuffer;
    }
}

//===============================================================================================
// DATA & TIME
//===============================================================================================
/**
 * @brief Initializes the Wlan module and sets the system time (local time) by connecting to the
 * defined NTP server. Afterwards the wifi is disconnected again and the two file systems
 * (log file and data file) are mounted and initialized by setting up the files and their contents
 * @return SUCCESS is all modules are initalized correctly, FAILURE otherwise
 */
int init() {
    if(Wlan::init()) {
        Serial.printf("[ERROR] Failed to initialize Wlan module!\r\n");
        return FAILURE;
    }

    if(Wlan::connect()) { // connect wlan for time initalization
        Serial.printf("[ERROR] Failed to connect to Wlan module!\r\n");
        return FAILURE;
    }
    if(Time::init()) {
        Serial.printf("[ERROR] Failed to initialize Time module!\r\n");
        return FAILURE;
    }
    Wlan::disconnect();

    const char* timeString = Time::toString();
    if(Log::init(timeString)) {
        Serial.printf("[ERROR] Failed to initialize log system!\r\n");
        return FAILURE;
    }
    // const char* logString = Log::readFile();
    // Serial.print(" <<< LOG FILE /log.txt >>>\n");
    // Serial.print(logString);
    // Serial.print(" >>> END OF LOG FILE <<<\n");

    struct tm timeinfo = Time::getTimeinfo();
    char fileName[FILE_NAME_LENGTH]; // Format: "/data_YYYY-MM-DD.txt"
    sprintf(fileName, "/data_%04d-%02d-%02d.txt",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);
    if(FileSystem::init(fileName)) {
        Serial.printf("Failed to initialize file system (SD-Card).\n");
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * @brief Connects to one of the predefined wifi networks available.
 * @return SUCCESS if the login process was succesfull. FAILURE otherwise
 */
int connectWlan() {
    int connectSuccess = Wlan::connect();
    if (connectSuccess != SUCCESS) {
        Log::msg(Log::ERROR,Time::toString(),"Failed to connect WiFi after multple retries.");
        return FAILURE;
    }
    Log::msg(Log::INFO,Time::toString(),"Connected to WiFi.");
    return SUCCESS;
}

/**
 * @brief Disconnects the Wifi connection if there is any connection alive
 * @return SUCCESS
 */
int disconnectWlan() {
    Wlan::disconnect();
    Log::msg(Log::INFO,Time::toString(),"Disconnected from WiFi.");
    return SUCCESS;
}

/**
 * @brief: Checks if there is currently a wifi network connected. If not, a connection in the form
 * of a login process is started. Otherwise nothing happens.
 * @return SUCCESS if the wifi was still connected or is now, FAILURE otherwise
 */
int reconnectWlan() {
    bool isConnected = Wlan::isConnected();
    if (!isConnected) {
        int connectSuccess = Wlan::connect();
        if (connectSuccess != SUCCESS) {
            Log::msg(Log::ERROR,Time::toString(),"Failed to reconnect to WiFi after multple retries.");
            return FAILURE;
        }
        Log::msg(Log::INFO,Time::toString(),"reconnected to WiFi.");
    }
    return SUCCESS;
}

/**
 * @brief returns the status of the wifi connection
 * @return true, if a network is connected
 */
bool isWlanConnected() {
    return Wlan::isConnected();
}

/**
 * @brief Loads the current time (local time) from system and returns it
 * @return struct holding the current time
 */
tm loadTimeinfo() {
    return Time::getTimeinfo();
}

/**
 * @brief Converts the current time into a readable string format[20]: "DD-MM-YYYY HH:MM:SS"
 * @return timestamp in string format with max length TIME_STRING_LENGTH
 */
char* timeToString() {
    return Time::toString();
}

/**
 * @brief writes info message into log file.
 * @param msg the actual message
 * @return SUCCESS
 */
int logInfoMsg(const char* msg) {
    return Log::msg(Log::INFO,Time::toString(),msg);
}

/**
 * @brief writes warning message into log file.
 * @param msg the actual message
 * @return SUCCESS
 */
int logWarningMsg(const char* msg) {
    return Log::msg(Log::WARNING,Time::toString(),msg);
}

/**
 * @brief writes error message into log file.
 * @param msg the actual message
 * @return SUCCESS
 */
int logErrorMsg(const char* msg) {
    return Log::msg(Log::ERROR,Time::toString(),msg);
}

/**
 * @brief writes debug message into log file.
 * @param msg the actual message
 * @return SUCCESS
 */
int logDebugMsg(const char* msg) {
    return Log::msg(Log::DEBUG,Time::toString(),msg);
}

/**
 * @brief Reads the content of the log file and returns it
 * @return content of the log file
 */
const char* readLogFile() {
    return Log::readFile();
}

/**
 * @brief Checks the size of the log file. If its bigger than maxSize the log file gets cleared
 * so its empty again.
 * @param maxSize maximum size of lof file in bytes (e.g 1MB = 1000000)
 * @return SUCCESS if 
 */
int checkLogFile(int maxSize) {
    int fSize = Log::getFileSize();
    if(fSize > maxSize) { // bigger than 1MB
        int logSuccess = Log::clearFile();
        if(logSuccess != SUCCESS) {
            Serial.printf("Failed to clear log file!\r\n");
            return FAILURE;
        }
        Log::msg(Log::INFO,Time::toString(),"Log File cleared.");
    }
    return SUCCESS;
}

/**
 * @brief Looks for the log file (/log.txt) and returns its size
 * @return size of log file in bytes
 */
int getLogFileSize() {
    return Log::getFileSize();
}

/**
 * @brief sets the file name of the file currently used to store data to the current date.
 * @return SUCCESS if the file was created/opened correcly, FAILURE otherwise.
 */
int createCurrentDataFile() {
    struct tm timeinfo = Time::getTimeinfo();
    char fileName[FILE_NAME_LENGTH]; // Format: "/data_YYYY-MM-DD.txt"
    sprintf(fileName, "/data_%04d-%02d-%02d.txt",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);
    int ret = FileSystem::init(fileName);
    if(ret != SUCCESS) {
        Log::msg(Log::ERROR,Time::toString(),"Failed to initialize file system (SD-Card).");
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * @brief Loads the file name of the file currently used to store the sensor data
 * @return file name of file currently used
 */
char* loadActiveDataFileName() {
    return FileSystem::getFileName();
}

/**
 * @brief Deletes the file currently used to store the sensor data
 */
void deleteActiveDataFile() {
    FileSystem::deleteFile(SD, FileSystem::getFileName());
}

/**
 * @brief sets the file name of the file currently used to store data. Similar to createCurrentDataFile()
 * @param fName file name to set (name of the file to be used now)
 * @return SUCCESS if the file was created/opened correcly, FAILURE otherwise.
 */
int setActiveDataFile(const char* fName) {
    int fsSuccess = FileSystem::init(fName);
    if (fsSuccess != SUCCESS) {
        Log::msg(Log::ERROR,Time::toString(),"Failed to initalize new data file.");
        return FAILURE;
    }
    return SUCCESS; 
}

/**
 * @brief appends the given string into the (currently active) data file
 * @param msg data string to write to data file
 */
void writeToDataFile(const char* msg) {
    FileSystem::appendFile(SD, FileSystem::getFileName(), msg);
}

}
