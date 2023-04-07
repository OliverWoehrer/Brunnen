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
     * Initalizes the Wlan module by setting the mode to STA (=station mode: the ESP32
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
     * Does the actual linking to the WiFi by the provided credentials and
     * prints out the IP-adress on the network. If a failure occures during login the process is
     * repeated/retried 3 times. This function is called by connect().
     * @return SUCCESS if the login succeded and FAILURE if something went wrong
     */
    int login(const char* ssid, const char* pw) {
        WiFi.begin(ssid, pw);
        unsigned long now = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            if (millis() > now+10000) { // wait for max. 100 sec for WiFi to connect
                Serial.printf("login attempt timed out\r\n");
                break;
            }
        }

        if (WiFi.status() == WL_CONNECTED) { // print local IP address if connected
            // delay(500);
            Serial.printf("Wifi connected at ");Serial.println(WiFi.localIP());
            return SUCCESS;
        }

        return FAILURE;
    }

    /**
     * Scans the available wifi networks and connects to the predefined WiFi network(s) by
     * trying to log in.
     * @return SUCCESS if the login process was succesfull. FAILURE otherwise
     */
    int connect() {
        unsigned char retries = 2; // number of tries to login
        while(retries > 0) {
            retries--;
                
            Serial.printf("trying to login at %s network\r\n",WIFI_SSID_HOME);
            if (login(WIFI_SSID_HOME, WIFI_PASSWORD_HOME) == SUCCESS) {
                return SUCCESS;
            }

            Serial.printf("trying to login at %s network\r\n",WIFI_SSID_FIELD);
            if (login(WIFI_SSID_FIELD, WIFI_PASSWORD_FIELD) == SUCCESS) {
                return SUCCESS;
            }

            Serial.printf("trying to login at %s network\r\n",WIFI_SSID_MOBILE);
            if (login(WIFI_SSID_MOBILE, WIFI_PASSWORD_MOBILE) == SUCCESS) {
                return SUCCESS;
            }
        }
        return FAILURE;
    } 

    /**
     * Disconnects the Wifi connection if there is any connection alive
     * @return SUCCESS
     */
    int disconnect() {
        WiFi.disconnect(true);
        return SUCCESS;
    }

    /**
     * Wrapper function to check the current status of the WiFi connection
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
     * Checks if the onboard(local) time system is valid and returns the time struct
     * @return time struct holding current time if valid or default time (00:00:00 00/00/0000) if not 
     */
    tm getTimeinfo() {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) { // set time manually
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
     * Coneverts the current time into a string with format[20]: "DD-MM-YYYY HH:MM:SS"
     * @return char string of current time
     */
    char* toString() {
        struct tm timeinfo = getTimeinfo();
        strftime(timeString, TIME_STRING_LENGTH, "%d-%m-%Y %H:%M:%S", &timeinfo);
        return timeString;
    }

    /**
     * Initalizes the onboard(local) time system by connecting to the defined NTP server
     * @return FAILURE if the time could get configured, SUCCESS otherwise
     */
    int init() {
        configTime(GMT_TIME_ZONE, DAYLIGHT_OFFSET, NTP_SERVER);
        delay(700);
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) { // check for local time
            Serial.printf("Failed to config time.\r\n");
            return FAILURE;
        }
        Serial.printf("time initialized at %s\r\n", toString());
        return SUCCESS;
    }
}

//===============================================================================================
// LOG SYSTEM
//===============================================================================================
namespace Log {
    /**
     * Mounts the internal file system and initializes the logfile (/log.txt) with a welcome
     * message. If there is already a log file in place the new content is appended at the end.
     * @return SUCCESS if the welcome message was written successfully
     */
    int init(const char* timestamp) {
        //Mount internal file system:
        if(!SPIFFS.begin(true)) {
            Serial.printf("Unable to mount SPIFFS.\r\n");
            return FAILURE;
        }

        //Open File to Write:
        File fileToWrite = SPIFFS.open("/log.txt", FILE_APPEND);
        if(!fileToWrite){
            Serial.printf("Unable to open log file for writing.\r\n");
            return FAILURE;
        }
        
        /* write first line to file:
        char logMsg[TIME_STRING_LENGTH+24] = "";
        sprintf(logMsg,"%s [INFO] System booted!\r\n",timestamp);
        if(!fileToWrite.printf(logMsg)) {
            Serial.printf("Failed to write to log file.\r\n");
            return FAILURE;
        }*/
        fileToWrite.close();
        return SUCCESS;
    }

