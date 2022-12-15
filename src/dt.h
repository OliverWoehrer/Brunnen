#ifndef DT_H
#define DT_H

#include <Preferences.h>

//Global return value:
#define SUCCESS 0
#define FAILURE 1

//Network Credentials:
#define WIFI_SSID_HOME "RadlerfreieWohnung_2.4G"
#define WIFI_PASSWORD_HOME "radlerraus"
#define WIFI_SSID_FIELD "TP-Link_BCDC"
#define WIFI_PASSWORD_FIELD "69001192"

//Time system:
#define TIME_STRING_LENGTH 20
#define NTP_SERVER "pool.ntp.org"
#define GMT_TIME_ZONE 3600
#define DAYLIGHT_OFFSET 3600

//File System:
#define FILE_NAME_LENGTH 21
#define SD_CARD
#define SPI_CD 5
#define SPI_MOSI 23
#define SPI_CLK 18
#define SPI_MISO 19

namespace DataTime {

namespace Wlan {}

namespace Time {}

namespace Log {
    typedef enum {INFO, WARNING, ERROR, DEBUG} log_mode_t;
}

namespace FileSystem {}

int init();
int connectWlan();
int disconnectWlan();
bool isWlanConnected();

tm loadTimeinfo();
char* timeToString();

int logInfoMsg(const char* msg);
int logWarningMsg(const char* msg);
int logErrorMsg(const char* msg);
int logDebugMsg(const char* msg);
const char* readLogFile();
int checkLogFile(int maxSize);
int getLogFileSize();

int createCurrentDataFile();
char* loadActiveDataFileName();
void deleteActiveDataFile();
int setActiveDataFile(const char* fName);
void writeToDataFile(const char* msg);

}

#endif /* DT_H */