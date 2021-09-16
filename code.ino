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
#include "Arduino.h" // include basic arduino functions
#include "hw.h"
#include "dt.h"
#include "ui.h"

#define BAUD_RATE 115200

//===============================================================================================
// PERIODIC LOOP UTILITIES:
//===============================================================================================
#define LOOP_PERIOD 1000000 // loop period in us
volatile bool loopEntry; // gets set true every LOOP_PERIOD (default: 1 sec)
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; // mutex semaphore for loopEntry variable
hw_timer_t *loopTimer = NULL;
static void IRAM_ATTR onTimer(){
  portENTER_CRITICAL_ISR(&timerMux);
  loopEntry = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}


//===============================================================================================
// MAIN PROGRAMM
//===============================================================================================
void setup() {
    delay(1000); // wait for hardware on PCB to wake up
    Serial.begin(BAUD_RATE);

    //Initialize system hardware:
    Led::init();
    Sensors::init();
    Button::init();
    Relais::init();

    //Initialize Flash Memory and Read Out Preferences:
    Prefs.init();
    Serial.printf("[INFO] Intervals:\n");
    for (unsigned int i = 0; i < MAX_INTERVALLS; i++) {
        //Read out preferences from flash:
        struct tm start = Prefs.getStartTime(i);
        struct tm stop = Prefs.getStopTime(i);
        unsigned char wday = Prefs.getWeekDay(i);

        //Initialize interval:
        Relais::interval_t interval = {.start = start, .stop = stop, .wday = wday};
        Relais::setInterval(interval, i);
        Serial.printf("(%d) %d:%d - %d:%d {%u}\n",i,interval.start.tm_hour,interval.start.tm_min,interval.stop.tm_hour,interval.stop.tm_min, interval.wday);
    }

    //Setup wifi connection and system time:
    if (Wlan.init()) { // error handling
        Serial.printf("Failed to connect WiFi.\n");
        Led::turnOn(Led::RED);
        return;
    }
    if (Time.init()) {
        Serial.printf("Failed to initialize local time.\n");
        Led::turnOn(Led::RED);
        return;
    }

    //Initialize Web Server User Interface:
    Ui.init();
    Wlan.disable();

    //Set up log and storage:
    if (Log.init()) { // error with internal SPIFFS
        Serial.printf("Failed to mount SPIFFS.\n");
        Led::turnOn(Led::RED);
        return;
    }
    Log.readFile();

    if (FileSystem.init()) { // error mounting SD card
        Serial.printf("Failed to initialize file system (SD-Card).\n");
        Led::turnOn(Led::RED);
        return;
    }

    //Initialize e-mail client:
    if (Mail.init()) { // error connecting to SD card
        Serial.printf("Failed to setup mail client.\n");
        Led::turnOn(Led::RED);
        return;
    }
    
    //Initialize loop timer:
    loopTimer = timerBegin(0, 80, true); // initialize timer0
    timerAttachInterrupt(loopTimer, &onTimer, true);
    timerAlarmWrite(loopTimer, LOOP_PERIOD, true);
    timerAlarmEnable(loopTimer);

    //Log.msg(LOG::INFO, "ESP32 device has been set up!");
}


void loop() {
    //Periodic Meassurements:
    if (loopEntry) { // check for loop entry
        //Loop entry:
        portENTER_CRITICAL(&timerMux);
        loopEntry = false; // mutex reset of loop entry
        portEXIT_CRITICAL(&timerMux);
        Led::turnOn(Led::BLUE);

        //Read sensors:
        Sensors::requestValues();
        while(Sensors::hasValuesReady() == false); // wait for sensor to weak up
        bool isNominal = Sensors::readValues();
        
        //Write data to file:
        String valueString = String(Time.toString())+", "+String(Sensors::toString())+"\r\n"; 
        FileSystem.appendFile(SD, FileSystem.getFileName(), valueString.c_str());
        // Serial.print(valueString.c_str());

        //Check time switch for relais:
        struct tm timeinfo = Time.getTimeinfo();
        switch (Relais::getOpMode()) {
        case Relais::MANUAL:
            // do not toggel relais
            break;
        case Relais::SCHEDULED:
            if (Relais::checkIntervals(timeinfo))
                Relais::turnOn();
            else
                Relais::turnOff();
            break;
        case Relais::AUTOMATIC:
            if (Relais::checkIntervals(timeinfo) && Sensors::getWaterLevel() > 1100)
                Relais::turnOn();
            else
                Relais::turnOff();
            break;
        default:
            Relais::turnOff();
            break;
        }

        //Check for midnight:
        if (timeinfo.tm_hour == 23 && timeinfo.tm_min == 59 && timeinfo.tm_sec == 59) {
            Log.msg(LOG::INFO, "sending Email");
            bool wasConnected = Wlan.isConnected(); // chache if the wifi was enabled
            if (!wasConnected) {
                if (Wlan.init()) { // reenable wifi, if web server is nor enabled currently
                    Log.msg(LOG::ERROR, "Failed to connect WiFi.");
                    Led::turnOn(Led::RED);
                }
            }
            //Check nominal range of sensors, send Mail:
            String mailText = isNominal ? "Sensors out of nominal range." : "All sensors in nominal range.";
            if (Mail.send(mailText.c_str())) {
                Log.msg(LOG::ERROR, "Failed to send Email");
            }
            //Delete old file after successful send:
            FileSystem.deleteFile(SD, FileSystem.getFileName());
            if (FileSystem.init()) { // error mounting SD card
                Log.msg(LOG::ERROR, "Failed to initialize file system (SD-Card)");
                Led::turnOn(Led::RED);
                return;
            }
            //TODO: reconnect to NTp server and get time
            if (!wasConnected) {
                Wlan.disable(); // disable wifi, web server not running atm
            }
        }
        
        //Loop exit:
        Led::turnOff(Led::BLUE);
    }


    //Handle Short Button Press:
    if (Button::isShortPressed()) {
        Button::resetShortFlag();
        Log.msg(LOG::INFO, "toggle web server");
        if (!Wlan.isConnected()) { // check for wifi
            if (Wlan.init()) { // reenable wifi
                Log.msg(LOG::ERROR, "Failed to connect WiFi.");
                Led::turnOn(Led::RED);
            }
        }
        if (!Ui.toggle()) { // web interface not enabled
            Wlan.disable(); // disable wifi again
        }
        Led::toggle(Led::GREEN);
    }


    //Handle Long Button Press:
    if (Button::isLongPressed()) {
        Log.msg(LOG::INFO, "toggle relais and operating mode");
        Button::resetLongFlag();
        Relais::toggle();
    }


    //Handle Web Interface:
    Ui.handleClient();

}