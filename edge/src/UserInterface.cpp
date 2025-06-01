#include "UserInterface.h"
#include <SPIFFS.h>
#include <Update.h>
#include "SD.h"
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

String readableSize(const size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
    else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

String processor(const String& var) {
    if (var == "INTERVAL_0") {
        interval_t inv = Config.loadPumpInterval(0);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}"; 
    }
    if (var == "INTERVAL_1") {
        interval_t inv = Config.loadPumpInterval(1);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    }
    if (var == "INTERVAL_2") {
        interval_t inv = Config.loadPumpInterval(2);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    }
    if (var == "INTERVAL_3") {
        interval_t inv = Config.loadPumpInterval(3);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    }
    if (var == "INTERVAL_4") {
        interval_t inv = Config.loadPumpInterval(4);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    }
    if (var == "INTERVAL_5") {
        interval_t inv = Config.loadPumpInterval(5);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    }
    if (var == "INTERVAL_6") {
        interval_t inv = Config.loadPumpInterval(6);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    }
    if (var == "INTERVAL_7") {
        interval_t inv = Config.loadPumpInterval(7);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    }
    if (var == "THRESHOLD") {
        return String(Config.loadRainThresholdLevel());
    }
    if (var == "MAIL_ADDRESS") {
        return String("not used");
    }
    if (var == "MAIL_PASSWORD") {
        return String("not used");
    }
    if(var == "API_HOST") {
        return String(Config.loadAPIHost().c_str());
    }
    if(var == "API_PORT") {
        return String(Config.loadAPIPort());
    }
    if(var == "API_PATH") {
        return String(Config.loadAPIPath().c_str());
    }
    if(var == "API_USERNAME") {
        return String(Config.loadAPIUsername().c_str());
    }
    if(var == "API_PASSWORD") {
        return String(Config.loadAPIPassword().c_str());
    }
    if (var == "TOTAL_SPIFFS") {
        return readableSize(SPIFFS.totalBytes());
    }
    if (var == "USED_SPIFFS") {
        return readableSize(SPIFFS.usedBytes());
    }
    if (var == "TOTAL_SD") {
        return readableSize(SD.totalBytes());
    }
    if (var == "USED_SD") {
        return readableSize(SD.usedBytes());
    }
    if (var == "STATUS") {
        if (Update.isRunning()) return "IN PROGRESS";
        else return Update.hasError() ? "FAIL" : "OK";
    }
    return ""; // default fallback
}

//===============================================================================================
// WEBPAGE REQUEST HANDLER
//===============================================================================================

void _home(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/index.html", String(), false, processor);
}

