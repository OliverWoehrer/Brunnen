#include "Gateway.h"
#include <ArduinoJson.h>

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
        "software": {
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

bool GatewayClass::synchronize() {
    // Connect to WiFi:
    if(!Wlan.connect()) {
        LogFile.log(WARNING, "Cannot synchronize without network connection");
        return false;
    }

    this->led.on();
    HTTPClient http;
    std::string payload;
    bool success = true;
    do {

    // Initialize and Make GET Request:
    if(!http.begin(this->api_host.c_str(), this->api_port, this->api_path.c_str())) {
        LogFile.log(WARNING,"Failed to begin request!");
        success = false;
        break;
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
        success = false;
        break;
    }
    if(httpCode != HTTP_CODE_OK) {
        LogFile.log(WARNING,"Response: ["+std::to_string(httpCode)+" "+statusToString(httpCode)+"] "+http.getString().c_str());
        success = false;
        break;
    }
    if(http.getSize() > RESPONSE_BUFFER_SIZE) { // check reponse body size
        LogFile.log(WARNING,"Response body too large.");
        success = false;
        break;
    }

    // Parse JSON Data:
    payload = http.getString().c_str();
    DeserializationError error = deserializeJson(this->doc, payload);
    if(error) {
        std::string msg = error.c_str();
        LogFile.log(WARNING,"Failed to parse JSON data: "+msg);
        success = false;
        break;
    }
    log_v("Response: %s", response.c_str());

    } while(0);

    // Read HTTP Response Body:
    payload.clear();
    http.end(); // clear http object
    this->led.off();

    // Success at This Point:
    return success;
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
    unsigned int short_periode = sync["short"].as<unsigned int>();
    if(!short_periode) {
        LogFile.log(WARNING,"sync does not have 'short' key");
        return false;
    }
    unsigned int medium_periode = sync["medium"].as<unsigned int>();
    if(!medium_periode) {
        LogFile.log(WARNING,"sync does not have 'medium' key");
        return false;
    }
    unsigned int long_periode = sync["long"].as<unsigned int>();
    if(!long_periode) {
        LogFile.log(WARNING,"sync does not have 'long' key");
        return false;
    }
    const char* sync_mode = sync["mode"].as<const char*>();
    if(!sync_mode) {
        LogFile.log(WARNING,"sync does not have 'mode' key");
        return false;
    }

    // Return Sync:
    buffer->periods[SHORT] = short_periode;
    buffer->periods[MEDIUM] = medium_periode;
    buffer->periods[LONG] = long_periode;
    buffer->mode = stringToMode(sync_mode);
    return true;
}

GatewayClass Gateway = GatewayClass();
