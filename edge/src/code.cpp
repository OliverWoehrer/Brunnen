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
#include "dt.h"
#include "hw.h"
#include "ui.h"
#include "gw.h"

#define BAUD_RATE 115200
//===============================================================================================
// SCHEDULED TASKS
//===============================================================================================
#define TRANSMISSION_PERIODE (1000 * 60 * 60) // loop period in ms, once every hour
#define SYNCHRONIZATION_PERIOD (1000 * 20)
#define SERVICE_PERIOD (1000 * 60) // loop period in ms, once per minute
#define MEASUREMENT_PERIOD 1000 // loop period in ms, once per second
#define DEFAULT_STACK_SIZE (1024 * 4) // stack size in bytes

TaskHandle_t buttonHandlerHandle = NULL;
TaskHandle_t heapWatcherHandle = NULL;
TaskHandle_t syncLoopHandle = NULL;
SemaphoreHandle_t dataFileSemaphore = NULL;
unsigned int syncLoopParameters = SYNCHRONIZATION_PERIOD;

/* TODO: run plan to implement sync
FileSystem (+)
    use semaphore (not notifications) to mutex data file access
Log
    use semaphore (not notifications) to mutex data file access
measurementLoop (+)
    write data to data file
    use semaphore (not notifications) to mutex data file access
synchronizationLoop
    runs every 10 sec
    change loop period via pointer
    synchronizationLoop calls synchronizationTask
synchronizationTask
    use semaphore for file access
    read at most N lines
    give semaphore after file access
    make request
    clean processed lines on success     
    process response    
        update settings
    update loop periode
*/