void _favicon(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/favicon.ico", "image/*", true); //image/x-icon
}

void _filesystem(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/filesystem.html", String(), false, processor);
}

void _reboot(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/reboot.html", String(), false, processor);
}

void fileUpload(AsyncWebServerRequest *req, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    std::vector<uint8_t> *buffer = reinterpret_cast<std::vector<uint8_t> *>(req->_tempObject);

    if(!index) { // upload not started yet
        log_d("Start upload of %s at %s", filename.c_str(), req->url().c_str());     
        buffer = new std::vector<uint8_t>(); // initalize buffer and store it in req object
        req->_tempObject = buffer;
    }
    if(len) { // read incoming bytes
        buffer->insert(buffer->end(), data, data + len);
        log_d("uploading %u", index);
    }
    if(final) { // upload finished
        log_d("Upload complete %s (%u bytes)", filename.c_str(), index+len);
    }
}

void firmwareUpload(AsyncWebServerRequest *req, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if(!index) { // upload not started yet
        log_d("Firmware upload start: %s\n", filename.c_str());
        int cmd = U_FLASH; // alternative: U_FLASH
        if(!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) { //start with max available size
            log_e("Could not begin update: %s", Update.errorString());
            return;
        }
    }
    if(len) { // read incoming bytes
        if(Update.write(data, len) != len) {
            Update.printError(Serial);
            log_e("Could not write update: %s", Update.errorString());
            return;
        }
        log_d("uploading %u", index);
    }
    if(final) { // upload finished
        log_d("Upload complete %s (%u bytes)", filename.c_str(), index+len);
    }
}

void notFound(AsyncWebServerRequest *req) {
    req->send(404, "text/plain", "Not Found");
}

//===============================================================================================
// API REQUEST HANDLER
//===============================================================================================

void _api_status(AsyncWebServerRequest *req) {
    std::string timestamp = Time.toString();
    req->send(200, "text/plain", String(timestamp.c_str()));
}

void _api_interval(AsyncWebServerRequest *req) {
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

    // Send Response:
    req->redirect("/");
}

void _api_threshold(AsyncWebServerRequest *req) {
    if (req->hasParam("threshold",true)) {
        AsyncWebParameter* p = req->getParam("threshold", true, false);
        String s = p->value();
        int level = s.toInt();
        Config.storeRainThresholdLevel((uint8_t)level);        
        LogFile.log(INFO, "Updated rain threshold to "+std::to_string(level)+" mm.");
        req->redirect("/"); // redirect to home
    } else {
        req->send(400, "text/plain", "missing threshold");
    }
}

void _api_gateway(AsyncWebServerRequest *req) {
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

    // Check Host Parameter:
    String apiHost;
    if (req->hasParam("api_host", true)) {
        AsyncWebParameter* p = req->getParam("api_host",true,false);
        apiHost = p->value();
    } else { // invalid request
        req->send(400, "text/plain", "missing api host");
    }

    // Check Port Parameter:
    size_t apiPort = 80;
    if (req->hasParam("api_port", true)) {
        AsyncWebParameter* p = req->getParam("api_port",true,false);
        apiPort = (size_t)p->value().toInt();
    } else { // invalid request
        req->send(400, "text/plain", "missing api port");
    }

    // Check Path Parameter:
    String apiPath;
    if(req->hasParam("api_path", true)) {
        AsyncWebParameter* p = req->getParam("api_path",true,false);
        apiPath = p->value();
    } else { // invalid request
        req->send(400, "text/plain", "missing api path");
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

    // Store Parameters:
    Config.storeMailAddress(address.c_str());
    Config.storeMailPassword(password.c_str());
    Config.storeAPIHost(apiHost.c_str());
    Config.storeAPIPort(apiPort);
    Config.storeAPIPath(apiPath.c_str());
    Config.storeAPIUsername(apiUsername.c_str());
    Config.storeAPIPassword(apiPassword.c_str());
    Gateway.load();
    LogFile.log(INFO, "Updated credentials");

    // Send Response After Success:
    req->redirect("/"); // redirect to home
}

void _api_listfiles(AsyncWebServerRequest *req) {
    // Declare Objects:
    JsonDocument doc = JsonDocument();
    JsonArray files = doc["files"].to<JsonArray>();
    File root;
    File foundfile;

    /* Example JSON:
    {
        "files": [
            { "system": "SPIFFS", "name": "log.txt", "size": "2.1 MB" },
            { "system": "SPIFFS", "name": "index.html", "size": "345 kB" }
            { "system": "SD", "name": "data_2025-04-16.txt", "size": "2.1 MB" },
            { "system": "SD", "name": "data_2025-04-18.txt", "size": "3 kB" }
            ...
        ]
    }
    */
    
    // Scan SPIFFS:
    root = SPIFFS.open("/");
    if(!root) {
        log_e("Failed to open SPIFFS root");
        req->send(502, "text/plain", "Failed to open SPIFFS root");
    }

    //JsonArray spiffs = doc["spiffs"].to<JsonArray>();
    foundfile = root.openNextFile();
    while(foundfile) {
        JsonObject file = files.createNestedObject();
        file["system"] = "SPIFFS";
        file["name"] = (char*)foundfile.name(); // cast to "char*" instead of "const char*" to initiate deep copy
        file["size"] = readableSize(foundfile.size());
        foundfile = root.openNextFile(); // iterate next
    }
    root.close();
    foundfile.close();

    // Scan SD:
    root = SD.open("/");
    if(root) {
        //JsonArray sd = doc["sd"].to<JsonArray>();
        foundfile = root.openNextFile();
        while(foundfile) {
            JsonObject file = files.createNestedObject();
            file["system"] = "SD";
            file["name"] = (char*)foundfile.name(); // cast to "char*" instead of "const char*" to initiate deep copy
            file["size"] = readableSize(foundfile.size());
            foundfile = root.openNextFile(); // iterate next
        }
        root.close();
        foundfile.close();
    }


    // Set Payload:
    if(doc.isNull()) {
        log_e("Could not build JSON response (doc is null)");
        req->send(500, "text/plain", "Could not build JSON response");
    }
    std::string payload;
    serializeJsonPretty(doc, payload);
    req->send(200, "application/json", payload.c_str());
}

