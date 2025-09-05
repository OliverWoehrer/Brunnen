#include "Gateway.h"
#include "Config.h"
#include <ArduinoJson.h>
#include <Update.h>
#include <MD5Builder.h>  // For MD5 checksum

const char* statusToString(int statusCode) {
    switch (statusCode) {
    case 100:
        return "Continue";
    case 101:
        return "Switching Protocols";
    case 102:
        return "Processing";
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 202:
        return "Accepted";
    case 203:
        return "Non-Authoritative Information";
    case 204:
        return "No Content";
    case 205:
        return "Reset Content";
    case 206:
        return "Partial Content";
    case 207:
        return "Multi-Status";
    case 208:
        return "Already Reported";
    case 226:
        return "Im Used";
    case 300:
        return "Multiple Choices";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 303:
        return "See Other";
    case 304:
        return "Not Modified";
    case 305:
        return "Use Proxy";
    case 307:
        return "Temporary Redirect";
    case 308:
        return "Permanent Redirect";
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 402:
        return "Payment Required";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 406:
        return "Not Acceptable";
    case 407:
        return "Proxy Authentication Required";
    case 408:
        return "Request Timeout";
    case 409:
        return "Conflict";
    case 410:
        return "Gone";
    case 411:
        return "Length Required";
    case 412:
        return "Precondition Failed";
    case 413:
        return "Payload Too Large";
    case 414:
        return "URI Too Long";
    case 415:
        return "Unsupported Media Type";
    case 416:
        return "Range Not Satisfiable";
    case 417:
        return "Expectation Failed";
    case 421:
        return "Misdirected Request";
    case 422:
        return "Unprocessable Entity";
    case 423:
        return "Locked";
    case 424:
        return "Failed Dependency";
    case 426:
        return "Upgrade Required";
    case 428:
        return "Precondition Required";
    case 429:
        return "Too Many Requests";
    case 431:
        return "Request Header Fields Too Large";
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavailable";
    case 504:
        return "Gateway Timeout";
    case 505:
        return "HTTP Version Not Supported";
    case 506:
        return "Variant Also Negotiates";
    case 507:
        return "Insufficient Storage";
    case 508:
        return "Loop Detected";
    case 510:
        return "Not Extended";
    case 511:
        return "Network Authentication Required";
    default:
        return "Unknown Status";
    }
}

sync_mode_t stringToMode(const char* modeString) {
    if(strcmp(modeString, "short") == 0) {
        return SHORT;
    } else if(strcmp(modeString, "medium") == 0) {
        return MEDIUM;
    } else if(strcmp(modeString, "long") == 0) {
        return LONG;
    } else {
        return MEDIUM;
    }
}

// General Methods:

GatewayClass::GatewayClass() : led(LED_BLUE) {
    this->api_username = "";
    this->api_password = "";
    this->doc = JsonDocument();
}

void GatewayClass::load() {
    this->api_host = Config.loadAPIHost();
    this->api_port = Config.loadAPIPort();
    this->api_path = Config.loadAPIPath();
    this->api_username = Config.loadAPIUsername();
    this->api_password = Config.loadAPIPassword();
}

void GatewayClass::clear() {
    this->doc.clear();
}

std::string GatewayClass::getResponse() {
    return this->response;
}

// Tree API:
/* Example JSON:
{
    "data": {
        "columns": ["flow", "pressure", "level"],
        "values": {
            "2024-09-10T00:00:00": [3, 2, 1],
            ...
        }
    },
    "logs": {
        "2024-09-10T00:00:00": ["I am a log message", "debug"],
        "2024-09-10T00:00:01": ["I am a info message", "info"],
        ...
    },
    "settings": {
        "pump": {
            "state": false
        },
        "firmware": {
            "version": "2024-09-10T00:00:00"
        }
    }
}
*/

