/**
 * @author Oliver Woehrer, 11907563
 * @date 21.03.2021
 * 
 * > > > > > BRUNNEN  < < < < <
 * System to meassure water flow and water level in well
 * 
 * [For use of Visual Studio Code]
 * Set configuration parameters in c_cpp_properties.json
 *  > "defines": ["USBCON"]  for IntelliSense to work with serial monitor functions.
 * To disable the debugg logger messages (e.g "TRACE StatusLogger") add the following
 *  > -DDEBUG=false
 * to the arduino config file in the arduino directory.
 * On windows this can be found at "C:\Program Files (x86)\Arduino\arduino_debug.l4j.ini"
 * This has to be done because debug messages are enabled per default in Visual Studio Code
 * For use with Arduino Nano set in arduino.json
 *  > "configuration": "cpu=atmega328old" in order to use old bootloader on Arduino Nano board.
 *  > "programmer": "AVRISP mkII"
 *  > "output": "../build"
 * 
**/
//===============================================================================================
// LIBRARIES
//===============================================================================================
#include <SPIFFS.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include "Arduino.h" // include basic arduino functions
#include "sys.h" // provides functions for loop handling and system time
#include "wlan.h"
#include "sensors.h"
#include "data.h"

#include "hw.h"

// define the number of bytes you want to access
#define EEPROM_SIZE 12
//===============================================================================================
// MAIN FUNCTIONS
//===============================================================================================
//Variables for periodic loop:
volatile bool loopEntry; // gets set true every LOOP_PERIOD (default: 1 sec)
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; // mutex semaphore for loopEntry variable
hw_timer_t *loopTimer = NULL;
static void IRAM_ATTR onTimer(){
  portENTER_CRITICAL_ISR(&timerMux);
  loopEntry = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}

static void stopTimer(hw_timer_t *timer) {
    if (timer) { // check if timer still runs
        timerEnd(timer); // Stop and free timer
        timer = NULL;
    }
}

extern bool buttonPressed;
volatile bool longpress;
hw_timer_t *longpressTimer = NULL;
static void IRAM_ATTR longpressed(){
  longpress = true;
}

uint32_t flow = 0;
uint16_t pressure = 0;
uint16_t level = 0;

AsyncWebServer server(80);


interval_t int1;
interval_t int2;
interval_t int3;

