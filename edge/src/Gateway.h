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

// TreeAPI:
#define RESPONSE_BUFFER_SIZE 1024

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
    void load();
    void clear();
    std::string getResponse();
    
    // Tree API:
    bool insertData(std::vector<sensor_data_t> sensorData);
    bool insertLogs(std::vector<log_message_t> logMessages);
    bool insertFirmwareVersion(std::string &version);
    bool synchronize();
    bool getIntervals(std::vector<interval_t>& intervals);
    bool getSync(sync_t* sync);
    bool getFirmware(std::string &firmware);
    bool downloadFirmware();

private:
    // Hardware:
    Output::Digital led;
    
    // General Methods:
    std::string api_host;
    size_t api_port;
    std::string api_path;
    std::string api_username;
    std::string api_password;
    std::string response;

    // Requests:
    JsonDocument doc;
};

extern GatewayClass Gateway;

#endif /* GATEWAY_H */