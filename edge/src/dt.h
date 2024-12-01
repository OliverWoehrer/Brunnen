#ifndef DT_H
#define DT_H

#include <Preferences.h>
#include "hw.h"

//Global return value:
#define SUCCESS 0
#define FAILURE 1

//Network Credentials:
#define WIFI_SSID_HOME "RadlerfreieWohnung_2.4G"
#define WIFI_PASSWORD_HOME "radlerraus"
#define WIFI_SSID_MOBILE "hotcon"
#define WIFI_PASSWORD_MOBILE "1gutespasswort"
#define WIFI_SSID_FIELD "TP-Link_BCDC"
#define WIFI_PASSWORD_FIELD "69001192"

//Time system:
#define TIME_STRING_LENGTH 20
#define NTP_SERVER "pool.ntp.org"
#define GMT_TIME_ZONE 3600
#define DAYLIGHT_OFFSET 3600

//Log System:
#define MAX_LOG_LENGTH 100

//Preferences:
#define FILE_NAME_LENGTH 21
#define MAX_INTERVALLS 8
#define STRING_LENGTH 40

namespace DataTime {

namespace Wlan {}

namespace Time {}

namespace Log {
    typedef enum {INFO, WARNING, ERROR, DEBUG} log_mode_t;
    int clearFile(); // TODO: remove this line
    int getFileSize();
    int readFile(char* buffer, size_t size);
}

namespace Pref {}

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
int exportLogFile(char* buffer, size_t size);
int shrinkLogFile(size_t size);

void saveStartTime(tm start, unsigned int i);
tm loadStartTime(unsigned int i);
void saveStopTime(tm stop, unsigned int i);
tm loadStopTime(unsigned int i);
void saveWeekDay(unsigned char wday, unsigned int i);
unsigned char loadWeekDay(unsigned int i);

void savePumpInterval(Hardware::button_indicator_t interval, unsigned int i);
void savePumpIntervals(Hardware::pump_intervall_t* intervals);
Hardware::pump_intervall_t loadPumpInterval(unsigned int i);

void saveJobLength(unsigned char jobLength);
unsigned char loadJobLength();
void saveJob(unsigned char jobNumber, const char* fileName);
const char* loadJob(unsigned char jobNumber);
void deleteJob(unsigned char jobNumber);
void saveRainThresholdLevel(unsigned char level);
unsigned char loadRainThresholdLevel();
void savePassword(const char* pw);
const char* loadPassword();

}

#endif /* DT_H */