/* TODO: proposed structure:
Main (code.cpp)
Peripherals
    FileManager
        FileManager
        SDFileManager
        SPIFFSFileManager
    Inputs
        Button
        Analog
        Digital
    Outputs
        Digital
    Pref
    (Time)
    (Wlan)
Modules
    Button      Inputs::Button
    ErrorLed    Outputs::Digital errorLed
    Sensors     Outputs::Digital led, Inputs::Analog waterPressure, Inputs::Analog waterLevel, Inputs::Digital waterFlow
    Pump        Outputs::Digital relais, Outputs::Digital led
    LogFile     FileManagerSPIFFS logFile
                -> store semaphore handle in object
    DataFile    FileManagerSD dataFile
                -> store semaphore handle in object
    Config      Pref pref
                -> store semaphore handle in object
    Time        Time time
    Wlan        Wlan wlan
    UserInterface   Outputs::Digital led
    Gateway
Tasks
    ButtonHandler       Button, UserInterface, Pump
    synchronizationTask Gateway, 
    synchronizationLoop .
    serviceTask         
    measurementTask     .
Connection:
    Button      ButtonHandler
    ErrorLed    <shared>: all
    Sensors     <shared>: measurementTask, serviceTask
    Pump        <shared>: ButtonHandler, serviceTask
    LogFile     <shared> all
    DataFile    <shared> syncTask, measurementTask
*/

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
            Hardware::togglePump();
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
    // Build String for Request URL:
    struct tm timeinfo = DataTime::loadTimeinfo();
    char currentDate[11]; // Format: "YYYY-MM-DD"
    sprintf(currentDate, "%04d-%02d-%02d",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);

    // Send Request to OpenMeteoAPI:
    DataTime::logInfoMsg("Requesting weather data from OpenMeteo API.");
    bool isConnected = DataTime::isWlanConnected();
    int ret = DataTime::connectWlan(); // (re-)connect to wifi
    if (ret == SUCCESS) {
        ret = Gateway::requestWeatherData(currentDate, currentDate);
        if (ret == SUCCESS) { // got data successfully
            int rain = Gateway::getWeatherData("precipitation");
            char weatherTxt[30];
            sprintf(weatherTxt,"Rain today is %d mm.",rain);
            DataTime::logInfoMsg(weatherTxt);

            if (rain >= Hardware::getPumpOperatingLevel()) {
                DataTime::logInfoMsg("Too much rain, pause pump operation.");
                Hardware::pauseScheduledPumpOperation();
            } else {
                DataTime::logInfoMsg("Too little rain, resume pump operation.");
                Hardware::resumeScheduledPumpOperation();
            }
        } else { // failed to request data
            // Write Error Message to Log:
            const char* errorMsg = Gateway::getWeatherResponse(); // response is error on fail
            DataTime::logErrorMsg("Failed to request weather data from OpenMeteo API.");
            DataTime::logInfoMsg(errorMsg);

            // Unknown Weather Report; Resume Scheduled Operation Just in Case:
            Hardware::resumeScheduledPumpOperation();
        }
    } else {
        DataTime::logErrorMsg("Cannot request weather data without network connection.");
    }
    if (!isConnected) DataTime::disconnectWlan(); // disconnect wifi again if not connected before

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
    // Append Info Message to Gateway Text:
    int rain = Gateway::getWeatherData("precipitation");
    char weatherTxt[60];
    sprintf(weatherTxt,"To my knowledge it is about to rain %d mm today. ",rain);
    Gateway::addInfoText(weatherTxt);

    // Check the Amount of Predicted Precipitation:
    if (rain >= Hardware::getPumpOperatingLevel()) {
        Gateway::addInfoText("That's enough rain, I will pause pump operation for now. ");
    } else {
        Gateway::addInfoText("That's too little rain, I will resume pump operation for now. ");
    }

    //Build Text to Send:
    const unsigned char jobLength = DataTime::loadJobLength();
    char jobTxt[50];
    sprintf(jobTxt,"With %d data file(s) to attache, namly\r\n",jobLength+1);
    Gateway::addInfoText(jobTxt);

    // Attach File(s) to Object:
    Gateway::addData(Hardware::loadActiveDataFileName());
    for (unsigned int i=0; i < jobLength; i++) {
        const char* jobName = DataTime::loadJob(i);
        int freeMemory = Gateway::addData(jobName);
        Serial.printf("EMail::attachFile(%s); with %d bytes left.\r\n",jobName,freeMemory);
        if (freeMemory <= 0) break;
    }

    // Check Free Memory (File System):
    DataTime::checkLogFile(1000000); // check free space for log file

    // Send Data:
    DataTime::logInfoMsg("Sending Mail.");
    bool isConnected = DataTime::isWlanConnected();
    int ret = DataTime::connectWlan(); // (re-)connect to wifi
    if (ret == SUCCESS) {
        ret = Gateway::sendData();
        if (ret == SUCCESS) { // delete old file after successful send
            Hardware::deleteActiveDataFile();
            for (unsigned int i=0; i < jobLength; i++) {
                const char* jobName = DataTime::loadJob(i);
                Hardware::setActiveDataFile(jobName);
                Hardware::deleteActiveDataFile();
                DataTime::deleteJob(i);
            }
            DataTime::saveJobLength(0);
        } else { // error occured while sending mail
            const char* errorMsg = Gateway::getErrorMsg();
            DataTime::logErrorMsg("Failed to send Email, adding file to job list.");
            DataTime::logInfoMsg(errorMsg);
            Gateway::clearData();
            const char* fName = Hardware::loadActiveDataFileName();
            DataTime::saveJob(jobLength, fName);
            DataTime::saveJobLength(jobLength+1);
        }
    } else {
        DataTime::logErrorMsg("Cannot send mail without network connection.");
    }
    if (!isConnected) DataTime::disconnectWlan(); // disconnect wifi again if not connected before
    
    //Set up new data file:
    struct tm timeinfo = DataTime::loadTimeinfo();
    char fileName[FILE_NAME_LENGTH]; // Format: "/data_YYYY-MM-DD.txt"
    sprintf(fileName, "/data_%04d-%02d-%02d.txt",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);
    if (Hardware::setActiveDataFile(fileName)) {
        DataTime::logErrorMsg("Failed to initialize file system (SD-Card)");
        Hardware::setErrorLed(HIGH);
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
    Serial.printf("Created heapWatcherTask{periode %u sec} on Core %d\r\n",xFrequency/1000,xPortGetCoreID());
    size_t lastFreeHeapSize = -1;
    
    // Periodic Loop:
    while (1) {
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
        struct tm timeinfo = DataTime::loadTimeinfo();

        if (timeinfo.tm_hour == 0) { // check heap size once a day
            size_t freeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
            if(freeHeapSize < lastFreeHeapSize) {
                char heapInfoTxt[60];
                sprintf(heapInfoTxt,"Largest region currently free in heap at %u bytes.",freeHeapSize);
                DataTime::logDebugMsg(heapInfoTxt);
                lastFreeHeapSize = freeHeapSize;
            }
        }

        if (timeinfo.tm_hour == 0) { // request weather data once at/after midnight
            // Wait to Get Notified for Free Heap:
            ulTaskNotifyTake(pdTRUE, (180*1000)/portTICK_PERIOD_MS); // blocking wait for notification up to 180 seconds
            
            // Create Task for Requesting Weather:
            xTaskCreate(requestWeatherDataTask,"requestWeatherDataTask",2*DEFAULT_STACK_SIZE,NULL,0,NULL); // priority 0 (same as idle task) to prevent idle task from starvation
        }

        if (timeinfo.tm_hour == 0) { // send mail once at/after midnight
            // Wait to Get Notified for Free Heap:
            ulTaskNotifyTake(pdTRUE, (180*1000)/portTICK_PERIOD_MS); // blocking wait for notification up to 180 seconds
            
            // Create Task for Sending Mail:
            xTaskCreate(sendMailTask,"sendMailTask",2*DEFAULT_STACK_SIZE,NULL,0,NULL); // priority 0 (same as idle task) to prevent idle task from starvation
        }
    }
}

