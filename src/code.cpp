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
        Serial.printf("Failed to setup gateway.\r\n");
        Hardware::setErrorLed(HIGH);
        return;
    }

    //Initialize Web Server User Interface:
    if (UserInterface::init()) {
        Serial.printf("Failed to init ui.\r\n");
        Hardware::setErrorLed(HIGH);
        return;
    }
    DataTime::connectWlan();
    UserInterface::enableInterface();
    Hardware::setUILed(HIGH);
    
    
    //Initialize loop timer:
    loopTimer = timerBegin(0, 80, true); // initialize timer0
    timerAttachInterrupt(loopTimer, &onTimer, true);
    timerAlarmWrite(loopTimer, LOOP_PERIOD, true);
    timerAlarmEnable(loopTimer);

    Hardware::setIndexLed(LOW);
    delay(1000);
}

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
        char dataString[TIME_STRING_LENGTH+1+VALUE_STRING_LENGTH+3];
        sprintf(dataString,"%s,%s\r\n",DataTime::timeToString(),Hardware::sensorValuesToString());
        DataTime::writeToDataFile(dataString);

        //Check time switch for relais:
        struct tm timeinfo = DataTime::loadTimeinfo();
        Hardware::managePumpIntervals(timeinfo);

        //Check for midnight:
        if ((timeinfo.tm_hour & 0x1) == 0 && timeinfo.tm_min == 10 && timeinfo.tm_sec == 00) { //(timeinfo.tm_hour == 0 && timeinfo.tm_min == 10 && timeinfo.tm_sec == 00)
            // (Re-)Connect to WiFi:
            bool isConnected = DataTime::isWlanConnected();
            if (!isConnected) DataTime::connectWlan(); // re-connect wifi if not previously connected
            DataTime::logInfoMsg("sending Email");
            
            // Check Free Memory (File System and Heap):
            DataTime::checkLogFile(1000000); // check free space for log file
            size_t freeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);

            // Get Weather Data from WeatherAPI:
            char currentDate[11]; // Format: "YYYY-MM-DD"
            sprintf(currentDate, "%04d-%02d-%02d",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);
            char weatherTxt[40];
            if (Gateway::requestWeatherData(currentDate, currentDate)) { // failed to request data
                const char* errorMsg = Gateway::getWeatherResponse(); // response is error on fail
                DataTime::logErrorMsg("Failed to request weather data from OpenMeteo API.");
                DataTime::logInfoMsg(errorMsg);
                sprintf(weatherTxt,"Failed to request weather data. ");
                Hardware::resumeScheduledPumpOperation();
            } else { // got data successfully
                int rain = Gateway::getWeatherData("precipitation");
                sprintf(weatherTxt,"It is about to rain %d mm today. ",rain);
                if (rain > 2) {
                    Serial.printf("Too much rain, pause pump operation\r\n");
                    Hardware::pauseScheduledPumpOperation();
                } else {
                    Serial.printf("Too little rain, resume pump operation\r\n");
                    Hardware::resumeScheduledPumpOperation();
                } 
            }
            Gateway::addInfoText(weatherTxt);

            //Build Text to Send:
            bool isNominal = Hardware::hasNominalSensorValues();
            const char* nomTxt = isNominal ? "All sensors in nominal range." : "Sensors out of nominal range.";
            Gateway::addInfoText(nomTxt);
            unsigned char jobLength = Hardware::loadJobLength();
            char jobTxt[34];
            sprintf(jobTxt," With %d data file(s) to send.\r\n",jobLength+1);
            Gateway::addInfoText(jobTxt);

            // Attach File(s) to Object:
            Gateway::addData(DataTime::loadActiveDataFileName());
            for (unsigned int i=0; i < jobLength; i++) {
                const char* jobName = Hardware::loadJob(i);
                int freeMemory = Gateway::addData(jobName);
                Serial.printf("EMail::attachFile(%s); with %d bytes left.\r\n",jobName,freeMemory);
            }

            char heapInfoTxt[40];
            sprintf(heapInfoTxt,"Largest region currently free in heap at %d bytes.\r\n",freeHeapSize);
            Gateway::addInfoText(heapInfoTxt);

            // Send Data:
            if (Gateway::sendData()) { // error occured while sending mail
                const char* errorMsg = Gateway::getErrorMsg();
                DataTime::logErrorMsg("Failed to send Email, adding file to job list.");
                DataTime::logInfoMsg(errorMsg);
                Gateway::clearData();
                const char* fName = DataTime::loadActiveDataFileName();
                Hardware::saveJob(jobLength, fName);
                Hardware::saveJobLength(jobLength+1);
            } else { // delete old file after successful send
                DataTime::deleteActiveDataFile();
                for (unsigned int i=0; i < jobLength; i++) {
                    const char* jobName = Hardware::loadJob(i);
                    DataTime::setActiveDataFile(jobName);
                    DataTime::deleteActiveDataFile();
                    Hardware::deleteJob(i);
                }
                Hardware::saveJobLength(0);
            }

            //Set up new data file:
            if (DataTime::createCurrentDataFile()) {
                DataTime::logErrorMsg("Failed to initialize file system (SD-Card)");
                Hardware::setErrorLed(HIGH);
                return;
            }

            

            if (!isConnected) DataTime::disconnectWlan(); // disconnect wifi again (if connected before)
        }
        
        //Loop exit:
        Hardware::setIndexLed(LOW);
    }


    //Handle Short Button Press:
    if (Hardware::buttonIsShortPressed()) {
        Hardware::resetButtonFlags();
        DataTime::logInfoMsg("toggle user interface");
        if (UserInterface::isEnabled()) {
            UserInterface::disableInterface();
            DataTime::disconnectWlan();
            Hardware::setUILed(LOW);
        } else {
            int cwSuccess = SUCCESS;
            if (!DataTime::isWlanConnected()) { // reenable wifi
                cwSuccess = DataTime::connectWlan();
            }
            if (cwSuccess != SUCCESS) { // re-connect failed
                DataTime::logErrorMsg("Failed to connect WiFi.");
                Hardware::setErrorLed(HIGH);
            } else {
                UserInterface::enableInterface();
                Hardware::setUILed(HIGH);   
            }
        }
    }


    //Handle Long Button Press:
    if (Hardware::buttonIsLongPressed()) {
        Hardware::resetButtonFlags();
        DataTime::logInfoMsg("toggle relais and operating mode");
        Hardware::manuallyToggleWaterPump();
    }

}