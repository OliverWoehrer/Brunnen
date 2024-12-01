/**
 * @author Oliver Woehrer
 * @date 17.08.2022
 * @file ui.cpp
 * This modul [User Interface] provides functions to handle requests at the web server
 * representing the user interface of the application.
 */
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <SPIFFS.h>
#include "Arduino.h"
#include "hw.h"
#include "dt.h"
#include "gw.h"
#include "ui.h"

namespace UserInterface {

//===============================================================================================
// STRING SUPPORT
//===============================================================================================

String split(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;
    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String wdayToString(unsigned char wday) {
    String ret = "";
    if (wday & 0b00000010) ret = ret + "Mon ";
    if (wday & 0b00000100) ret = ret + "Tue ";
    if (wday & 0b00001000) ret = ret + "Wed ";
    if (wday & 0b00010000) ret = ret + "Thu ";
    if (wday & 0b00100000) ret = ret + "Fri ";
    if (wday & 0b01000000) ret = ret + "Sat ";
    if (wday & 0b00000001) ret = ret + "Sun";
    return ret;
}

String processor(const String& var) {
    if (var == "INTERVAL_0") {
        Hardware::pump_intervall_t inv = Hardware::getPumpInterval(0);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}"; 
    } else if (var == "INTERVAL_1") {
        Hardware::pump_intervall_t inv = Hardware::getPumpInterval(1);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_2") {
        Hardware::pump_intervall_t inv = Hardware::getPumpInterval(2);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_3") {
        Hardware::pump_intervall_t inv = Hardware::getPumpInterval(3);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_4") {
        Hardware::pump_intervall_t inv = Hardware::getPumpInterval(4);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_5") {
        Hardware::pump_intervall_t inv = Hardware::getPumpInterval(5);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_6") {
        Hardware::pump_intervall_t inv = Hardware::getPumpInterval(6);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_7") {
        Hardware::pump_intervall_t inv = Hardware::getPumpInterval(7);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "LIVE_DATA") {
        return String(DataTime::timeToString())+", "+String(Hardware::sensorValuesToString());
    } else if (var == "FILE_SIZE") {
        return String(DataTime::getLogFileSize());
    } else if (var == "FILE_CONTENT") {
        const char* logString = DataTime::readLogFile();
        String logStr = String(logString);
        logStr.replace("\n","<br>");
        return String(logStr);
    } else if(var == "RAIN") {
        int rain = Gateway::getWeatherData("precipitation");
        String str = "To my knowledge it is about to rain "+String(rain)+ "mm today. ";
        if (rain >= Hardware::getPumpOperatingLevel()) str += "That's enough rain, I will pause pump operation for today. ";
        else str += "That's too little rain, I will resume pump operation for today. ";
        return str;
    } else if (var == "THRESHOLD") {
        return String(Hardware::getPumpOperatingLevel());
    } else if (var == "JOB_LENGTH") {
        return String(DataTime::loadJobLength()+1);
    } else if (var == "STATUS") {
        if (Update.isRunning()) return "IN PROGRESS";
        else return Update.hasError() ? "FAIL" : "OK";
    } else if (var == "SMTP_SERVER") {
        return String(SMTP_SERVER);
    } else if (var == "SMTP_PORT") {
        return String(SMTP_SERVER_PORT);
    } else if (var == "ADDRESS") {
        return String(EMAIL_SENDER_ACCOUNT);
    } else if (var == "PASSWORD") {
        return String(DataTime::loadPassword());
    } else {
        return String();
    }
}

//===============================================================================================
// REQUEST HANDLER
//===============================================================================================
namespace MyHandler {
    //Homepage:
    void GET_homepage(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/index.html", String(), false, processor);
    }

    void POST_homepage(AsyncWebServerRequest *req) {
        if (req->hasParam("clearLed", true)) {
            Hardware::setErrorLed(LOW);
            DataTime::logInfoMsg("Cleared error led.");
            req->send(SPIFFS, "/index.html", String(), false, processor);
        } else if (req->hasParam("threshold", true)) {
            AsyncWebParameter* p = req->getParam("threshold",true,false);
            String s = p->value();
            int level = s.toInt();
            Hardware::setPumpOperatingLevel(level);
            DataTime::saveRainThresholdLevel(level);
            char txtBuffer[40];
            sprintf(txtBuffer,"Updated rain threshold to %02d mm.",level);
            DataTime::logInfoMsg(txtBuffer);
            req->send(SPIFFS, "/index.html", String(), false, processor);
        } else if (req->hasParam("clearJobs", true)) {
            unsigned char jobLength = DataTime::loadJobLength();
            for (unsigned int i=0; i < jobLength; i++) {
                const char* jobName = DataTime::loadJob(i);
                Hardware::setActiveDataFile(jobName);
                Hardware::deleteActiveDataFile();
                DataTime::deleteJob(i);
            }
            DataTime::saveJobLength(0);
            DataTime::logInfoMsg("Cleared list of jobs.");
            req->send(SPIFFS, "/index.html", String(), false, processor);
        } else { // invalid request
            req->send(400, "text/plain", "invalid request");
        }
    }