void _api_file(AsyncWebServerRequest *req) {
    // Check Parameters:
    if(!req->hasParam("system", false, false)) {
        log_d("missing system");
        req->send(400, "text/plain", "missing system");
    }
    if(!req->hasParam("name", false, false)) {
        log_d("missing name");
        req->send(400, "text/plain", "missing name");
    }
    if(!req->hasParam("action", false, false)) {
        log_d("missing action");
        req->send(400, "text/plain", "missing action");
    }

    // Log Request:
    const char *url = req->url().c_str();
    const char *filesystem = req->getParam("system", false, false)->value().c_str();
    String filename = "/" + req->getParam("name", false, false)->value();
    const char* filepath = filename.c_str();
    log_d("Action %s?system=%s&name=%s", url, filesystem, filepath);

    // File Action with Variable Filesystem:
    auto fileAction = [req, filepath](FS fs) {
        // Check If File Exists:
        if(!fs.exists(filepath)) {
            log_d("file %s does not exist");
            req->send(409, "text/plain", "file "+String(filepath)+" does not exist");
        }

        // Perform Action Based on Method:
        if(req->method() == HTTP_GET) { // download
            req->send(fs, filepath, "plain/text", true);
        }
        if(req->method() == HTTP_DELETE) { // delete
            if(!fs.remove(filepath)) {
                req->send(200, "text/plain", "failed to delete file");
            }
            req->send(200, "text/plain", "deleted file");
        }
        return;
    };

    // System Action:
    if(strcmp(filesystem, "SPIFFS") == 0) {
        fileAction(SPIFFS);
    } else if(strcmp(filesystem, "SD") == 0) {
        fileAction(SD);
    } else {
        req->send(400, "text/plain", "unknown file system");
    }
}

void _api_upload(AsyncWebServerRequest *req) {
    // Check Parameters:
    if(!req->hasParam("system", true, false)) {
        log_d("missing system");
        req->send(400, "text/plain", "missing system");
    }
    if(!req->hasParam("name", true, true)) {
        log_d("missing name");
        req->send(400, "text/plain", "missing name");
    }
    if(!req->hasParam("action", true, false)) {
        log_d("missing action");
        req->send(400, "text/plain", "missing action");
    }

    // Log Request:
    const char *url = req->url().c_str();
    const char *filesystem = req->getParam("system", true, false)->value().c_str();
    String filename = "/" + req->getParam("name", true, true)->value();
    const char* filepath = filename.c_str();
    log_d("Upload %s?system=%s&name=%s", url, filesystem, filepath);

    auto getFS = [](const char *filesystem) -> FS {
        if(strcmp(filesystem, "SPIFFS") == 0) {
            return SPIFFS;
        } else if(strcmp(filesystem, "SD") == 0) {
            return SD;
        } else {
            return SPIFFS; // default fallback
        }
    };
    FS fs = getFS(filesystem);

    // Check If File Exists:
    if(fs.exists(filepath)) {
        log_d("file %s already exists", filepath);
        req->send(409, "text/plain", "file "+String(filepath)+" already exists");
    }

    // Write File To Storage:
    log_d("Storing uploaded file %s", filepath);
    std::vector<uint8_t> *buffer = reinterpret_cast<std::vector<uint8_t> *>(req->_tempObject);
    File file = fs.open(filepath, FILE_WRITE);
    if(!file) {
        log_e("Could not open file %s", filepath);
        req->send(500, "text/plain", "failed to open file for writng");
    }
    size_t bytes = file.printf("%s", buffer->data());
    if(bytes == 0) {
        log_e("Could not write to file %s", filepath);
        req->send(500, "text/plain", "failed to write file");
    }
    req->send(200, "text/plain", "uploaded file");
}

void _api_update(AsyncWebServerRequest *req) {
    // Finalize Update:
    if(!Update.end(true)) { // true to set the size to the current progress
        log_e("Could not finalize update: %s", Update.errorString());
        req->send(502, "text/plain", "Could not finalize update");
        return;
    }

    // Reboot:
    req->redirect("/reboot"); // redirect to loading screen
    LogFile.log(INFO, "Device updated. Rebooting...");
    delay(3000);
    ESP.restart();
}

UserInterfaceClass::UserInterfaceClass() : led(LED_GREEN), server(UI_PORT) {
    this->state = false;
    
    server.on("/", HTTP_GET, _home);
    server.on("/favicon.ico", HTTP_GET, _favicon);
    server.on("/filesystem", HTTP_GET, _filesystem);
    server.on("/reboot", HTTP_GET, _reboot);
    server.on("/api/status", HTTP_GET, _api_status);
    server.on("/api/interval", HTTP_POST, _api_interval);
    server.on("/api/gateway", HTTP_POST, _api_gateway);
    server.on("/api/listfiles", HTTP_GET, _api_listfiles);
    server.on("/api/file", HTTP_GET | HTTP_DELETE, _api_file);
    server.on("/api/upload", HTTP_POST, _api_upload, fileUpload);
    server.on("/api/update", HTTP_POST, _api_update, firmwareUpload);

    // server.serveStatic("/", SPIFFS, "/");
    // server.serveStatic("/sd/", SD, "/"); // serve files from SD card
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