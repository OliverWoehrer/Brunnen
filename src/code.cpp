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
// SCHEDULED TASKS
//===============================================================================================
#define TRANSMISSION_PERIODE (1000 * 60 * 60) // loop period in ms
#define SERVICE_PERIOD (1000 * 60) // loop period in ms
#define MEASUREMENT_PERIOD 1000 // loop period in ms

TaskHandle_t buttonHandlerHandle = NULL;

void buttonHandlerTask(void* parameter) {
    Serial.printf("[DEBUG] Created buttonHandlerTask\r\n");
    while(1) {
        vTaskSuspend(NULL); // suspend this task, resume from button ISR
        Hardware::button_indicator_t btnIndicator = Hardware::getButtonIndicator();

        //Handle Short Button Press:
        if (btnIndicator.shortPressed) {
            Serial.printf("[DEBUG] button short pressed\r\n");
            Hardware::resetButtonFlags();
            DataTime::logInfoMsg("toggle user interface");
            if (UserInterface::isEnabled()) { // interface is enabled, disable again
                UserInterface::disableInterface();
                DataTime::disconnectWlan();
                Hardware::setUILed(LOW);
            } else { // interface is disabled, enable again
                int cwSuccess = DataTime::connectWlan();
                if (cwSuccess != SUCCESS) { // re-connect failed
                    DataTime::logErrorMsg("Failed to connect WiFi.");
                    Hardware::setErrorLed(HIGH);
                    continue;
                }
                UserInterface::enableInterface();
                Hardware::setUILed(HIGH);   
            }
        }


        //Handle Long Button Press:
        if (btnIndicator.longPressed) {
            Serial.printf("[DEBUG] button long pressed\r\n");
            Hardware::resetButtonFlags();
            DataTime::logInfoMsg("toggle relais and operating mode");
            Hardware::manuallyToggleWaterPump();
        }
    }

    // Delete Task When Done:
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

void requestWeatherDataTask(void* parameter) {
    Serial.printf("[DEBUG] Created requestWeatherDataTask\r\n");

    // Build String for Request URL:
    struct tm timeinfo = DataTime::loadTimeinfo();
    char currentDate[11]; // Format: "YYYY-MM-DD"
    sprintf(currentDate, "%04d-%02d-%02d",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);
    
    // Send Request to OpenMeteoAPI:
    char weatherTxt[40];
    if (Gateway::requestWeatherData(currentDate, currentDate)) { // failed to request data
        // Write Error Message to Log:
        const char* errorMsg = Gateway::getWeatherResponse(); // response is error on fail
        DataTime::logErrorMsg("Failed to request weather data from OpenMeteo API.");
        DataTime::logInfoMsg(errorMsg);
        
        // Append Info Message to Gateway Text:
        Gateway::addInfoText("Failed to request weather data. ");

        // Unknown Weather Report; Resume Scheduled Operation Just in Case:
        Hardware::resumeScheduledPumpOperation();
    } else { // got data successfully
        // Get Amount of Rain:
        int rain = Gateway::getWeatherData("precipitation");

        // Append Info Message to Gateway Text:
        sprintf(weatherTxt,"It is about to rain %d mm today. ",rain);
        Gateway::addInfoText(weatherTxt);

        // Check the Amount of Predicted Precipitation:
        if (rain > 2) {
            Serial.printf("Too much rain, pause pump operation\r\n");
            Hardware::pauseScheduledPumpOperation();
        } else {
            Serial.printf("Too little rain, resume pump operation\r\n");
            Hardware::resumeScheduledPumpOperation();
        } 
    }
    

    // Delete Task When Done:
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

void sendMailTask(void* parameter) {
    Serial.printf("[DEBUG] Created sendMailTask\r\n");

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

    // Check Free Memory (File System and Heap):
    DataTime::checkLogFile(1000000); // check free space for log file
    size_t freeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
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

    // Delete Task When Done:
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

void transmissionTask(void* pvParameters) {
    // Initalize Task:
    const TickType_t xFrequency = TRANSMISSION_PERIODE / portTICK_PERIOD_MS;
    BaseType_t xWasDelayed;
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    Serial.printf("[DEBUG] Created transmissionTask{periode %u sec}\r\n", xFrequency/1000);
    
    // Periodic Loop:
    while (1) {
        xWasDelayed = xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
        //Check for midnight:
        struct tm timeinfo = DataTime::loadTimeinfo();
        if (timeinfo.tm_hour == 0) { //(timeinfo.tm_hour == 0 && timeinfo.tm_min == 10 && timeinfo.tm_sec == 00)
            // Get Connection State of WiFi:
            bool isConnected = DataTime::isWlanConnected();

            // (Re-)Connect to WiFi:
            DataTime::connectWlan();
            DataTime::logInfoMsg("sending Email");

            // Create and Start Tasks for Transmission:
            xTaskCreate(requestWeatherDataTask,"requestWeatherDataTask",2048,NULL,1,NULL);
            xTaskCreate(sendMailTask,"sendMailTask",2048,NULL,1,NULL);

            // Reset Connection WiFi Connection:
            if (!isConnected) DataTime::disconnectWlan(); // disconnect wifi again if not connected before
        }
    }
}

void serviceTask(void* pvParameters) {
    // Initalize Task:
    const TickType_t xFrequency = SERVICE_PERIOD / portTICK_PERIOD_MS;
    BaseType_t xWasDelayed;
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    Serial.printf("[DEBUG] Created serviceTask{periode %u sec}\r\n", xFrequency/1000);
    
    // Periodic Loop:
    while (1) {
        xWasDelayed = xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
        // Check Scheduled Pump Intervals:
        struct tm timeinfo = DataTime::loadTimeinfo();
        Hardware::managePumpIntervals(timeinfo);
    }
}

void measurementTask(void* pvParameter) {
    // Initalize Task:
    const TickType_t xFrequency = MEASUREMENT_PERIOD / portTICK_PERIOD_MS;
    BaseType_t xWasDelayed;
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    Serial.printf("[DEBUG] Created measurementTask{periode %u sec}\r\n", xFrequency/1000);
    
    // Periodic Loop:
    while (1) {
        xWasDelayed = xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
        // Indicate Loop Entry:
        Hardware::setIndexLed(HIGH);

        // Read sensors:
        Hardware::readSensorValues();
        
        // Write data to file:
        char dataString[TIME_STRING_LENGTH+1+VALUE_STRING_LENGTH+3];
        sprintf(dataString,"%s,%s\r\n",DataTime::timeToString(),Hardware::sensorValuesToString());
        DataTime::writeToDataFile(dataString);

        // Indicate Loop Exit:
        Hardware::setIndexLed(LOW);
    }
}

//===============================================================================================
// MAIN PROGRAMM
//===============================================================================================
void setup() {
    delay(1000); // wait for hardware on PCB to wake up

    Hardware::setIndexLed(HIGH);
    Serial.begin(BAUD_RATE);

    // Create and Start Scheduled Tasks:
    xTaskCreate(measurementTask,"measurementTask",2048*4,NULL,1,NULL);
    xTaskCreate(serviceTask,"serviceTask",2048*4,NULL,2,NULL);
    xTaskCreate(transmissionTask,"transmissionTask",2048*4,NULL,3,NULL);
    xTaskCreate(buttonHandlerTask, "buttonHandlerTask",2048*4,NULL,1,&buttonHandlerHandle);
    configASSERT(buttonHandlerHandle);

    //Initialize system hardware:
    if(Hardware::init(&buttonHandlerHandle)) {
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

    

    Hardware::setIndexLed(LOW);
    delay(1000);
}

void loop() {}