    //Live Data:
    void GET_live(AsyncWebServerRequest *req) {
        String str = "";
        if (req->hasParam("dataString",false,false)) {
            AsyncWebParameter* p = req->getParam("dataString",false,false);
            if (p->value().equals("true")) {
                str = String(DataTime::timeToString())+", "+String(Hardware::sensorValuesToString());
            }
        } else if (req->hasParam("progress",false,false)) {
            AsyncWebParameter* p = req->getParam("progress",false,false);
            if (p->value().equals("true")) {
                str = Update.isRunning() ? "Upload "+String((Update.progress()*100) / Update.size())+"%" : "";
            }
        }
        req->send(200, "text/plain", str.c_str());
    }

    //Intervals:
    void GET_interval(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/interval.html", String(), false, processor);
    }

    void POST_interval(AsyncWebServerRequest *req) {
        struct tm start;
        struct tm stop;
        unsigned char wday = 0; // 0xFF
        int index = 0;

        // Check Start Time Parameter:
        if (req->hasParam("start_time", true)) {
            AsyncWebParameter* p = req->getParam("start_time",true,false);
            String s = p->value();
            start.tm_hour = split(s, ':', 0).toInt();
            start.tm_min = split(s, ':', 1).toInt();
            start.tm_sec = 0;
        } else { // invalid request
            req->send(400, "text/plain", "invalid request: missing start_time parameter");
        }

        // Check Stop Time Parameter:
        if (req->hasParam("stop_time", true)) {
            AsyncWebParameter* p = req->getParam("stop_time",true,false);
            String s = p->value();
            stop.tm_hour = split(s, ':', 0).toInt();
            stop.tm_min = split(s, ':', 1).toInt();
            stop.tm_sec = 0;
        } else { // invalid request
            req->send(400, "text/plain", "invalid request: missing stop_time parameter");
        }

        // Check Index Parameter:
        if (req->hasParam("index", true)) {
            AsyncWebParameter* p = req->getParam("index",true,false);
            index = (p->value()).toInt();
        } else { // invalid request
            req->send(400, "text/plain", "invalid request: missing index parameter");
        }

        // Check Weekday Parameter:
        if (req->hasParam("sun", true)) wday = wday | 0b00000001;
        if (req->hasParam("mon", true)) wday = wday | 0b00000010;
        if (req->hasParam("tue", true)) wday = wday | 0b00000100;
        if (req->hasParam("wed", true)) wday = wday | 0b00001000;
        if (req->hasParam("thu", true)) wday = wday | 0b00010000;
        if (req->hasParam("fri", true)) wday = wday | 0b00100000;
        if (req->hasParam("sat", true)) wday = wday | 0b01000000;
        // if (wday == 0) req->send(400, "text/plain", "invalid request: missing weekday parameter");

        // Initialize interval:
        Hardware::pump_intervall_t interval = {.start = start, .stop = stop, .wday = wday};
        Hardware::setPumpInterval(interval, index);
        DataTime::saveStartTime(start, index);
        DataTime::saveStopTime(stop, index);
        DataTime::saveWeekDay(wday, index);

        // Log Update:
        char txtBuffer[40];
        sprintf(txtBuffer,"Updated interval %02d (%02d:%02d - %02d:%02d).",index,interval.start.tm_hour,interval.start.tm_min,interval.stop.tm_hour,interval.stop.tm_min);
        DataTime::logInfoMsg(txtBuffer);

        // Send Response After Success:
        req->send(SPIFFS, "/interval.html", String(), false, processor);
    }

    //Log Page:
    void GET_log(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/log.html", String(), false, processor);
    }

    void POST_log(AsyncWebServerRequest *req) {
        if (req->hasParam("clearBtn", true)) {
            DataTime::checkLogFile(0);
            DataTime::saveJobLength(0);
            req->send(SPIFFS, "/log.html", String(), false, processor);
        } else { // invalid request
            req->send(400, "text/plain", "invalid request");
        }
    }

    //Account Page:
    void GET_account(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/account.html", String(), false, processor);
    }

    void POST_account(AsyncWebServerRequest *req) {
        char smtpServer[STRING_LENGTH] = "";
        int smtpPort = 0;
        char address[STRING_LENGTH] = "";
        char password[STRING_LENGTH] = "";

        // Check SMTP Server Parameter:
        if (req->hasParam("smtpServer", true)) {
            AsyncWebParameter* p = req->getParam("smtpServer",true,false);
            memcpy(smtpServer, p->value().c_str(), p->value().length());
        } else { // invalid request
            req->send(400, "text/plain", "invalid request: missing stmp server");
        }

        // Check SMTP Port Parameter:
        if (req->hasParam("smtpPort", true)) {
            AsyncWebParameter* p = req->getParam("smtpPort",true,false);
            int smtpPort = p->value().toInt();
        } else { // invalid request
            req->send(400, "text/plain", "invalid request: missing stmp port");
        }

        // Check Address Parameter:
        if (req->hasParam("address", true)) {
            AsyncWebParameter* p = req->getParam("address",true,false);
            memcpy(address, p->value().c_str(), p->value().length());
        } else { // invalid request
            req->send(400, "text/plain", "invalid request: missing address");
        }

        // Check Password Parameter:
        if (req->hasParam("password", true)) {
            AsyncWebParameter* p = req->getParam("password",true,false);
            memcpy(password, p->value().c_str(), p->value().length());
        } else { // invalid request
            req->send(400, "text/plain", "invalid request: missing password");
        }

        Gateway::init(SMTP_SERVER,SMTP_SERVER_PORT,EMAIL_SENDER_ACCOUNT,password);
        DataTime::savePassword(password);

        // Log Update:
        DataTime::logInfoMsg("Updated account info.");

        // Send Response After Success:
        req->send(SPIFFS, "/account.html", String(), false, processor);
    }

