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
#define TRANSMISSION_PERIODE (1000 * 60 * 60) // loop period in ms, once every hour
#define SERVICE_PERIOD (1000 * 60) // loop period in ms, once per minute
#define MEASUREMENT_PERIOD 1000 // loop period in ms, once per second
#define DEFAULT_STACK_SIZE (1024 * 4) // stack size in bytes

TaskHandle_t buttonHandlerHandle = NULL;
TaskHandle_t heapWatcherHandle = NULL;

/**
 * This function implements the buttonHandlerTask with an (blocking) infinite loop and gets
 * resumed as soon as the button is either short or long pressed. This means the task suspends
 * itself (e.g. blocking wait) until it gets resumed by the button handler (periodicButton) after
 * a short/long press was detected. When resumed all button flags indicating a short/long press
 * are checked. Altough this function is technically a loop it only runs once each time the button
 * is pressed. This means the task is reused again (instead of creating/deleting a new task each
 * time) and also means better performance in ISR because just resuming a task is much faster then
 * creating a new one. 
 * @param parameter Pointer to a parameter struct (unused for now)
 */
void buttonHandlerTask(void* parameter) {
    Serial.printf("[DEBUG] Created buttonHandlerTask on Core %d\r\n",xPortGetCoreID());
    while(1) {
        vTaskSuspend(NULL); // suspend this task, resume from button ISR
        Hardware::button_indicator_t btnIndicator = Hardware::getButtonIndicator();

        //Handle Short Button Press:
        if (btnIndicator.shortPressed) {
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
            Hardware::resetButtonFlags();
            DataTime::logInfoMsg("toggle relais and operating mode");
            Hardware::manuallyToggleWaterPump();
        }
    }

    // Delete Task When Done:
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

/**
 * This function implements the requestWeatherDataTask and requests weather data from the well
 * known OpenWeatherAPI for the location of the coordinates defined in gw.h. The task is created
 * in the function implementing the heapWatcherTask and notifies the heapWatcherTask that it has
 * finished before it gets deleted again afterwards.
 * @param parameter Pointer to a parameter struct (unused for now)
 * @note This function allocates a lot of heap memory for web requests, see heapWatcherTask for
 * details.
 */
void requestWeatherDataTask(void* parameter) {
    // Log Task Message:
    char debugTxt[50];
    sprintf(debugTxt,"Created requestWeatherDataTask on Core %d",xPortGetCoreID());
    DataTime::logDebugMsg(debugTxt);
    
    // Build String for Request URL:
    struct tm timeinfo = DataTime::loadTimeinfo();
    char currentDate[11]; // Format: "YYYY-MM-DD"
    sprintf(currentDate, "%04d-%02d-%02d",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);
    
    // Send Request to OpenMeteoAPI:
    DataTime::logInfoMsg("Requesting weather data from OpenMeteo API.");
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
        char weatherTxt[60];
        sprintf(weatherTxt,"To my knowledge it is about to rain %d mm today. ",rain);
        Gateway::addInfoText(weatherTxt);

        // Check the Amount of Predicted Precipitation:
        if (rain >= Hardware::getRainThresholdLevel()) {
            Gateway::addInfoText("That's enough rain, I will pause pump operation for today. ");
            DataTime::logInfoMsg("Too much rain, pause pump operation.");
            Hardware::pauseScheduledPumpOperation();
        } else {
            Gateway::addInfoText("That's too little rain, I will resume pump operation for today. ");
            DataTime::logInfoMsg("Too little rain, resume pump operation.");
            Hardware::resumeScheduledPumpOperation();
        }
    }

    // Exit This Task:
    xTaskNotify(heapWatcherHandle,1,eSetValueWithOverwrite); // notfiy heap watcher task by setting notification value to 1
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

/**
 * This function implements the sendMailTask and sends a mail with the appropriate files (data
 * files and log file) to the recipient defined in gw.h. The task is created in the function
 * implementing the heapWatcherTask and notifies the heapWatcherTask that it has finished
 * before it gets deleted again afterwards.
 * @param parameter Pointer to a parameter struct (unused for now)
 * @note This function allocates a lot of heap memory for web requests, see heapWatcherTask for
 * details
 */
void sendMailTask(void* parameter) {
    // Log Task Message:
    char debugTxt[50];
    sprintf(debugTxt,"Created sendMailTask on Core %d",xPortGetCoreID());
    DataTime::logDebugMsg(debugTxt);

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
        if (freeMemory <= 0) break;
    }

    // Check Free Memory (File System):
    DataTime::checkLogFile(1000000); // check free space for log file

    // Send Data:
    DataTime::logInfoMsg("Sending Mail.");
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

    // Exit This Task:
    xTaskNotify(heapWatcherHandle,1,eSetValueWithOverwrite); // notfiy heap watcher task by setting notification value to 1
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

/**
 * This function implements the heapWatcherTask and watches the heap usage by starting/stoping
 * the tasks requiring a lot of heap. It is implemented as a periodic loop with a periode 
 * length defined by TRANSMISSION_PERIODE (currently once per hour). Every two hours the
 * sendMailTask gets created and a mail is send. Once a day (between 00:00 - 01:00) the
 * requestWeatherDataTask is created and new data is requested.
 * 
 * This task makes sure that no two heap-heavy task run at the same time. The heap management is
 * done by deciding which heap-heavy task may run and creating a mutex for heap usage. To
 * implement this behaviour the heapWatcherTask makes use of its nofication parameter at index 0
 * (=default), which gets either set to 1 (heap may be used by heap-heavy tasks) or set to 0
 * (heap a already used by a heap-heavy task). This is similar to a semaphore guarding shared
 * memory, see FreeRTOS documentation for details on this.
 * 
 * Before any of the heap-heavy tasks gets created this function/tasks waits at ulTaskNotifyTake()
 * blocking wait for the notifcation parameter to be set again, e.g. heap usage is allowed. To
 * prevent a deadlock here it is important that the heap-heavy tasks do notify this task before
 * they finish and get deleted, to signal that they no longer require the heap usage.
 * @param parameter Pointer to a parameter struct (unused for now)
 * @note Loops evey hour
 */
void heapWatcherTask(void* parameter) {
    // Initalize Task:
    const TickType_t xFrequency = TRANSMISSION_PERIODE / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    Serial.printf("[DEBUG] Created heapWatcherTask{periode %u sec} on Core %d\r\n",xFrequency/1000,xPortGetCoreID());
    
    // Periodic Loop:
    while (1) {
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
        //Check for midnight:
        struct tm timeinfo = DataTime::loadTimeinfo();
        if ((timeinfo.tm_hour & 0x1) == 0) { //(timeinfo.tm_hour == 0) OR ((timeinfo.tm_hour & 0x1) == 0)
            // (Re-)Connect to WiFi:
            bool isConnected = DataTime::isWlanConnected();
            DataTime::connectWlan();

            // Wait to Get Notified for Free Heap:
            ulTaskNotifyTake(pdTRUE, (120*1000)/portTICK_PERIOD_MS); // blocking wait for notification up to 120 seconds
            
            // Create Task for Requesting Weather:
            size_t freeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
            char heapInfoTxt[60];
            sprintf(heapInfoTxt,"Largest region currently free in heap at %d bytes.",freeHeapSize);
            DataTime::logDebugMsg(heapInfoTxt);
            xTaskCreate(requestWeatherDataTask,"requestWeatherDataTask",2*DEFAULT_STACK_SIZE,NULL,0,NULL); // priority 0 (same as idle task) to prevent idle task from starvation
            
            // Wait to Get Notified for Free Heap:
            ulTaskNotifyTake(pdTRUE, (120*1000)/portTICK_PERIOD_MS); // blocking wait for notification up to 120 seconds

            // Create Tasks for Sending Mail:
            freeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
            sprintf(heapInfoTxt,"Largest region currently free in heap at %d bytes.",freeHeapSize);
            DataTime::logDebugMsg(heapInfoTxt);
            xTaskCreate(sendMailTask,"sendMailTask",2*DEFAULT_STACK_SIZE,NULL,0,NULL); // priority 0 (same as idle task) to prevent idle task from starvation

            // Reset Connection WiFi Connection:
            if (!isConnected) DataTime::disconnectWlan(); // disconnect wifi again if not connected before
        }
    }
}

/**
 * This function implements the serviceTask and periodically checks the pump intervals (is it
 * time to switch the pump on/off). It is implemented as a periodic loop with a periode length
 * defined by SERVICE_PERIOD (currently once per minute). This means the granularity of
 * intervalls is minutes.
 * @param parameter Pointer to a parameter struct (unused for now)
 * @note Loops evey minute
 */
void serviceTask(void* parameter) {
    // Initalize Task:
    const TickType_t xFrequency = SERVICE_PERIOD / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    Serial.printf("[DEBUG] Created serviceTask{periode %u sec} on Core %d\r\n",xFrequency/1000,xPortGetCoreID());
    
    // Periodic Loop:
    while (1) {
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
        // Check Scheduled Pump Intervals:
        struct tm timeinfo = DataTime::loadTimeinfo();
        Hardware::managePumpIntervals(timeinfo);
    }
}

/**
 * This function implements the measurementTask and periodically measures the sensor values.
 * It is implemented as a periodic loop with a periode length defined by MEASUREMENT_PERIOD
 * (currently once per second). This means the granularity of sensor data is seconds.
 * @param parameter Pointer to a parameter struct (unused for now)
 * @note Loops evey second
 */
void measurementTask(void* parameter) {
    // Initalize Task:
    const TickType_t xFrequency = MEASUREMENT_PERIOD / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    Serial.printf("[DEBUG] Created measurementTask{periode %u sec} on Core %d\r\n",xFrequency/1000,xPortGetCoreID());
    
    // Periodic Loop:
    while (1) {
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
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
/**
 * These functions setup() and loop() are part of the arduino loopTask which gets created
 * automatically after start up. Because the loop function is empty/unused in this application
 * this task delets it self after successfull setup to prevent the loop() from busy idling
 * (looping over and doing nothing) 
 */
void setup() {
    delay(1000); // wait for hardware on PCB to wake up

    Hardware::setIndexLed(HIGH);
    Serial.begin(BAUD_RATE);

    // Create Button Handler Task:
    xTaskCreate(buttonHandlerTask, "buttonHandlerTask",DEFAULT_STACK_SIZE,NULL,1,&buttonHandlerHandle);
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

    // Create and Start Scheduled Tasks:
    xTaskCreate(measurementTask,"measurementTask",DEFAULT_STACK_SIZE,NULL,1,NULL);
    xTaskCreate(serviceTask,"serviceTask",DEFAULT_STACK_SIZE,NULL,1,NULL);
    xTaskCreate(heapWatcherTask,"heapWatcherTask",DEFAULT_STACK_SIZE,NULL,1,&heapWatcherHandle);
    xTaskNotify(heapWatcherHandle,1,eSetValueWithOverwrite); // notfiy heap watcher task by setting notification value to 1

    // Finish Setup:
    DataTime::logInfoMsg("Device setup.");
    Hardware::setIndexLed(LOW);
    vTaskDelete(NULL); // delete this task to prevent busy idling in empty loop()
}

void loop() {}