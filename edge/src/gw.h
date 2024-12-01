#ifndef GW_H
#define GW_H

#include <ESP_Mail_Client.h>
#include <ArduinoJson.h>
#include "hw.h"

//Global return value:
#define SUCCESS 0
#define FAILURE 1

//Mail Client:
#define EMAIL_SENDER_ACCOUNT    "iot.baumgasse@gmail.com"
#define SMTP_SERVER            "smtp.gmail.com"
#define SMTP_SERVER_PORT        587 //465 //587
#define EMAIL_RECIPIENT        "oliver.woehrer@gmail.com"
#define EMAIL_SUBJECT          "ESP32 Mail with Attachment"
#define MAIL_TEXT_LENGTH 1024
#define ERROR_TEXT_LENGTH 512

//OpenMeteo API:
#define RESPONSE_BUFFER_SIZE 1024
#define OM_HOST "api.open-meteo.com"
#define OM_PATH "/v1/forecast"
#define LATITUDE "48.11"
#define LONGITUDE "16.39"
#define PARAMETER "precipitation_sum"
#define TIME_ZONE_MODE "auto"

//NTP Server:
#define NTP_SERVER "pool.ntp.org"
#define GMT_TIME_ZONE 3600
#define DAYLIGHT_OFFSET 3600

namespace Gateway {

namespace EMail {}

namespace OpenMeteoAPI {}

namespace TreeAPI{
    typedef enum {
        SHORT = 0,
        MEDIUM = 1,
        LONG = 2
    } sync_mode_t;
    typedef struct {
        unsigned int periods[3];
        sync_mode_t mode;
    } sync_t;
}

using api_sync_t = TreeAPI::sync_t;

int init(const char* smtpServer, int smtpPort, const char* address, const char* password);
int addInfoText(const char* text);
int addData(const char* fileName);
bool insertData(Hardware::sensor_data_t sensorData[], size_t lenght);
int sendData();
const char* getErrorMsg();
int clearData();

int requestWeatherData(const char* startDate, const char* endDate);
int getWeatherData(const char* data);
const char* getWeatherResponse();

int synchronize();
const char* getResponse();
int getIntervals(Hardware::pump_intervall_t* intervals);
int getSync(Gateway::api_sync_t* sync);
void clear();

}

#endif /* GW_H */