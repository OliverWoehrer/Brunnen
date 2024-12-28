#include "UserInterface.h"
#include <SPIFFS.h>
#include <Update.h>
#include "Config.h"
#include "DataFile.h"
#include "Gateway.h"
#include "LogFile.h"
#include "Pump.h"
#include "Sensors.h"
#include "TimeManager.h"
#include "WiFiManager.h"

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
        interval_t inv = Config.loadPumpInterval(0);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}"; 
    } else if (var == "INTERVAL_1") {
        interval_t inv = Config.loadPumpInterval(1);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_2") {
        interval_t inv = Config.loadPumpInterval(2);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_3") {
        interval_t inv = Config.loadPumpInterval(3);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_4") {
        interval_t inv = Config.loadPumpInterval(4);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_5") {
        interval_t inv = Config.loadPumpInterval(5);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_6") {
        interval_t inv = Config.loadPumpInterval(6);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_7") {
        interval_t inv = Config.loadPumpInterval(7);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "LIVE_DATA") {
        return String(Sensors.getWaterLevel());
    } else if (var == "FILE_SIZE") {
        return String(LogFile.size());
    } else if (var == "FILE_CONTENT") {
        std::vector<log_message_t> logs;
        logs.reserve(500);
        std::string logString;
        LogFile.exportLogs(logs);
        for(auto log: logs) {
            logString = logString + TimeManager::toString(log.timestamp) + log.message + "\r\n";
        }
        String ls = String(logString.c_str());
        ls.replace("\n","<br>");
        return ls;
    } else if(var == "RAIN") {
        int rain = Gateway.getPrecipitation();
        String str = "To my knowledge it is about to rain "+String(rain)+ "mm today. ";
        if(rain >= Config.loadRainThresholdLevel()) str += "That's enough rain, I will pause pump operation for today. ";
        else str += "That's too little rain, I will resume pump operation for today. ";
        return str;
    } else if (var == "THRESHOLD") {
        return String(Config.loadRainThresholdLevel());
    } else if (var == "JOB_LENGTH") {
        return String(Config.loadJobLength()+1);
    } else if (var == "STATUS") {
        if (Update.isRunning()) return "IN PROGRESS";
        else return Update.hasError() ? "FAIL" : "OK";
    } else if (var == "MAIL_ADDRESS") {
        return String(EMAIL_SENDER_ACCOUNT);
    } else if (var == "MAIL_PASSWORD") {
        return String(Config.loadMailPassword().c_str());
    } else if(var == "API_USERNAME") {
        return String(Config.loadAPIUsername().c_str());
    } else if(var == "API_PASSWORD") {
        return String(Config.loadAPIPassword().c_str());
    } else {
        return String();
    }
}


//===============================================================================================
// REQUEST HANDLER
//===============================================================================================
void GET_homepage(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/index.html", String(), false, processor);
}

void POST_homepage(AsyncWebServerRequest *req) {
    if(req->hasParam("clearLed", true)) {
        LogFile.acknowledge();
        req->send(SPIFFS, "/index.html", String(), false, processor);
        
    } else if(req->hasParam("threshold", true)) {
        AsyncWebParameter* p = req->getParam("threshold", true, false);
        String s = p->value();
        int level = s.toInt();
        Config.storeRainThresholdLevel((uint8_t)level);        
        LogFile.log(INFO, "Updated rain threshold to "+std::to_string(level)+" mm.");
        req->send(SPIFFS, "/index.html", String(), false, processor);
    } else if (req->hasParam("clearJobs", true)) {
        size_t jobLength = Config.loadJobLength();
        for(size_t i = 0; i < jobLength; i++) {
            Config.deleteJob(i);
        }
        Config.storeJobLength(0);
        LogFile.log(INFO, "Cleared list of jobs");
        req->send(SPIFFS, "/index.html", String(), false, processor);
    } else { // invalid request
        req->send(400, "text/plain", "invalid request");
    }
}

void GET_live(AsyncWebServerRequest *req) {
    String str = "";

    if(req->hasParam("dataString", false, false)) {
        AsyncWebParameter* p = req->getParam("dataString", false, false);
        if (p->value().equals("true")) {
            str = "Not supported";
        }
    } else if (req->hasParam("progress",false,false)) {
        AsyncWebParameter* p = req->getParam("progress",false,false);
        if (p->value().equals("true")) {
            str = Update.isRunning() ? "Upload "+String((Update.progress()*100) / Update.size())+"%" : "";
        }
    }
    req->send(200, "text/plain", str.c_str());
}

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
    if(req->hasParam("index", true)) {
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
    interval_t interval = {.start = start, .stop = stop, .wday = wday};
    Pump.removeInterval(index);
    Pump.addInterval(interval);
    Config.storePumpInterval(interval, index);
    LogFile.log(INFO, "Updated interval "+std::to_string(index)+"("+TimeManager::toTimeString(interval.start)+" - "+TimeManager::toTimeString(interval.stop)+")");

    // Send Response After Success:
    req->send(SPIFFS, "/interval.html", String(), false, processor);
}

void GET_log(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/log.html", String(), false, processor);
}

