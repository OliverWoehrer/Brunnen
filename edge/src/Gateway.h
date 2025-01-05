#ifndef GATEWAY_H
#define GATEWAY_H


// Libraries:
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <ESP_Mail_Client.h>

// Peripherals:
#include "Config.h"
#include "LogFile.h"
#include "TimeManager.h"
#include "WiFiManager.h"

// Modules:
#include "Pump.h"
#include "Sensors.h"

// Pin Definitions:
#define LED_BLUE 2

// Tree API:
#define TREE_HOST "app.woehrer-consulting.at" //"192.168.1.104"
#define TREE_PORT 80 //5000
#define TREE_PATH "/api/device/brunnen"

// OpenMeteo API:
#define RESPONSE_BUFFER_SIZE 1024
#define OM_HOST "api.open-meteo.com"
#define OM_PATH "/v1/forecast"
#define LATITUDE "48.11"
#define LONGITUDE "16.39"
#define PARAMETER "precipitation_sum"
#define TIME_ZONE_MODE "auto"

// Mail Client:
#define EMAIL_SENDER_ACCOUNT "iot.baumgasse@gmail.com"
#define SMTP_SERVER "smtp.gmail.com"
#define SMTP_SERVER_PORT 587
#define EMAIL_RECIPIENT "oliver.woehrer@gmail.com"
#define EMAIL_SUBJECT "ESP32 Mail with Attachment"
#define MAIL_TEXT_LENGTH 1024
#define ERROR_TEXT_LENGTH 512

// NTP Server:
#define NTP_SERVER "pool.ntp.org"
#define GMT_TIME_ZONE 3600
#define DAYLIGHT_OFFSET 3600

typedef enum {
    SHORT = 0,
    MEDIUM = 1,
    LONG = 2
} sync_mode_t;

typedef struct {
    unsigned int periods[3];
    sync_mode_t mode;
} sync_t;

class GatewayClass {
public:
    // General Methods:
    GatewayClass();
    void begin();
    void clear();
    std::string getResponse();
    
    // Tree API:
    bool insertData(std::vector<sensor_data_t> sensorData);
    bool insertLogs(std::vector<log_message_t> logMessages);
    bool synchronize();
    bool getIntervals(std::vector<interval_t>& intervals);
    bool getSync(sync_t* sync);

    // OpenMeteo API:
    bool requestWeatherData();
    int getPrecipitation();

    // E-Mail:
    bool attachFile(const std::string filename);
    bool sendMail(std::string& text);

private:
    // Hardware:
    Output::Digital led;
    
    // General Methods:
    std::string api_username;
    std::string api_password;
    std::string mail_address;
    std::string mail_password;
    std::string response;

    // Requests:
    JsonDocument doc;
};

extern GatewayClass Gateway;

#endif /* GATEWAY_H */