//===============================================================================================
// MAIN PROGRAMM
//===============================================================================================
void setup() {
    Serial.begin(BAUD_RATE);
    delay(1000); // wait for hardware on PCB to wake up

    //Initialize system utilities
    initLEDs();
    initRelaisPins();
    if (initSPIFFS()) { // no log file available
        Serial.printf("[ERROR] failed to mount SPIFFS\n");
        digitalWrite(LED_RED, HIGH);
        return;
    }
    if (DEBUG) readLogFile();
    

    //Setup wifi connection and system time:
    if (initWifi()) { // error handling
        logMsg(LOG_ERROR, "Failed to connect WiFi");
        digitalWrite(LED_RED, HIGH);
        return;
    }
    initTime();
    

    //Set up cd card storage:
    EEPROM.begin(EEPROM_SIZE);
    if (initSDcard()) {
        logMsg(LOG_ERROR, "Failed to initialze SD card shield");
        digitalWrite(LED_RED, HIGH);
        return;
    }
    initMail();


    //Initialize Web Server User Interface:
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){   
        request->send(200, "text/plain", "up and running");
    });

    server.on("/live", HTTP_GET, [](AsyncWebServerRequest *request){
        String dataMessage = String(getTimeString())+","+String(flow)+","+String(pressure)+","+String(level)+"\r\n";
        request->send(200, "text/plain", dataMessage.c_str());
    });

    server.on("/setInt1", HTTP_GET, [](AsyncWebServerRequest *req) {
        String res = "invalid url query";
        if (req->hasParam("start_hour") && req->hasParam("start_min")) {
            int1.start.tm_hour = (req->getParam("start_hour")->value()).toInt();
            int1.start.tm_min = (req->getParam("start_min")->value()).toInt();
            res = "Start Intervall 2 set to "+req->getParam("start_hour")->value()+":"+req->getParam("start_min")->value();
            EEPROM.write(0, int1.start.tm_hour);
            EEPROM.write(1, int1.start.tm_min);
            EEPROM.commit();
            req->send(200, "text/plain", "message received: "+res);
        } else if (req->hasParam("stop_hour") && req->hasParam("stop_min")) {
            int1.stop.tm_hour = (req->getParam("stop_hour")->value()).toInt();
            int1.stop.tm_min = (req->getParam("stop_min")->value()).toInt();
            res = "Stop Intervall 1 set to "+req->getParam("stop_hour")->value()+":"+req->getParam("stop_min")->value();
            EEPROM.write(2, int1.stop.tm_hour);
            EEPROM.write(3, int1.stop.tm_min);
            EEPROM.commit();
            req->send(200, "text/plain", "message received: "+res);
        } else { // invalid request
            req->send(400, "text/plain", "message received: "+res);
        }
    });

    server.on("/setInt2", HTTP_GET, [](AsyncWebServerRequest *req) {
        String res = "invalid url query";
        if (req->hasParam("start_hour") && req->hasParam("start_min")) {
            int2.start.tm_hour = (req->getParam("start_hour")->value()).toInt();
            int2.start.tm_min = (req->getParam("start_min")->value()).toInt();
            res = "Start Intervall 2 set to "+req->getParam("start_hour")->value()+":"+req->getParam("start_min")->value();
            EEPROM.write(4, int2.start.tm_hour);
            EEPROM.write(5, int2.start.tm_min);
            EEPROM.commit();
            req->send(200, "text/plain", "message received: "+res);
        } else if (req->hasParam("stop_hour") && req->hasParam("stop_min")) {
            int2.stop.tm_hour = (req->getParam("stop_hour")->value()).toInt();
            int2.stop.tm_min = (req->getParam("stop_min")->value()).toInt();
            res = "Stop Intervall 2 set to "+req->getParam("stop_hour")->value()+":"+req->getParam("stop_min")->value();
            EEPROM.write(6, int2.stop.tm_hour);
            EEPROM.write(7, int2.stop.tm_min);
            EEPROM.commit();
            req->send(200, "text/plain", "message received: "+res);
        } else { // invalid request
            req->send(400, "text/plain", "message received: "+res);
        }
    });

    server.on("/setInt3", HTTP_GET, [](AsyncWebServerRequest *req) {
        String res = "invalid url query";
        if (req->hasParam("start_hour") && req->hasParam("start_min")) {
            int3.start.tm_hour = (req->getParam("start_hour")->value()).toInt();
            int3.start.tm_min = (req->getParam("start_min")->value()).toInt();
            res = "Start Intervall 2 set to "+req->getParam("start_hour")->value()+":"+req->getParam("start_min")->value();
            EEPROM.write(8, int3.start.tm_hour);
            EEPROM.write(9, int3.start.tm_min);
            EEPROM.commit();
            req->send(200, "text/plain", "message received: "+res);
        } else if (req->hasParam("stop_hour") && req->hasParam("stop_min")) {
            int3.stop.tm_hour = (req->getParam("stop_hour")->value()).toInt();
            int3.stop.tm_min = (req->getParam("stop_min")->value()).toInt();
            res = "Stop Intervall 3 set to "+req->getParam("stop_hour")->value()+":"+req->getParam("stop_min")->value();
            EEPROM.write(10, int3.stop.tm_hour);
            EEPROM.write(11, int3.stop.tm_min);
            EEPROM.commit();
            req->send(200, "text/plain", "message received: "+res);
        } else { // invalid request
            req->send(400, "text/plain", "message received: "+res);
        }
    });

    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
        String dataMessage = String(getTimeString());
        request->send(200, "text/plain", dataMessage.c_str());
    });

    Serial.printf("[INFO] server established.\n");
    server.begin();
    

    //Initialize sensors:
    
    initWaterflowSensor();
    initWaterPressureSensor();
    initWaterLevelSensor();


    //Initialize relias intervals:
    int1.start.tm_hour = EEPROM.read(0);
    int1.start.tm_min = EEPROM.read(1);
    int1.stop.tm_hour = EEPROM.read(2);
    int1.stop.tm_min = EEPROM.read(3);
    int2.start.tm_hour = EEPROM.read(4);
    int2.start.tm_min = EEPROM.read(5);
    int2.stop.tm_hour = EEPROM.read(6);
    int2.stop.tm_min = EEPROM.read(7);
    int3.start.tm_hour = EEPROM.read(8);
    int3.start.tm_min = EEPROM.read(9);
    int3.stop.tm_hour = EEPROM.read(10);
    int3.stop.tm_min = EEPROM.read(11);
    Serial.printf("[INFO] intervalls initialized to\n(1) %d:%d - %d:%d\n(2) %d:%d - %d:%d\n(3) %d:%d - %d:%d\n",
        int1.start.tm_hour,int1.start.tm_min,int1.stop.tm_hour,int1.stop.tm_min,
        int2.start.tm_hour,int2.start.tm_min,int2.stop.tm_hour,int2.stop.tm_min,
        int3.start.tm_hour,int3.start.tm_min,int3.stop.tm_hour,int3.stop.tm_min);
    
    //Initialize loop timer:
    loopTimer = timerBegin(0, 80, true); // initialize timer0
    timerAttachInterrupt(loopTimer, &onTimer, true);
    timerAlarmWrite(loopTimer, LOOP_PERIOD, true);
    timerAlarmEnable(loopTimer);

    //Turn off WIFI:
    //disableWifi();

    Serial.printf("[INFO] ESP32 device has been set up!\n");
}

