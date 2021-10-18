/**
 * @author Oliver Woehrer
 * @date 17.08.2021
 * @file ui.cpp
 * This modul [User Interface] provides functions to handle requests at the web server
 *  representing the user interface of the application.
 */
#include <ESPAsyncWebServer.h>
#include "Arduino.h"
#include "hw.h"
#include "dt.h"
#include "ui.h"

//===============================================================================================
// STRING SUPPORT
//===============================================================================================
String int0String, int1String, int2String;
String liveDataString;

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
    if (wday & 0b00100000) ret = ret + "Tue ";
    if (wday & 0b01000000) ret = ret + "Fri ";
    if (wday & 0b00000001) ret = ret + "Sat ";
    return ret;
}

String processor(const String& var){
    if (var == "INTERVAL_0") {
        Relais::interval_t inv = Relais::getInterval(0);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}"; 
    } else if (var == "INTERVAL_1") {
        Relais::interval_t inv = Relais::getInterval(1);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_2") {
        Relais::interval_t inv = Relais::getInterval(2);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_3") {
        Relais::interval_t inv = Relais::getInterval(3);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_4") {
        Relais::interval_t inv = Relais::getInterval(4);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_5") {
        Relais::interval_t inv = Relais::getInterval(5);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_6") {
        Relais::interval_t inv = Relais::getInterval(6);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "INTERVAL_7") {
        Relais::interval_t inv = Relais::getInterval(7);
        return String(inv.start.tm_hour)+":"+String(inv.start.tm_min)+" - "+String(inv.stop.tm_hour)+":"+String(inv.stop.tm_min)+" {"+wdayToString(inv.wday)+"}";
    } else if (var == "LIVE_DATA") {
        liveDataString = String(Time.toString())+", "+String(Sensors::toString());
        return liveDataString;
    } else {
        return String();
    }
}


//===============================================================================================
// REQUEST HANDLER
//===============================================================================================
//Homepage:
void handle_Homepage(AsyncWebServerRequest *req) {
    req->send(SPIFFS, "/index.html","text/html");
}

//Live Data:
void handle_live(AsyncWebServerRequest *req) {
    String str = String(Time.toString())+", "+String(Sensors::toString());
    req->send(200, "text/plain", str.c_str());
}

//Intervals:
void handle_GET_interval(AsyncWebServerRequest *req) {
    //req->send(SPIFFS, "/interval.html","text/html");
    req->send(SPIFFS, "/interval.html", String(), false, processor);
}

void handle_POST_interval(AsyncWebServerRequest *req) {
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
        Relais::interval_t interval = {.start = start, .stop = stop, .wday = wday};
        Relais::setInterval(interval, index);
        Prefs.setStartTime(start, index);
        Prefs.setStopTime(stop, index);
        Prefs.setWeekDay(wday, index);

        req->send(SPIFFS, "/interval.html", String(), false, processor);
    } else { // invalid request
        req->send(400, "text/plain", "invalid request");
    }
}

//Error Page:
void handle_NotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "not found.");
}


//===============================================================================================
// USER INTERFACE
//===============================================================================================
AsyncWebServer server(80);

WEBINTERFACE::WEBINTERFACE() {
    enabled = false;
}

int WEBINTERFACE::init() {
    server.on("/", HTTP_GET, handle_Homepage);
    server.on("/live", HTTP_GET, handle_live);
    server.on("/interval", HTTP_GET, handle_GET_interval);
    server.on("/interval", HTTP_POST, handle_POST_interval);
    server.serveStatic("/", SPIFFS, "/");
    server.onNotFound(handle_NotFound);
}

void WEBINTERFACE::handleClient() {
    //server.handleClient();
}

int WEBINTERFACE::toggle() {
    if (enabled) // is enabled, turn off now
        server.end();
    else // not enbaled, turn on now
        server.begin();
    enabled = !enabled;
    return enabled;
}

WEBINTERFACE Ui;
