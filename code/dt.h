#ifndef DT_H
#define DT_H

#include <Preferences.h>
#include <ESP_Mail_Client.h>

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
#define SD_CARD
#define SPI_CD 5
#define SPI_MOSI 23
#define SPI_CLK 18
#define SPI_MISO 19

/* redefined in em.h
Mail Client:
#define EMAIL_SENDER_ACCOUNT    "iot.baumgasse@gmail.com"
#define EMAIL_SENDER_PASSWORD   "xhqzrrmabcdttlwy" //"1ot_baumgasse"
#define EMAIL_RECIPIENT        "oliver.w@live.at"
#define SMTP_SERVER            "smtp.gmail.com"
#define SMTP_SERVER_PORT        465
#define EMAIL_SUBJECT          "ESP32 Mail with Attachment"*/

namespace Wlan {
    int init();
    int login();
    int connect();
    int disable();
    bool isConnected();
}

//===============================================================================================
// TIME
//===============================================================================================
class TIME {
private:
    char timeString[TIME_STRING_LENGTH];
public:
    TIME();
    char* toString();
    int init();
    tm getTimeinfo();
    
};

extern TIME Time;


//===============================================================================================
// LOG SYSTEM
//===============================================================================================
class LOG {
public:
    typedef enum {INFO, WARNING, ERROR, DEBUG} log_mode_t;
    LOG();
    int init();
    int msg(log_mode_t mode, const char* str);
    const char* readFile();
    int getFileSize();
    int clearFile();
};

extern LOG Log;


//===============================================================================================
// FILE SYSTEM
//===============================================================================================
class FILE_SYSTEM {
private:
    String fileName;
public:
    FILE_SYSTEM();
    void listDirectory(fs::FS &fs, const char *dirname, uint8_t levels);
    void createDir(fs::FS &fs, const char *path);
    void removeDir(fs::FS &fs, const char *path);
    void readFile(fs::FS &fs, const char *path);
    void writeFile(fs::FS &fs, const char *path, const char *message);
    void appendFile(fs::FS &fs, const char *path, const char *message);
    void deleteFile(fs::FS &fs, const char *path);
    int init();
    const char* getFileName();
};

extern FILE_SYSTEM FileSystem;

#endif /* DT_H */