// BrunnenSensors sensors();
// sensors.begin();
//now = sensors.requestSample(); 
//sensors.readValues();
// flow = sensors.getWaterflow();
struct tm timeinfo; // struct holding time-object
bool manualOverride = false;
unsigned long flowSum = 0; 
void loop() {
    /* relais pin test:
    digitalWrite(RELAIS, HIGH);
    digitalWrite(LED_YELLOW, !digitalRead(LED_YELLOW));
    delay(1000);
    digitalWrite(RELAIS, LOW);
    digitalWrite(LED_YELLOW, !digitalRead(LED_YELLOW));
    delay(1000); */
    
    //Periodic Meassurements:
    //if (false) {
    if (loopEntry) { // check for loop entry
        //Loop entry:
        digitalWrite(LED_BLUE, HIGH);
        uint64_t now = switchOnWaterLevelSensor();
        //if (DEBUG) Serial.printf("---\nLoop entry at %s\n", getTimeString());

        //Mutex reset of loop entry
        portENTER_CRITICAL(&timerMux);
        loopEntry = false;
        portEXIT_CRITICAL(&timerMux);

        //Read sensors:
        flow = readWaterflow();
        pressure = readWaterPressure();
        while ( millis() < now+360 ); // wait for water level sensor to wake up
        level = readWaterLevel();
        if (DEBUG) {
            Serial.printf("water flow: %d Edges\n", flow);
            Serial.printf("water pressure: %d V\n", pressure);
            Serial.printf("water level: %d V\n", level);
        }
        
        //Write data to file
        writeSDCard(flow, pressure, level);

        //Check for midnight
        if (getLocalTime(&timeinfo)) {
            if (timeinfo.tm_hour == 23 && timeinfo.tm_min == 59 && timeinfo.tm_sec == 59) {
                sendMail();
            }
        } else {
            Serial.println("Failed to obtain time");
        }

        
        //Check time switch for relais:
        if (!manualOverride) {
            if (checkRelaisInterval(timeinfo, int1, int2, int3)) {
                digitalWrite(RELAIS, HIGH);
                digitalWrite(LED_YELLOW, HIGH);
            } else {
                digitalWrite(RELAIS, LOW);
                digitalWrite(LED_YELLOW, LOW);
            }
        }
        

        //Loop exit:
        digitalWrite(LED_BLUE, LOW);
    }

    
   


    //Button Press Handling:
    if (buttonPressed) {
        //Disable interrupt
        detachInterrupt(BUTTON);
        buttonPressed = false;
        
        //Start timer for long press
        longpressTimer = timerBegin(1, 80, true); // initialize timer0
        timerAttachInterrupt(longpressTimer, &longpressed, true);
        timerAlarmWrite(longpressTimer, LONGPRESS_DELAY, true);
        timerAlarmEnable(longpressTimer);

        //Initialize server
        // -> turn on when server is running LED_GREEN
        Serial.printf("enable wifi\n");

        manualOverride = !manualOverride;

        //Switch relais
        digitalWrite(RELAIS, !digitalRead(RELAIS));
        digitalWrite(LED_YELLOW, !digitalRead(LED_YELLOW));
        
    }
    if (longpress) {
        stopTimer(longpressTimer);
        longpress = false;
        attachInterrupt(BUTTON, buttonISR, RISING); // interrupt on change
    }

}