    /**
     * Concatenates a log message in following form: [TIMESTAMP] [HEADER] [msg] [LF] and prints
     * it to serial. If there is a timeString given the message is also written to log file ("/log.txt").
     * The message itself followed by a linefeed ("\r\n").
     * @param mode type of message (e.g INFO, ERROR, ...), matches the HEADER field
     * @param timeString is the given TIMESTAMP, can be NULL
     * @param msg actual message itself
     * @return SUCCESS
     */
    int msg(log_mode_t mode, const char* timeString, const char* msg) {
        char buffer[TIME_STRING_LENGTH+9+strlen(msg)]; // LENGTH+9 because prefix " [ERROR] "
        switch (mode) {
        case INFO:
            sprintf(buffer, "%s [INFO] %s\r\n",timeString,msg);
            break;
        case WARNING:
            sprintf(buffer, "%s  > %s\r\n",timeString,msg);
            break;
        case ERROR:
            sprintf(buffer, "%s [ERROR] %s\r\n",timeString,msg);
            break;
        case DEBUG:
            sprintf(buffer, "%s ## %s\r\n",timeString,msg);
            break;
        default:
            sprintf(buffer, "%s %s\r\n",timeString,msg);
            break;
        }
        Serial.printf("%s",buffer);

        if (timeString != NULL) { // write buffer to log file if timestamp is available
            if(!SPIFFS.begin(false)) { // mount internal file system
                Serial.printf("Unable to mount SPIFFS.\r\n");
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
     * Reads the content of the log file (/log.txt) and returns it
     * @return content of the log file
     */
    const char* readFile() {
        if(!SPIFFS.begin(false)) { // mount internal file system
            Serial.printf("Unable to mount SPIFFS.\r\n");
            return "";
        }
        File file = SPIFFS.open("/log.txt", FILE_READ);
        if (!file) {
            Serial.print("Failed to open log file\r\n");
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
            Serial.print("Failed to open log file\r\n");
            return FAILURE;
        }
        Serial.print(" <<< LOG FILE /log.txt >>>\r\n");
        while(file.available()){
            Serial.write(file.read());
        }
        Serial.print(" >>> END OF LOG FILE <<<\r\n");
        file.close();
        return SUCCESS;
    }*/

    /**
     * Looks for the log file (/log.txt) and returns it size
     * @return size of log file in bytes
     */
    int getFileSize() {
        if(!SPIFFS.begin(false)) { // mount internal file system
            Serial.printf("Unable to mount SPIFFS.\r\n");
            return 0;
        }
        File file = SPIFFS.open("/log.txt", FILE_READ);
        if (!file) {
            Serial.print("Failed to open log file\r\n");
            return 0;
        }
        int fileSize = file.size();
        file.close();
        return fileSize;
    }

    /**
     * Clears all content of the log file (/log.txt) by creats a new log file instead
     * @return SUCCESS if the new (empty/cleared) file was created successfully, FAILURE otherwise
     */
    int clearFile() {
        int ret = SUCCESS;
        if(!SPIFFS.begin(false)) { // mount internal file system
            Serial.printf("Unable to mount SPIFFS.\r\n");
            return FAILURE;
        }
        File file = SPIFFS.open("/log.txt", FILE_WRITE);
        if (!file) {
            Serial.print("Failed to open log file\r\n");
            ret = FAILURE;
        }
        return ret;
    }
}

//===============================================================================================
// PREFERENCES
//===============================================================================================
namespace Pref {
    Preferences preferences;
    char fileNameBuffer[FILE_NAME_LENGTH]; // format[20]: "/data_YYYY-MM-DD.txt"
    char passwordBuffer[STRING_LENGTH];

    /**
     * Initalizes the preferences and mounts the flash memory
     */
    int init() {
        bool ret = preferences.begin("brunnen", false);
        preferences.end();
        return ret ? SUCCESS : FAILURE;
    }

    /**
     * Writes the the values for the starting time for intervall i into preferences
     * @param start time struct holding starting time
     * @param i index of intervall
     */
    void setStartTime(tm start, unsigned int i) {
        char startHrString[14] = "start_hour_XX";
        char startMinString[13] = "start_min_XX";
        sprintf(startHrString, "start_hour_%02d", i);
        sprintf(startMinString, "start_min_%02d", i);
        preferences.begin("brunnen", false);
        preferences.putUInt(startHrString, start.tm_hour);
        preferences.putUInt(startMinString, start.tm_min);
        preferences.end();
    }

    /**
     * Reads the the values for the starting time for intervall i from preferences
     * @param i index of intervall
     * @return time struct holding the start time
     */
    tm getStartTime(unsigned int i) {
        char startHrString[14] = "start_hour_XX";
        char startMinString[13] = "start_min_XX";
        sprintf(startHrString, "start_hour_%02d", i);
        sprintf(startMinString, "start_min_%02d", i);

        struct tm start;
        preferences.begin("brunnen", false);
        start.tm_hour = preferences.getUInt(startHrString);
        start.tm_min = preferences.getUInt(startMinString);
        start.tm_sec = 0;
        preferences.end();
        return start;
    }

    /**
     * Writes the the values for the stop time for intervall i into preferences
     * @param stop time struct holding stop time
     * @param i index of intervall
     */
    void setStopTime(tm stop, unsigned int i) {
        char stopHrString[13] = "stop_hour_XX";
        char stopMinString[12] = "stop_min_XX";
        sprintf(stopHrString, "stop_hour_%02d", i);
        sprintf(stopMinString, "stop_min_%02d", i);
        preferences.begin("brunnen", false);
        preferences.putUInt(stopHrString, stop.tm_hour);
        preferences.putUInt(stopMinString, stop.tm_min);
        preferences.end();
    }

    /**
     * Reads the the values for the stop time for intervall i from preferences
     * @param i index of intervall
     * @return time struct holding the stop time
     */
    tm getStopTime(unsigned int i) {
        char stopHrString[13] = "stop_hour_XX";
        char stopMinString[12] = "stop_min_XX";
        sprintf(stopHrString, "stop_hour_%02d", i);
        sprintf(stopMinString, "stop_min_%02d", i);

        struct tm stop;
        preferences.begin("brunnen", false);
        stop.tm_hour = preferences.getUInt(stopHrString);
        stop.tm_min = preferences.getUInt(stopMinString);
        stop.tm_sec = 0;
        preferences.end();
        return stop;
    }

    /**
     * Writes the the value for the week day for intervall i into preferences
     * @param wday value holding week days
     * @param i index of intervall
     */
    void setWeekDay(unsigned char wday, unsigned int i) {
        char wdayString[8] = "wday_XX";
        sprintf(wdayString, "wday_%02d", i);      
        preferences.begin("brunnen", false);
        preferences.putUChar(wdayString, wday);
        preferences.end();
    }

    /**
     * Reads the the value for the week day for intervall i from preferences
     * @param i index of intervall
     * @return value for week days
     */
    unsigned char getWeekDay(unsigned int i) {
        char wdayString[8] = "wday_XX";
        sprintf(wdayString, "wday_%02d", i);
        preferences.begin("brunnen", false);
        unsigned char wd = preferences.getUChar(wdayString);
        preferences.end();
        return wd;
    }

    /**
     * Writes the number of jobs to be done into preferences
     * @param jobLength number to set
     */
    void setJobLength(uint8_t jobLength) {
        preferences.begin("brunnen", false);
        preferences.putUChar("jobLength", jobLength);
        preferences.end();
    }

    /**
     * Reads the number of jobs to be done from preferences
     * @return number of jobs to be done
     */
    unsigned char getJobLength() {
        preferences.begin("brunnen", false);
        unsigned char jl = preferences.getUChar("jobLength", 0);
        preferences.end();
        return jl;
    }

    /**
     * Takes the fileName and stores it into flash memory at the position/index given by jobNumber
     * @param jobNumber position/index in job list to write to
     * @param fileName name of the data file to save to
     */
    void setJob(unsigned char jobNumber, const char* fileName) {
        //Convert filename into hash integer (DDMMYYYY as integer number)
        unsigned char i = 0; // format for fileName "/data_2022-06-23.txt"
        unsigned char offset = 0;
        while(fileName[i] != '_') {
            i++;
        }
        i++;

        char yearString[5] = "XXXX";
        offset = 0;
        while(fileName[i] != '-') {
            yearString[offset] = fileName[i];
            i++;
            offset++;
        }
        i++;
        
        char monthString[3] = "XX";
        offset = 0;
        while(fileName[i] != '-') {
            monthString[offset] = fileName[i];
            i++;
            offset++;
        }
        i++;

        char dayString[3] = "XX";
        offset = 0;
        while(fileName[i] != '.') {
            dayString[offset] = fileName[i];
            i++;
            offset++;
        }
        i++;
        
        long int year = strtol(yearString,NULL,10);
        long int month = strtol(monthString,NULL,10);
        long int day = strtol(dayString,NULL,10);
        unsigned int hash = year*10000 + month*100 + day; // hash integer (YYYYMMDD as integer number)

        char jobKey[7] = "job_XX"; // build string for preferences key
        sprintf(jobKey,"job_%02d",jobNumber);

        preferences.begin("brunnen", false);
        preferences.putUInt(jobKey, hash);
        preferences.end();
    }
    
    /**
     * Loads the file name from flash memory from position/index given by jobNumber
     * @param jobNumber position/index in job list to load from
     * @return name of data file loaded from job list
     */
    const char* getJob(unsigned char jobNumber) {
        char jobKey[7] = "job_XX"; // build string for preferences key
        sprintf(jobKey,"job_%02d",jobNumber);

        preferences.begin("brunnen", false);
        const unsigned int hash = preferences.getUInt(jobKey); // hash integer (YYYYMMDD as integer number)
        preferences.end();
        
        const unsigned int year = (hash/10000) % 10000;
        const unsigned int month = (hash/100) % 100;
        const unsigned int day = hash % 100;

        sprintf(fileNameBuffer, "/data_%04d-%02d-%02d.txt",year,month,day);
        return fileNameBuffer;
    }

    /**
     * Delets the job with the given number from preferences memory
     * @param jobNumber number (=index) of the number to delete
     */
    void removeJob(unsigned char jobNumber) {
        char jobKey[7] = "job_XX"; // build string for preferences key
        sprintf(jobKey,"job_%02d",jobNumber);

        preferences.begin("brunnen", false);
        preferences.remove(jobKey);
        preferences.end();
    }

    /**
     * Takes the threshold level and stores it into flash memory
     * @param level threshold level to store
     */
    void setThreshold(int level) {
        preferences.begin("brunnen", false);
        preferences.putInt("threshold", level);
        preferences.end();
    }

    /**
     * Loads the threshold level from flash memory
     * @return threshold level from memory
     */
    int getThreshold() {
        preferences.begin("brunnen", false);
        int threshold = preferences.getInt("threshold");
        preferences.end();
        return threshold;
    }

    /**
     * Takes a (e-mail) password and stores it into flash memory
     * @param pw password string to store
     */
    void setPassword(const char* pw) {
        preferences.begin("brunnen", false);
        preferences.putBytes("password", pw, strlen(pw));
        preferences.end();
    }

    /**
     * Loads the (e-mail) password from flash memory
     * @return password string from memory
     */
    const char* getPassword() {
        preferences.begin("brunnen", false);
        size_t pwLength = preferences.getBytes("password",passwordBuffer,STRING_LENGTH);
        preferences.end();

        return passwordBuffer;
    }

    
}

//===============================================================================================
// DATA & TIME
//===============================================================================================
/**
 * Initializes the Wlan module and sets the system time (local time) by connecting to the
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
    // Serial.print(" <<< LOG FILE /log.txt >>>\r\n");
    // Serial.print(logString);
    // Serial.print(" >>> END OF LOG FILE <<<\r\n");

    if(Pref::init()) {
        Serial.printf("Failed to read out preferences from flash memory!\r\n");
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * Connects to one of the predefined wifi networks available if not already connected to a network.
 * @return SUCCESS if the login process was succesfull. FAILURE otherwise
 */
int connectWlan() {
    bool isConnected = Wlan::isConnected();
    if (!isConnected) {
        int connectSuccess = Wlan::connect();
        if (connectSuccess != SUCCESS) {
            Log::msg(Log::ERROR,Time::toString(),"Failed to connect WiFi after multple retries.");
            return FAILURE;
        }
        Log::msg(Log::INFO,Time::toString(),"Connected to WiFi.");
    }
    return SUCCESS;
}

/**
 * Disconnects the Wifi connection if there is any connection alive
 * @return SUCCESS
 */
int disconnectWlan() {
    Wlan::disconnect();
    Log::msg(Log::INFO,Time::toString(),"Disconnected from WiFi.");
    return SUCCESS;
}

/**
 * Returns the status of the wifi connection
 * @return true, if a network is connected
 */
bool isWlanConnected() {
    return Wlan::isConnected();
}

/**
 * Loads the current time (local time) from system and returns it
 * @return struct holding the current time
 */
tm loadTimeinfo() {
    return Time::getTimeinfo();
}

/**
 * Converts the current time into a readable string format[20]: "DD-MM-YYYY HH:MM:SS"
 * @return timestamp in string format with max length TIME_STRING_LENGTH
 */
char* timeToString() {
    return Time::toString();
}

/**
 * Writes info message into log file.
 * @param msg the actual message
 * @return SUCCESS
 */
int logInfoMsg(const char* msg) {
    return Log::msg(Log::INFO,Time::toString(),msg);
}

/**
 * Writes warning message into log file.
 * @param msg the actual message
 * @return SUCCESS
 */
int logWarningMsg(const char* msg) {
    return Log::msg(Log::WARNING,Time::toString(),msg);
}

/**
 * Writes error message into log file.
 * @param msg the actual message
 * @return SUCCESS
 */
int logErrorMsg(const char* msg) {
    return Log::msg(Log::ERROR,Time::toString(),msg);
}

/**
 * Writes debug message into log file.
 * @param msg the actual message
 * @return SUCCESS
 */
int logDebugMsg(const char* msg) {
    return Log::msg(Log::DEBUG,Time::toString(),msg);
}

/**
 * Reads the content of the log file and returns it
 * @return content of the log file
 */
const char* readLogFile() {
    return Log::readFile();
}

/**
 * Checks the size of the log file. If its bigger than maxSize the log file gets cleared
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
 * Looks for the log file (/log.txt) and returns its size
 * @return size of log file in bytes
 */
int getLogFileSize() {
    return Log::getFileSize();
}

void saveStartTime(tm start, unsigned int i) {
    Pref::setStartTime(start, i);
}

tm loadStartTime(unsigned int i) {
    return Pref::getStartTime(i);
}

void saveStopTime(tm stop, unsigned int i) {
    Pref::setStopTime(stop, i);
}

tm loadStopTime(unsigned int i) {
    return Pref::getStopTime(i);
}

void saveWeekDay(unsigned char wday, unsigned int i) {
    Pref::setWeekDay(wday, i);
}

unsigned char loadWeekDay(unsigned int i) {
    return Pref::getWeekDay(i);
}

void saveJobLength(unsigned char jobLength) {
    Pref::setJobLength(jobLength);
}

unsigned char loadJobLength() {
    return Pref::getJobLength();
}

void saveJob(unsigned char jobNumber, const char* fileName) {
    Pref::setJob(jobNumber, fileName);
}

const char* loadJob(unsigned char jobNumber) {
    return Pref::getJob(jobNumber);
}

void deleteJob(unsigned char jobNumber) {
    Pref::removeJob(jobNumber);
}

void saveRainThresholdLevel(unsigned char level) {
    Pref::setThreshold(level);
}

unsigned char loadRainThresholdLevel() {
    return Pref::getThreshold();
}

void savePassword(const char* pw) {
    Pref::setPassword(pw);
}

const char* loadPassword() {
    return Pref::getPassword();
}

}