bool GatewayClass::insertData(std::vector<sensor_data_t> sensorData) {
    if(sensorData.size() == 0) {
        return true;
    }

    JsonObject data = this->doc["data"].to<JsonObject>();
    JsonArray columns = data["columns"].to<JsonArray>();
    columns.add("flow");
    columns.add("pressure");
    columns.add("level");
    JsonObject values = data["values"].to<JsonObject>();
    
    for(sensor_data_t sensdata : sensorData) {
        std::string ts = TimeManager::toString(sensdata.timestamp);
        JsonArray a = values[ts].to<JsonArray>();
        a.add(sensdata.flow);
        a.add(sensdata.pressure);
        a.add(sensdata.level);
    }

    return true;
}

bool GatewayClass::insertLogs(std::vector<log_message_t> logMessages) {
    if(logMessages.size() == 0) {
        return true;
    }
    JsonObject logs = this->doc["logs"].to<JsonObject>();
    
    for(log_message_t log : logMessages) {
        std::string ts = TimeManager::toString(log.timestamp);
        JsonArray a = logs[ts].to<JsonArray>();
        a.add(log.message);
        a.add(log.tag);
    }

    return true;
}

bool GatewayClass::insertFirmwareVersion(std::string &version) {
    JsonObject settings = this->doc["settings"].to<JsonObject>();
    JsonObject firmware = settings["firmware"].to<JsonObject>();
    firmware["version"] = version.c_str();
    
    return true;
}

bool GatewayClass::synchronize() {
    // Connect to WiFi:
    if(!Wlan.connect()) {
        LogFile.log(WARNING, "Cannot synchronize without network connection");
        return false;
    }

    // Initialize Resources:
    Output::Runtime run(this->led);
    HTTPClient http;
    std::string payload;

    // Initialize and Make GET Request:
    if(!http.begin(this->api_host.c_str(), this->api_port, this->api_path.c_str())) {
        LogFile.log(WARNING,"Failed to begin request!");
        return false;
    }

    // Set Headers:
    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.setAuthorization(this->api_username.c_str(), this->api_password.c_str());
    http.setUserAgent("ESP32 Brunnen");
    http.setTimeout(8000);

    // Set Payload:
    if(this->doc.isNull()) {
        payload = "{}";
    } else {
        serializeJsonPretty(this->doc, payload);
    }
    log_v("Payload:\r\n%s", payload.c_str());
    // log_d("Synchronize with payload size: %d", payload.size());

    // Start Connection:
    int httpCode = http.POST((uint8_t*)payload.c_str(), payload.size()); // start connection and send HTTP header

    // Check Response:
    if(httpCode < 0) { // httpCode is negative on error
        LogFile.log(WARNING,"Request failed: "+std::string(http.errorToString(httpCode).c_str()));
        return false;
    }
    if(httpCode != HTTP_CODE_OK) {
        LogFile.log(WARNING,"Response: ["+std::to_string(httpCode)+" "+statusToString(httpCode)+"] "+http.getString().c_str());
        return false;
    }
    if(http.getSize() > RESPONSE_BUFFER_SIZE) { // check reponse body size
        LogFile.log(WARNING,"Response body too large.");
        return false;
    }

    // Parse JSON Data:
    payload = http.getString().c_str();
    DeserializationError error = deserializeJson(this->doc, payload);
    if(error) {
        std::string msg = error.c_str();
        LogFile.log(WARNING,"Failed to parse JSON data: "+msg);
        return false;
    }
    log_v("Response: %s", response.c_str());

    // Read HTTP Response Body:
    // payload.clear();
    // http.end();
    // this->led.off();

    // Success at This Point:
    return true;
}

