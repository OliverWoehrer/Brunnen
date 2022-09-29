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
#include "em.h"

#define BAUD_RATE 115200

//===============================================================================================
// PERIODIC LOOP UTILITIES:
//===============================================================================================
#define LOOP_PERIOD 1000000 // loop period in us
volatile bool loopEntry; // gets set true every LOOP_PERIOD (default: 1 sec)
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; // mutex semaphore for loopEntry variable
hw_timer_t *loopTimer = NULL;
static void IRAM_ATTR onTimer() {
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
    Hardware::init();

    //Read Out Preferences from Flash Memory:
    Serial.printf("[INFO] Intervals:\n");
    for (unsigned int i = 0; i < MAX_INTERVALLS; i++) {
        //Read out preferences from flash:
        struct tm start = Hardware::loadStartTime(i);
        struct tm stop = Hardware::loadStopTime(i);
        unsigned char wday = Hardware::loadWeekDay(i);

        //Initialize interval:
        Hardware::Relais::interval_t interval = {.start = start, .stop = stop, .wday = wday};
        Hardware::setPumpInterval(interval, i);
        Serial.printf("(%d) %d:%d - %d:%d {%u}\n",i,interval.start.tm_hour,interval.start.tm_min,interval.stop.tm_hour,interval.stop.tm_min, interval.wday);
    }

    //Setup wifi connection and system time:
    Wlan::init();
    if (Wlan::connect()) { // error handling
        Serial.printf("Failed to connect WiFi.\n");
        Hardware::setErrorLed(HIGH);
        return;
    }
    if (Time.init()) {
        Serial.printf("Failed to initialize local time.\n");
        Hardware::setErrorLed(HIGH);
        return;
    }

    //Initialize Web Server User Interface:
    Ui.init();
    Ui.toggle(); // enable user interface
    Hardware::setUILed(HIGH);

    //Set up log and storage:
    if (Log.init()) { // error with internal SPIFFS
        Serial.printf("Failed to mount SPIFFS.\n");
        Hardware::setErrorLed(HIGH);
        return;
    }
    const char* logString = Log.readFile();
    Serial.print(" <<< LOG FILE /log.txt >>>\n");
    // Serial.print(logString);
    Serial.print(" >>> END OF LOG FILE <<<\n");

    if (FileSystem.init()) { // error mounting SD card
        Serial.printf("Failed to initialize file system (SD-Card).\n");
        Hardware::setErrorLed(HIGH);
        return;
    }

    //Initialize e-mail client:
    int mailSucess = EMail::init();
    if (mailSucess != SUCCESS) { // error connecting to SD card
        Serial.printf("Failed to setup mail client.\n");
        Hardware::setErrorLed(HIGH);
        return;
    }
    
    //Initialize loop timer:
    loopTimer = timerBegin(0, 80, true); // initialize timer0
    timerAttachInterrupt(loopTimer, &onTimer, true);
    timerAlarmWrite(loopTimer, LOOP_PERIOD, true);
    timerAlarmEnable(loopTimer);

    Log.msg(LOG::INFO, "ESP32 device has been set up!");
    delay(1000);
}

unsigned char test = 0;
void loop() {
    //Periodic Meassurements:
    if (loopEntry) {
        //Loop entry:
        portENTER_CRITICAL(&timerMux);
        loopEntry = false; // mutex reset of loop entry
        portEXIT_CRITICAL(&timerMux);
        Hardware::setIndexLed(HIGH);

        //Read sensors:
        Hardware::readSensorValues();
        
        //Write data to file:
        String valueString = String(Time.toString())+","+String(Hardware::sensorValuesToString())+"\r\n";

        //Check time switch for relais:
        struct tm timeinfo = Time.getTimeinfo();
        Hardware::managePumpIntervals(timeinfo);

        //Check for midnight:
        test++;
        if(test == 20) {
        //if (timeinfo.tm_hour == 00 && timeinfo.tm_min == 10 && timeinfo.tm_sec == 00) {
            //Check Free Space for Log File:
            if(Log.getFileSize() > 1000000) {
                Log.clearFile(); // clear log file if bigger than 1MB
                Log.msg(LOG::INFO, "Cleared Log File");
            }
            
            Log.msg(LOG::INFO, "sending Email");
            bool wasConnected = Wlan::isConnected(); // chache if the wifi was enabled
            if (!wasConnected) { // reenable wifi, if not connected
                Wlan::connect();
            }
            

            //Build mail text and send mail:
            char mailText[64] = "";
            
            bool isNominal = Hardware::hasNominalSensorValues();
            strcpy(mailText, isNominal ? "Sensors out of nominal range." : "All sensors in nominal range.");
            //TODO: EMail::addText(const char*);

            unsigned char jobLength = Hardware::loadJobLength();
            sprintf(&mailText[29]," With %d data file(s) to send.\r\n", jobLength);
            //TODO: EMail::addText(const char*);
            
            for (unsigned int i=1; i <= jobLength; i++) {
                const char* jobName = Hardware::loadJob(i);
                //TODO: EMail::attachFile(fName); <-- TODO: implement such function
            }

            int mailStatus = EMail::send("This is a test mail.");
            if (mailStatus == FAILURE) { // error occured while sending mail
                Log.msg(LOG::ERROR, "Failed to send Email, adding file to job list.");
                const char* fName = FileSystem.getFileName();
                Hardware::saveJob(jobLength+1, fName);
                Hardware::saveJobLength(jobLength+1);
            } else { // delete old file after successful send
                FileSystem.deleteFile(SD, FileSystem.getFileName());
            }
            ESP.restart(); // software reset

            //Set up new data file:
            if (FileSystem.init()) { // error mounting SD card
                Log.msg(LOG::ERROR, "Failed to initialize file system (SD-Card)");
                Hardware::setErrorLed(HIGH);
                return;
            }
            //TODO: reconnect to NTp server and get time
            if (!wasConnected) {
                Wlan::disable(); // disable wifi, web server not running atm
            }
        }
        
        //Loop exit:
        Hardware::setIndexLed(LOW);
    }


    //Handle Short Button Press:
    if (Hardware::buttonIsShortPressed()) {
        Hardware::resetButtonFlags();
        Log.msg(LOG::INFO, "toggle web server");
        if (!Wlan::isConnected()) { // check for wifi
            if (Wlan::init()) { // reenable wifi
                Log.msg(LOG::ERROR, "Failed to connect WiFi.");
                Hardware::setErrorLed(HIGH);
            }
        }
        int isEnabled = Ui.toggle();
        if (isEnabled) { // web interface now enabled
            Hardware::setUILed(HIGH);
        } else {
            Wlan::disable(); // disable wifi again
            Hardware::setUILed(LOW);
        }
    }


    //Handle Long Button Press:
    if (Hardware::buttonIsLongPressed()) {
        Hardware::resetButtonFlags();
        Log.msg(LOG::INFO, "toggle relais and operating mode");
        Hardware::toggleWaterPump();
    }


    //Handle Web Interface:
    Ui.handleClient();

}