void POST_log(AsyncWebServerRequest *req) {
    if (req->hasParam("clearBtn", true)) {
        if(!LogFile.clear()) {
            req->send(500, "text/plain", "failed to clear log file");    
        }
        Config.storeJobLength(0);
        req->send(SPIFFS, "/log.html", String(), false, processor);
    } else { // invalid request
        req->send(400, "text/plain", "invalid request");
    }
}

void GET_account(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/account.html", String(), false, processor);
}

void POST_account(AsyncWebServerRequest *req) {
    // Check Address Parameter:
    String address;
    if (req->hasParam("mail_address", true)) {
        AsyncWebParameter* p = req->getParam("mail_address",true,false);
        address = p->value();
    } else { // invalid request
        req->send(400, "text/plain", "missing mail address");
    }

    // Check Password Parameter:
    String password;
    if (req->hasParam("mail_password", true)) {
        AsyncWebParameter* p = req->getParam("mail_password",true,false);
        password = p->value();
    } else { // invalid request
        req->send(400, "text/plain", "missing mail password");
    }

    // Check Username Parameter:
    String apiUsername;
    if (req->hasParam("api_username", true)) {
        AsyncWebParameter* p = req->getParam("api_username",true,false);
        apiUsername = p->value();
    } else { // invalid request
        req->send(400, "text/plain", "missing api username");
    }

    // Check Password Parameter:
    String apiPassword;
    if (req->hasParam("api_password", true)) {
        AsyncWebParameter* p = req->getParam("api_password",true,false);
        apiPassword = p->value();
    } else { // invalid request
        req->send(400, "text/plain", "missing api password");
    }

    Config.storeMailAddress(address.c_str());
    Config.storeMailPassword(password.c_str());
    Config.storeAPIUsername(apiUsername.c_str());
    Config.storeAPIPassword(apiPassword.c_str());
    Gateway.begin();
    LogFile.log(INFO, "Updated account info.");

    // Send Response After Success:
    req->send(SPIFFS, "/account.html", String(), false, processor);
}

void GET_update(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/update.html", String(), false, processor);
}

void POST_update(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/update.html", String(), false, processor);
    delay(3000);
    LogFile.log(INFO, "Device updated.");
    ESP.restart();
}

void uploadFirmware(AsyncWebServerRequest *req, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    if(!index) { // upload not started yet
        log_d("UploadStart: %s\n", filename.c_str());
        int cmd = U_FLASH; // alternative: U_FLASH
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) { //start with max available size
            Update.printError(Serial);
        }
    }
    if (Update.write(data, len) != len) {
        Update.printError(Serial);
    }
    if (final) { // upload finished
        log_d("Update Success: %u\r\n", index+len);
        if (Update.end(true)) { //true to set the size to the current progress
            log_d("Update Success: %u\nRebooting...\n", index+len);
        } else {
            Update.printError(Serial);
        }
    }
}

void GET_fileimage(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/update.html", String(), false, processor);
}

void POST_fileimage(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/update.html", String(), false, processor);
    delay(3000);
    LogFile.log(INFO, "Device updated.");
    ESP.restart();
}

void uploadFileimage(AsyncWebServerRequest *req, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    if(!index) { // upload not started yet
        log_d("UploadStart: %s\n", filename.c_str());
        int cmd = U_SPIFFS; // alternative: U_FLASH
        if(!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) { //start with max available size
            Update.printError(Serial);
        }
    }
    if(Update.write(data, len) != len) {
        Update.printError(Serial);
    }
    if(final) { // upload finished
        log_d("Update Success: %u\r\n", index+len);
        if (Update.end(true)) { //true to set the size to the current progress
            log_d("Update Success: %u\nRebooting...\n", index+len);
        } else {
            Update.printError(Serial);
        }
    }
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "not found.");
}

UserInterfaceClass::UserInterfaceClass() : led(LED_GREEN), server(UI_PORT) {
    this->state = false;

    server.on("/", HTTP_GET, GET_homepage);
    server.on("/", HTTP_POST, POST_homepage);
    server.on("/live", HTTP_GET, GET_live);
    server.on("/interval", HTTP_GET, GET_interval);
    server.on("/interval", HTTP_POST, POST_interval);
    server.on("/log", HTTP_GET, GET_log);
    server.on("/log", HTTP_POST, POST_log);
    server.on("/account", HTTP_GET, GET_account);
    server.on("/account", HTTP_POST, POST_account);
    server.on("/update", HTTP_GET, GET_update);
    server.on("/update", HTTP_POST, POST_update, uploadFirmware);
    server.on("/fileimage", HTTP_GET, GET_fileimage);
    server.on("/fileimage", HTTP_POST, POST_fileimage, uploadFileimage);
    server.serveStatic("/", SPIFFS, "/");
    // server.onFileUpload(upload);
    server.onNotFound(notFound);
}

bool UserInterfaceClass::enable() {
    if(!Wlan.connect()) {
        this->led.off();
        LogFile.log(ERROR, "Failed to enable user interface");
        return false;
    }
    this->led.on();
    this->server.begin();
    this->state = true;
    return true;
}

bool UserInterfaceClass::disable() {
    this->led.off();
    this->server.end();
    this->state = false;
    return true;
}

bool UserInterfaceClass::toggle() {
    if(this->state) {
        return this->disable();
    } else {
        return this->enable();
    }
}

UserInterfaceClass UserInterface = UserInterfaceClass();