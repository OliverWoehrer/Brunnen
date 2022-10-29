/**
 * @author Oliver Woehrer
 * @date 17.08.2022
 * @file ui.cpp
 * This modul [User Interface] provides functions to handle requests at the web server
 *  representing the user interface of the application.
 */
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <SPIFFS.h>
#include "Arduino.h"
#include "hw.h"
#include "dt.h"
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
    if (wday & 0b00000001) ret = ret + "Sun ";
    return ret;
}

String processor(const String& var){
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
    } else if (var == "STATUS") {
        return Update.hasError() ? "FAIL" : "OK";
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
        req->send(SPIFFS, "/index.html","text/html");
    }

    //Live Data:
    void GET_live(AsyncWebServerRequest *req) {
        String str = String(DataTime::timeToString())+", "+String(Hardware::sensorValuesToString());
        req->send(200, "text/plain", str.c_str());
    }

    //Intervals:
    void GET_interval(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/interval.html", String(), false, processor);
    }

    void POST_interval(AsyncWebServerRequest *req) {
        if (req->hasParam("start_time", true) && req->hasParam("stop_time", true)) {
            struct tm start;
            struct tm stop;
            unsigned char wday = 0; // 0xFF
            int index = 0;

            int paramsNr = req->params();
            for(int i=0;i<paramsNr;i++){
                AsyncWebParameter* p = req->getParam(i);
                if (p->name().equals("start_time")) {
                    String s = p->value();
                    start.tm_hour = split(s, ':', 0).toInt();
                    start.tm_min = split(s, ':', 1).toInt();
                    start.tm_sec = 0;
                } else if (p->name().equals("stop_time")) {
                    String s = p->value();
                    stop.tm_hour = split(s, ':', 0).toInt();
                    stop.tm_min = split(s, ':', 1).toInt();
                    stop.tm_sec = 0;
                } else if (p->name().equals("index")) {
                    index = (p->value()).toInt();
                } else {
                    if (p->name().equals("sun")) wday = wday | 0b00000001;
                    if (p->name().equals("mon")) wday = wday | 0b00000010;
                    if (p->name().equals("tue")) wday = wday | 0b00000100;
                    if (p->name().equals("wed")) wday = wday | 0b00001000;
                    if (p->name().equals("thu")) wday = wday | 0b00010000;
                    if (p->name().equals("fri")) wday = wday | 0b00100000;
                    if (p->name().equals("sat")) wday = wday | 0b01000000;
                }
            }

            //Initialize interval:
            Hardware::pump_intervall_t interval = {.start = start, .stop = stop, .wday = wday};
            Hardware::setPumpInterval(interval, index);
            Hardware::saveStartTime(start, index);
            Hardware::saveStopTime(stop, index);
            Hardware::saveWeekDay(wday, index);

            req->send(SPIFFS, "/interval.html", String(), false, processor);
        } else { // invalid request
            req->send(400, "text/plain", "invalid request");
        }
    }

    //Log Page:
    void GET_log(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/log.html", String(), false, processor);
    }

    void POST_log(AsyncWebServerRequest *req) {
        if (req->hasParam("clearBtn", true)) {
            int paramsNr = req->params();
            for(int i=0;i<paramsNr;i++) {
                AsyncWebParameter* p = req->getParam(i);
                if (p->name().equals("clearBtn")) {
                    DataTime::checkLogFile(0);
                    Hardware::saveJobLength(0);
                }
            }
            req->send(SPIFFS, "/log.html", String(), false, processor);
        } else if (req->hasParam("clearLed", true)) {
            int paramsNr = req->params();
            for(int i=0;i<paramsNr;i++) {
                AsyncWebParameter* p = req->getParam(i);
                if (p->name().equals("clearLed")) {
                    Hardware::setErrorLed(LOW);
                }
            }
            req->send(SPIFFS, "/log.html", String(), false, processor);
        } else { // invalid request
            req->send(400, "text/plain", "invalid request");
        }
    }

    //Update Page:
    void GET_update(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/update.html", String(), false, processor);
    }

    void POST_update(AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/update.html", String(), false, processor);
        ESP.restart();
    }

    void upload(AsyncWebServerRequest *req, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (!index) { // upload not started yet
            Serial.printf("UploadStart: %s\n", filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
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
     * @brief initalize the web server and set the path handlers
     * @return SUCCESS
     */
    int init() {
        started = false;
        server.on("/", HTTP_GET, MyHandler::GET_homepage);
        server.on("/live", HTTP_GET, MyHandler::GET_live);
        server.on("/interval", HTTP_GET, MyHandler::GET_interval);
        server.on("/interval", HTTP_POST, MyHandler::POST_interval);
        server.on("/log", HTTP_GET, MyHandler::GET_log);
        server.on("/log", HTTP_POST, MyHandler::POST_log);
        server.on("/update", HTTP_GET, MyHandler::GET_update);
        server.on("/update", HTTP_POST, MyHandler::POST_update, MyHandler::upload);
        server.serveStatic("/", SPIFFS, "/");
        // server.onFileUpload(MyHandler::upload);
        server.onNotFound(MyHandler::notFound);
        return SUCCESS;
    }

    /**
     * @brief tells if the web server is started
     * @return true if started, false otherwise
     */
    bool isStarted() {
        return started;
    }

    /**
     * @brief starts the web server and sets the status variable used for tracking
     */
    void start() {
        server.begin();
        started = true;
    }

    /**
     * @brief stop the web server and sets the status variable used for tracking
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
 * @brief initalize the user interface
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
 * @brief Tells if the user interface is currently active/enabled
 * @return true is enabled, false otherwise
 */
bool isEnabled() {
    return MyServer::isStarted();
}

/**
 * @brief Enable user interface
 */
void enableInterface() {
    MyServer::start();
}

/**
 * @brief Disable user interface
 */
void disableInterface() {
    MyServer::stop();
}

}