bool GatewayClass::getIntervals(std::vector<interval_t>& inters) {
    // Convert to JSON:
    JsonObjectConst obj = this->doc.as<JsonObjectConst>();

    // Parse JSON Document:
    JsonObjectConst settings = obj["settings"].as<JsonObjectConst>();
    if(!settings) {
        log_w("response does not have key 'settings'");
        return false;
    }
    JsonArrayConst intervals = settings["intervals"].as<JsonArrayConst>();
    if(!intervals) {
        log_w("settings has no 'intervals' array");
        return false;
    }

    // Parse Intervals:
    for(JsonObjectConst interval: intervals) {
        interval_t newInterval;

        // Check Reserved Space:
        if(inters.size() >= inters.capacity()) {
            LogFile.log(WARNING,"No more memory reserved for itervals ("+std::to_string(inters.capacity())+")");
            return false;
        }

        // Parse Start:
        const char* start = interval["start"].as<const char*>();
        if(!start) {
            LogFile.log(WARNING,"interval has no 'start' string");
            return false;
        }
        if(!TimeManager::fromTimeString(start, newInterval.start)) {
            LogFile.log(WARNING,"failed to parse start string");
            return false;
        }

        // Parse Stop:
        const char* stop = interval["stop"].as<const char*>();
        if(!stop) {
            LogFile.log(WARNING,"interval has no 'stop' string");
            return false;
        }
        if(!TimeManager::fromTimeString(stop, newInterval.stop)) {
            LogFile.log(WARNING,"failed to parse stop string");
            return false;
        }

        // Parse Weekdays:
        newInterval.wday = interval["wdays"].as<unsigned char>();
        if(!newInterval.wday) {
            LogFile.log(WARNING,"interval has not 'wdays' integer");
            return false;
        }
        
        // Push Interval to Array:
        inters.push_back(newInterval);
    }

    // Return Array of Intervals:
    return true;
}

bool GatewayClass::getSync(sync_t* buffer) {
    // Convert to JSON:
    JsonObjectConst obj = this->doc.as<JsonObjectConst>();

    // Parse JSON Document:
    JsonObjectConst settings = obj["settings"].as<JsonObjectConst>();
    if(!settings) {
        log_w("response does not have key 'settings'");
        return false;
    }
    JsonObjectConst sync = settings["sync"].as<JsonObjectConst>();
    if(!sync) {
        log_w("settings has no 'sync' object");
        return false;
    }

    // Parse Sync Periods:
    unsigned int short_period = sync["short"].as<unsigned int>();
    if(!short_period) {
        LogFile.log(WARNING,"sync does not have 'short' key");
        return false;
    }
    unsigned int medium_period = sync["medium"].as<unsigned int>();
    if(!medium_period) {
        LogFile.log(WARNING,"sync does not have 'medium' key");
        return false;
    }
    unsigned int long_period = sync["long"].as<unsigned int>();
    if(!long_period) {
        LogFile.log(WARNING,"sync does not have 'long' key");
        return false;
    }
    const char* sync_mode = sync["mode"].as<const char*>();
    if(!sync_mode) {
        LogFile.log(WARNING,"sync does not have 'mode' key");
        return false;
    }

    // Return Sync:
    buffer->periods[SHORT] = short_period;
    buffer->periods[MEDIUM] = medium_period;
    buffer->periods[LONG] = long_period;
    buffer->mode = stringToMode(sync_mode);
    return true;
}

bool GatewayClass::getFirmware(std::string &fw) {
    // Convert to JSON:
    JsonObjectConst obj = this->doc.as<JsonObjectConst>();

    // Parse JSON Document:
    JsonObjectConst settings = obj["settings"].as<JsonObjectConst>();
    if(!settings) {
        log_w("response does not have key 'settings'");
        return false;
    }
    JsonObjectConst firmware = settings["firmware"].as<JsonObjectConst>();
    if(!firmware) {
        log_w("settings has no 'firmware' object");
        return false;
    }

    // Parse Firmware Version:
    const char* version = firmware["version"].as<const char*>();
    if(!version) {
        LogFile.log(WARNING,"firmware does not have 'version' key");
        return false;
    }

    // Copy Strings into Buffer:
    fw = version;

    return true;
}

