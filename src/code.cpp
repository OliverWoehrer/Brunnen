/**
 * @author Oliver Woehrer, 11907563
 * @date 21.03.2022
 * 
 * > > > > > BRUNNEN  < < < < <
 * System to meassure water flow and water level in well
 * 
 * 
**/
//===============================================================================================
// LIBRARIES
//===============================================================================================
#include "Arduino.h" // include basic arduino functions
#include "hw.h"
#include "dt.h"
#include "ui.h"
#include "gw.h"

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

    Hardware::setIndexLed(HIGH);
    Serial.begin(BAUD_RATE);

    //Initialize system hardware:
    if(Hardware::init()) {
        Serial.printf("Failed to initialize hardware module!\r\n");
        Hardware::setErrorLed(HIGH);
        return;
    }

    //Initialize data&time module:
    if (DataTime::init()) {
        Serial.printf("Failed to initialize DataTime module!\r\n");
        Hardware::setErrorLed(HIGH);
        return;
    }

    //Initialize e-mail client:
    if (Gateway::init()) { // error connecting to SD card
        Serial.printf("Failed to setup mail client.\n");
        Hardware::setErrorLed(HIGH);
        return;
    }

    //Initialize Web Server User Interface:
    Ui.init();
    Ui.toggle(); // enable user interface
    Hardware::setUILed(HIGH);
    
    
    //Initialize loop timer:
    loopTimer = timerBegin(0, 80, true); // initialize timer0
    timerAttachInterrupt(loopTimer, &onTimer, true);
    timerAlarmWrite(loopTimer, LOOP_PERIOD, true);
    timerAlarmEnable(loopTimer);

    DataTime::logInfoMsg("ESP32 device has been set up!");
    Hardware::setIndexLed(LOW);
    delay(1000);
}

unsigned char test = 0;
void loop() {
    return; /** TODO:*/

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
        char valueString[TIME_STRING_LENGTH+1+VALUE_STRING_LENGTH+3];
        sprintf(valueString,"%s,%s\r\n",DataTime::timeToString(),Hardware::sensorValuesToString());

        //Check time switch for relais:
        struct tm timeinfo = DataTime::loadTimeinfo();
        Hardware::managePumpIntervals(timeinfo);

        //Check for midnight:
        if (timeinfo.tm_hour == 00 && timeinfo.tm_min == 10 && timeinfo.tm_sec == 00) {
            DataTime::checkLogFile(1000000); // check free space for log file
            DataTime::reconnectWlan();
            DataTime::logInfoMsg("sending Email");

            //Build mail text and send mail:
            bool isNominal = Hardware::hasNominalSensorValues();
            const char* nomTxt = isNominal ? "All sensors in nominal range." : "Sensors out of nominal range.";
            Gateway::addInfoText(nomTxt);

            unsigned char jobLength = Hardware::loadJobLength();
            char jobTxt[34];
            sprintf(jobTxt," With %d data file(s) to send.\r\n",jobLength);
            Gateway::addInfoText(jobTxt);

            Gateway::addData(DataTime::loadActiveDataFileName());

            for (unsigned int i=0; i < jobLength; i++) {
                const char* jobName = Hardware::loadJob(i);
                test = Gateway::addData(jobName);
                Serial.printf("EMail::attachFile(%s); with %d bytes left.\r\n",jobName,test);
            }

            if (Gateway::sendData()) { // error occured while sending mail
                DataTime::logErrorMsg("Failed to send Email, adding file to job list.");
                const char* fName = DataTime::loadActiveDataFileName();
                Hardware::saveJob(jobLength, fName);
                Hardware::saveJobLength(jobLength+1);
            } else { // delete old file after successful send
                DataTime::deleteActiveDataFile();
            }

            ESP.restart(); // software reset

            /** TODO: reconnect to NTp server and get time */

            //Set up new data file:
            if (DataTime::createCurrentDataFile()) {
                DataTime::logErrorMsg("Failed to initialize file system (SD-Card)");
                Hardware::setErrorLed(HIGH);
                return;
            }
            
            DataTime::disconnectWlan();
        }
        
        //Loop exit:
        Hardware::setIndexLed(LOW);
    }


    //Handle Short Button Press:
    if (Hardware::buttonIsShortPressed()) {
        Hardware::resetButtonFlags();
        DataTime::logInfoMsg("toggle web server");
        if (!DataTime::isWlanConnected()) { // check for wifi
            int cwSuccess = DataTime::connectWlan();
            if (cwSuccess != SUCCESS) { // reenable wifi
                DataTime::logErrorMsg("Failed to connect WiFi.");
                Hardware::setErrorLed(HIGH);
            }
        }
        int isEnabled = Ui.toggle();
        if (isEnabled) { // web interface now enabled
            Hardware::setUILed(HIGH);
        } else {
            DataTime::disconnectWlan();
            Hardware::setUILed(LOW);
        }
    }


    //Handle Long Button Press:
    if (Hardware::buttonIsLongPressed()) {
        Hardware::resetButtonFlags();
        DataTime::logInfoMsg("toggle relais and operating mode");
        Hardware::toggleWaterPump();
    }


    //Handle Web Interface:
    Ui.handleClient();

}