    //Update Page:
    void GET_update(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/update.html", String(), false, processor);
    }

    void POST_update(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/update.html", String(), false, processor);
        delay(3000);
        DataTime::logInfoMsg("Device updated.");
        ESP.restart();
    }

    void uploadFirmware(AsyncWebServerRequest *req, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (!index) { // upload not started yet
            Serial.printf("UploadStart: %s\n", filename.c_str());
            int cmd = U_FLASH; // alternative: U_FLASH
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) { //start with max available size
                Update.printError(Serial);
            }
        }
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
        if (final) { // upload finished
            Serial.printf("Update Success: %u\r\n", index+len);
            if (Update.end(true)) { //true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", index+len);
            } else {
                Update.printError(Serial);
            }
        }
    }

    //Files Image Page:
    void GET_fileimage(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/update.html", String(), false, processor);
    }

    void POST_fileimage(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/update.html", String(), false, processor);
        delay(3000);
        DataTime::logInfoMsg("Device updated.");
        ESP.restart();
    }

    void uploadFileimage(AsyncWebServerRequest *req, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (!index) { // upload not started yet
            Serial.printf("UploadStart: %s\n", filename.c_str());
            int cmd = U_SPIFFS; // alternative: U_FLASH
            if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) { //start with max available size
                Update.printError(Serial);
            }
        }
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
        if (final) { // upload finished
            Serial.printf("Update Success: %u\r\n", index+len);
            if (Update.end(true)) { //true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", index+len);
            } else {
                Update.printError(Serial);
            }
        }
    }

    //Error Page:
    void notFound(AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "not found.");
    }
}

//===============================================================================================
// WEB SERVER
//===============================================================================================
namespace MyServer {
    AsyncWebServer server(80); // server object on port 80
    bool started;

    /**
     * Initalize the web server and set the path handlers
     * @return SUCCESS
     */
    int init() {
        started = false;
        server.on("/", HTTP_GET, MyHandler::GET_homepage);
        server.on("/", HTTP_POST, MyHandler::POST_homepage);
        server.on("/live", HTTP_GET, MyHandler::GET_live);
        server.on("/interval", HTTP_GET, MyHandler::GET_interval);
        server.on("/interval", HTTP_POST, MyHandler::POST_interval);
        server.on("/log", HTTP_GET, MyHandler::GET_log);
        server.on("/log", HTTP_POST, MyHandler::POST_log);
        server.on("/account", HTTP_GET, MyHandler::GET_account);
        server.on("/account", HTTP_POST, MyHandler::POST_account);
        server.on("/update", HTTP_GET, MyHandler::GET_update);
        server.on("/update", HTTP_POST, MyHandler::POST_update, MyHandler::uploadFirmware);
        server.on("/fileimage", HTTP_GET, MyHandler::GET_fileimage);
        server.on("/fileimage", HTTP_POST, MyHandler::POST_fileimage, MyHandler::uploadFileimage);
        server.serveStatic("/", SPIFFS, "/");
        // server.onFileUpload(MyHandler::upload);
        server.onNotFound(MyHandler::notFound);
        return SUCCESS;
    }

    /**
     * Tells if the web server is started
     * @return true if started, false otherwise
     */
    bool isStarted() {
        return started;
    }

    /**
     * Starts the web server and sets the status variable used for tracking
     */
    void start() {
        server.begin();
        started = true;
    }

    /**
     * Stop the web server and sets the status variable used for tracking
     */
    void stop() {
        server.end();
        started = false;
    }
}

//===============================================================================================
// USER INTERFACE
//===============================================================================================

/**
 * initalize the user interface
 * @return SUCCESS if all started, FAILURE otherwise
 */
int init() {
    if(MyServer::init()) {
        Serial.printf("[ERROR] Failed to initialize Userinterface!\r\n");
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * Tells if the user interface is currently active/enabled
 * @return true is enabled, false otherwise
 */
bool isEnabled() {
    return MyServer::isStarted();
}

/**
 * Enable user interface
 */
void enableInterface() {
    MyServer::start();
}

/**
 * Disable user interface
 */
void disableInterface() {
    MyServer::stop();
}

}