void synchronizationTask(void* parameter) {
    // Read Data File (Mutex):
    Hardware::sensor_data_t sensorData[20];
    xSemaphoreTake(dataFileSemaphore, (2*1000)/portTICK_PERIOD_MS); // blocking wait
    int used = Hardware::exportDataFile(sensorData, 20);
    xSemaphoreGive(dataFileSemaphore); // give back mutex semaphore
    if(used < 0) { // negative on error
        DataTime::logErrorMsg("Failed to export sensor values");
        xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
        vTaskDelete(NULL); // delete task when done, don't forget this!
    }

    // Send JSON to Backend:
    if(!Gateway::insertData(sensorData, used)) {
        DataTime::logErrorMsg("Failed to insert data");
        xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
        vTaskDelete(NULL); // delete task when done, don't forget this!
    }

    // Connect to WiFi:
    bool isConnected = DataTime::isWlanConnected();
    if(DataTime::connectWlan()) {
        DataTime::logErrorMsg("Cannot connect to network.");
        xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
        vTaskDelete(NULL); // delete task when done, don't forget this!
    }

    // Send Sync Request:
    if(Gateway::synchronize()) {
        const char* errorMsg = Gateway::getResponse(); // response is error on fail
        DataTime::logErrorMsg("Failed to synchronize with TreeAPI.");
        DataTime::logInfoMsg(errorMsg);
        if(!isConnected) { DataTime::disconnectWlan(); }
        xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
        vTaskDelete(NULL); // delete task when done, don't forget this!
    }
    
    // Shrink Data File:
    // TODO: change shrinkLogFile() to work with 'used' as line numbers, not bytes
    if(DataTime::shrinkLogFile(used)) {
        DataTime::logErrorMsg("Failed to shrink log file.");
        if(!isConnected) { DataTime::disconnectWlan(); }
        xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
        vTaskDelete(NULL); // delete task when done, don't forget this!
    }

    // Update Intervals:
    Hardware::pump_intervall_t intervals[MAX_INTERVALLS];
    if(Gateway::getIntervals(intervals)) {
        const char* errorMsg = Gateway::getResponse(); // response is error on fail
        DataTime::logInfoMsg(errorMsg);
        DataTime::logErrorMsg("Failed to parse intervals.");
        if(!isConnected) { DataTime::disconnectWlan(); }
        Gateway::clear();
        xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
        vTaskDelete(NULL); // delete task when done, don't forget this!
    }
    Hardware::setPumpIntervals(intervals);
    DataTime::savePumpIntervals(intervals);

    // Update Sync Periods:
    Gateway::api_sync_t sync;
    if(Gateway::getSync(&sync)) {
        const char* errorMsg = Gateway::getResponse(); // response is error on fail
        DataTime::logInfoMsg(errorMsg);
        DataTime::logErrorMsg("Failed to parse sync.");
        if(!isConnected) { DataTime::disconnectWlan(); }
        Gateway::clear();
        xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
        vTaskDelete(NULL); // delete task when done, don't forget this!
    }
    // TODO: store sync into preferences
    syncLoopParameters = sync.periods[sync.mode] * 1000; // sync loop period in milliseconds

    // Clear Gateway:
    Gateway::clear();

    // Disconnect Wifi Again:
    if(!isConnected) { // if not connected before
        DataTime::disconnectWlan();
    }

    // Exit This Task:
    xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

void synchronizationLoop(void* parameter) {
    // Initalize Task:
    TickType_t xFrequency = SYNCHRONIZATION_PERIOD / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    Serial.printf("Created synchronizationLoop{periode %u sec} on Core %d\r\n",xFrequency/1000,xPortGetCoreID());
    
    // Periodic Loop:
    while (1) {
        // Start Sync Task:
        ulTaskNotifyTake(pdTRUE, (180*1000)/portTICK_PERIOD_MS); // blocking wait for notification up to 180 seconds
        xTaskCreate(synchronizationTask,"synchronizationTask",2*DEFAULT_STACK_SIZE,NULL,0,NULL); // priority 0 (same as idle task) to prevent idle task from starvation

        // Set Sync Periode:
        unsigned int syncPeriod = *(unsigned int*)parameter;
        xFrequency = syncPeriod / portTICK_PERIOD_MS;
        Serial.printf("synchronizationLoop{periode %u sec}\r\n",xFrequency/1000);    
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking

        // TODO: remove for loop
        vTaskDelete(NULL); // delete task when done, don't forget this!
    }
}

/**
 * This function implements the serviceTask and periodically checks the pump intervals (is it
 * time to switch the pump on/off). It is implemented as a periodic loop with a periode length
 * defined by SERVICE_PERIOD (currently once per minute). This means the granularity of
 * intervalls is minutes.
 * @param parameter Pointer to a parameter struct (unused for now)
 * @note Loops every minute
 */
void serviceTask(void* parameter) {
    // Initalize Task:
    const TickType_t xFrequency = SERVICE_PERIOD / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    Serial.printf("Created serviceTask{periode %u sec} on Core %d\r\n",xFrequency/1000,xPortGetCoreID());
    int scheduledState = 0; // scheduled state of the pump, can differ from actual state
    
    // Periodic Loop:
    while (1) {
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
        
        // Check Scheduled Pump Intervals:
        struct tm timeinfo = DataTime::loadTimeinfo();
        int newScheduledState = Hardware::getScheduledPumpState(timeinfo);
        if (scheduledState != newScheduledState) { // check if scheduled pump state changed
            scheduledState = newScheduledState;
            Hardware::switchPump(newScheduledState); // update pump on change
        }
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
    Serial.printf("Created measurementTask{periode %u sec} on Core %d\r\n",xFrequency/1000,xPortGetCoreID());
    
    // Periodic Loop:
    while (1) {
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
        // TODO: check if tasks can take their notification multiple times (similar to a semaphore being available not only once)
        xSemaphoreTake(dataFileSemaphore, (2*1000)/portTICK_PERIOD_MS); // blocking wait
        Hardware::sampleSensors(DataTime::timeToString());
        xSemaphoreGive(dataFileSemaphore); // give back mutex semaphore
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

    Serial.begin(BAUD_RATE);

    // Initialize data&time module:
    if (DataTime::init()) {
        Serial.printf("Failed to initialize DataTime module!\r\n");
        Hardware::setErrorLed(HIGH);
        return;
    }
    
    // Create Button Handler Task:
    xTaskCreate(buttonHandlerTask, "buttonHandlerTask",DEFAULT_STACK_SIZE,NULL,1,&buttonHandlerHandle);
    configASSERT(buttonHandlerHandle);

    // Build Data File Name:
    struct tm timeinfo = DataTime::loadTimeinfo();
    char fileName[FILE_NAME_LENGTH]; // Format: "/data_YYYY-MM-DD.txt"
    sprintf(fileName, "/data_%04d-%02d-%02d.txt",timeinfo.tm_year+1900,timeinfo.tm_mon+1,timeinfo.tm_mday);

    delay(1000); // wait

    // Initialize system hardware:
    if(Hardware::init(&buttonHandlerHandle,fileName)) {
        Serial.printf("Failed to initialize hardware module!\r\n");
        Hardware::setErrorLed(HIGH);
        // return; disable for software testing on missing hardware
    }

    // Read Preferences from Flash Memory:
    Serial.printf("[INFO] Intervals:\r\n");
    for(unsigned int i = 0; i < MAX_INTERVALLS; i++) {
        //Read out preferences from flash:
        tm start = DataTime::loadStartTime(i);
        tm stop = DataTime::loadStopTime(i);
        unsigned char wday = DataTime::loadWeekDay(i);

        //Initialize interval:
        Hardware::pump_intervall_t interval = {.start = start, .stop = stop, .wday = wday};
        Hardware::setPumpInterval(interval, i);
        Serial.printf("(%d) %02d:%02d - %02d:%02d {%X}\r\n",i,interval.start.tm_hour,interval.start.tm_min,interval.stop.tm_hour,interval.stop.tm_min, interval.wday);
    }

    // Read Rain Threshold Level from Flash Memory:
    int rainThreshold = DataTime::loadRainThresholdLevel();
    Hardware::setPumpOperatingLevel(rainThreshold);

    // TODO: load sync from preferences

    // Initialize e-mail client:
    const char* mailPassword = DataTime::loadPassword();
    if (Gateway::init(SMTP_SERVER,SMTP_SERVER_PORT,EMAIL_SENDER_ACCOUNT,mailPassword)) { // error connecting to SD card
        Serial.printf("Failed to setup gateway.\r\n");
        Hardware::setErrorLed(HIGH);
        return;
    }

    // Initialize Web Server User Interface:
    if (UserInterface::init()) {
        Serial.printf("Failed to init ui.\r\n");
        Hardware::setErrorLed(HIGH);
        return;
    }
    DataTime::connectWlan();
    UserInterface::enableInterface();
    Hardware::setUILed(HIGH);

    // Create and Start Scheduled Tasks:
    // TODO: xTaskCreate(measurementTask,"measurementTask",DEFAULT_STACK_SIZE,NULL,1,NULL);
    // TODO: xTaskCreate(serviceTask,"serviceTask",DEFAULT_STACK_SIZE,NULL,1,NULL);
    xTaskCreate(synchronizationLoop,"syncLoop",DEFAULT_STACK_SIZE,&syncLoopParameters,1,&syncLoopHandle);
    xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
    dataFileSemaphore = xSemaphoreCreateMutex(); // mutex for data file
    if(dataFileSemaphore == NULL) {
        DataTime::logInfoMsg("Not enough heap to use data file semaphore.");
        Hardware::setErrorLed(HIGH);
        return;
    }

    // Finish Setup:
    DataTime::logInfoMsg("Device setup.");
    vTaskDelete(NULL); // delete this task to prevent busy idling in empty loop()
}

void loop() {}