bool GatewayClass::downloadFirmware() {
    // Connect to WiFi:
    if(!Wlan.connect()) {
        LogFile.log(WARNING, "Cannot fetch firmware without network connection");
        return false;
    }
    
    // Initialize Request:
    HTTPClient http;
    std::string path = this->api_path + "/firmware";
    if(!http.begin(this->api_host.c_str(), this->api_port, path.c_str())) {
        LogFile.log(WARNING,"Failed to begin request!");
        return false;
    }

    // Set Headers:
    http.addHeader("Accept", "application/octet-stream");
    http.setAuthorization(this->api_username.c_str(), this->api_password.c_str());
    http.setUserAgent("ESP32 Brunnen");
    http.setTimeout(8000);

    // Collect Response Headers:
    const char* headerKeys[] = {"X-Firmware-Version", "X-File-Checksum"};
    size_t numberOfHeaders = sizeof(headerKeys) / sizeof(headerKeys[0]);
    http.collectHeaders(headerKeys, numberOfHeaders); // set headers to collect in the response

    // Start Connection:
    this->led.on();
    int httpCode = http.GET(); // start connection and send HTTP header
    this->led.off();

    // Check Response:
    if(httpCode < 0) { // httpCode is negative on error
        LogFile.log(WARNING,"Request failed: "+std::string(http.errorToString(httpCode).c_str()));
        return false;
    }
    if(httpCode != HTTP_CODE_OK) {
        LogFile.log(WARNING,"Response: ["+std::to_string(httpCode)+" "+statusToString(httpCode)+"] "+http.getString().c_str());
        return false;
    }
    int contentLength = http.getSize();
    if(contentLength < 0) {
        LogFile.log(WARNING, "Response has invalid size ('Content-Length' not set by server)");
        return false;
    }

    // Check Version:
    if(!http.hasHeader("X-Firmware-Version")) {
        LogFile.log(ERROR, "Server did not include firmware version into response");
        return false;
    }
    String newVersion = http.header("X-Firmware-Version");
    String oldVersion = Config.loadFirmwareVersion().c_str();
    if(newVersion.equalsIgnoreCase(oldVersion)) {
        LogFile.log(INFO, "Firmware already up to date, updating anyway");
    }

    // Start the Update Process:
    if(!Update.begin(contentLength, U_FLASH)) { // U_FLASH for flashing the application, U_SPIFFS for updating SPIFFS
        LogFile.log(WARNING, Update.errorString());
        LogFile.log(ERROR, "Not enough space to begin update or invalid size");
        return false;
    }

    // Get the stream for the downloaded file
    log_d("Downloading firmware...");
    WiFiClient& client = http.getStream();
    size_t writtenBytes = 0;
    uint8_t buff[1024];
    MD5Builder md5;
    md5.begin();
    while(client.available() && writtenBytes < contentLength) {
        size_t readBytes = client.readBytes(buff, sizeof(buff));
        if(readBytes == 0) {
            log_d("Failed to read bytes");
            continue;
        }
        md5.add(buff, readBytes); // Add to MD5 calculation
        if(Update.write(buff, readBytes) != readBytes) {
            Update.end(false); // End update with error
            LogFile.log(WARNING, Update.errorString());
            LogFile.log(ERROR, "Error writing firmware to flash.");
            return false;
        }
        writtenBytes += readBytes;
    }

    log_d("Downloaded %u / %u bytes", writtenBytes, contentLength);

    // Calculate MD5 Checksum:
    md5.calculate();
    String calculatedChecksum = md5.toString();
    log_d("Calculated MD5 checksum: %s", calculatedChecksum.c_str());

    // Check Checksum:
    if(!http.hasHeader("X-File-Checksum")) {
        LogFile.log(ERROR, "Cannot verify checksum. Response header 'X-File-Checksum' missing");
        return false;
    }
    String expectedChecksum = http.header("X-File-Checksum");
    log_d("Expected MD5 checksum: %s", expectedChecksum.c_str());
    if(!calculatedChecksum.equalsIgnoreCase(expectedChecksum)) {
        LogFile.log(ERROR, "Checksum verification failed!");
        Update.end(false); // End update with error
        return false;
    }

    // Finalize Update:
    if(!Update.end(true)) { // true to set the size to the current progress
        LogFile.log(WARNING, Update.errorString());
        LogFile.log(ERROR, "Failed to finalize firmware update");
        return false;
    }

    // Store Firmware Version:
    Config.storeFirmwareVersion(newVersion.c_str());
    
    LogFile.log(INFO, "Firmware download successfully");
    return true;
}

GatewayClass Gateway = GatewayClass();
