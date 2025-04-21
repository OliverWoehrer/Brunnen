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

// Peripherals:
#include "Arduino.h" // include basic arduino functions
#include "TimeManager.h"
#include "WiFiManager.h"
#include "DataFile.h"
#include "LogFile.h"
#include "Config.h"

// Modules:
#include "Button.h"
#include "Gateway.h"
#include "UserInterface.h"
#include "Sensors.h"


//===============================================================================================
// GLOBAL SETTINGS
//===============================================================================================
#define BAUD_RATE 115200
#define DEFAULT_STACK_SIZE (1024 * 4) // stack size in bytes
#define SYNCHRONIZATION_PERIOD (1000 * 20)
#define SERVICE_PERIOD (1000 * 60) // loop period in ms, once per minute
#define MEASUREMENT_PERIOD 1000 // loop period in ms, once per second
#define BATCH_SIZE 60 // number of data points to be synced at once

//===============================================================================================
// SCHEDULED TASKS
//===============================================================================================
TaskHandle_t buttonHandlerHandle = NULL;
TaskHandle_t networkLoopHandle = NULL;
unsigned int networkLoopPeriode = SYNCHRONIZATION_PERIOD;

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
    log_d("Created buttonHandlerTask on Core %d", xPortGetCoreID());
    while(1) {
        vTaskSuspend(NULL); // suspend this task, resume from button ISR
        indicator_t btnIndicator = Button.getIndicator();

        // Short Button Press:
        if(btnIndicator == SHORT_PRESS) {
            Button.resetIndicator(); // reset manually on short press
            LogFile.log(INFO, "toggle user interface");
            if(!UserInterface.toggle()) {
                LogFile.log(ERROR, "Failed to enable interface");
            }
        }

        // Long Button Press:
        if(btnIndicator == LONG_PRESS) {
            LogFile.log(INFO, "toggle relais and operating mode");
            Pump.toggle();
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
    do { // loop allows to use "break" statement to exit earlyü

    // Send Request to OpenMeteoAPI:
    LogFile.log(INFO, "Requesting weather data from OpenMeteo API");
    if(!Gateway.requestWeatherData()) {
        std::string errorMsg = Gateway.getResponse(); // response is error on fail
        LogFile.log(ERROR, "Failed to request weather data: "+errorMsg);
        Pump.resumeSchedule(); // unknown weather, resume schedule just in case
        break;
    }

    int rain = Gateway.getPrecipitation();
    LogFile.log(INFO, "Rain today is "+std::to_string(rain)+" mm");
    if(rain >= Config.loadRainThresholdLevel()) {
        LogFile.log(INFO, "Too much rain ("+std::to_string(rain)+" mm), pause pump operation.");
        Pump.pauseSchedule();
    } else {
        LogFile.log(INFO, "Too little rain ("+std::to_string(rain)+" mm), resume pump operation");
        Pump.resumeSchedule();
    }

    } while(0); // single-iteration-loop

    // Exit This Task:
    xTaskNotify(networkLoopHandle,1,eSetValueWithOverwrite); // notfiy heap watcher task by setting notification value to 1
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
    do {
    std::string text;

    // Append Info Message to Gateway Text:
    int rain = Gateway.getPrecipitation();
    text = text + "To my knowledge it is about to rain "+std::to_string(rain)+" mm today. ";

    // Check the Amount of Predicted Precipitation:
    if (rain >= Config.loadRainThresholdLevel()) {
        text = text + "That's enough rain, I will pause pump operation for now. ";
    } else {
        text = text + "That's too little rain, I will resume pump operation for now. ";
    }

    // Attach Current Data File:
    Gateway.attachFile(DataFile.getFilename());
    
    // Attach Old Data File(s) (=Jobs):
    const size_t jobLength = Config.loadJobLength();
    for(size_t i = 0; i < jobLength; i++) {
        std::string jobName = Config.loadJob(i);
        if(!Gateway.attachFile(jobName)) {
            LogFile.log(ERROR, "Failed to attach file ("+jobName+")");
            break;
        }
    }

    // Keep LogFile From Growing:
    if(LogFile.size() > 5000) {
        LogFile.shrinkLogs(10);
    }

    // Send Data:
    LogFile.log(INFO, "Sending Mail");
    if(!Gateway.sendMail(text)) { // error occured while sending mail
        LogFile.log(ERROR, "Failed to send Email, adding file to job list.");
        std::string fName = DataFile.getFilename();
        Config.storeJob(fName.c_str(), jobLength);
        Config.storeJobLength(jobLength+1);
        break;
    }

    DataFile.remove();
    for (unsigned int i=0; i < jobLength; i++) {
        std::string jobName = Config.loadJob(i);
        DataFile.init(jobName);
        DataFile.remove();
        Config.deleteJob(i);
    }
    Config.storeJobLength(0);
    
    //Set up new data file:
    std::string filename = "/data_"+Time.toDateString()+".txt";
    if(!DataFile.init(filename)) {
        LogFile.log(ERROR, "Failed to initialize file system (SD-Card)");
        break;
    }

    } while(0);

    // Clear Gateway:
    Gateway.clear();

    // Exit This Task:
    xTaskNotify(networkLoopHandle,1,eSetValueWithOverwrite); // notfiy heap watcher task by setting notification value to 1
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

void synchronizationTask(void* parameter) {
    // Initalize Task:
    log_d("Created synchronizationTask on Core %d", xPortGetCoreID());

    do { // wrap task control in loop, use "break" to exit early

    // Connect to WiFi:
    if(!Wlan.connect()) {
        LogFile.log(ERROR, "Cannot connect to network.");
        break;
    }

    // Read Data File:
    std::vector<sensor_data_t> sensorData;
    sensorData.reserve(BATCH_SIZE);
    if(!DataFile.exportData(sensorData)) {
        LogFile.log(ERROR, "Failed to export sensor values");
        break;
    }

    // Read Log File:
    std::vector<log_message_t> logMessages;
    logMessages.reserve(20);
    if(!LogFile.exportLogs(logMessages)) {
        LogFile.log(ERROR, "Failed to export log messages");
        break;
    }

    do { // wrap Gateway in loop, use "break" to exit early

    // Append Data to JSON:
    if(!Gateway.insertData(sensorData)) {
        LogFile.log(ERROR, "Failed to insert data");
        break;
    }

    // Append Logs to JSON:
    if(!Gateway.insertLogs(logMessages)) {
        LogFile.log(ERROR, "Failed to insert logs");
        break;
    }

    // Send Sync Request:
    if(!Gateway.synchronize()) {
        LogFile.log(ERROR, "Failed to synchronize.");
        break;
    }

    // Clear Error Led: sync'ed any error logs
    LogFile.acknowledge();
    
    // Shrink Data File:
    if(!DataFile.shrinkData(sensorData.size())) {
        LogFile.log(WARNING, "Failed to shrink data file");
        break;
    }

    // Shrink Log File:
    if(!LogFile.shrinkLogs(logMessages.size())) {
        LogFile.log(WARNING, "Failed to shrink log file");
        break;
    }

    // Update Intervals:
    std::vector<interval_t> intervals;
    intervals.reserve(MAX_INTERVALLS);
    if(Gateway.getIntervals(intervals)) {
        Pump.scheduleIntervals(intervals);
        Config.storePumpIntervals(intervals);
    }

    // Update Sync Periods:
    // -> based on how much data is left to sync and what the web application asks
    sync_t sync;
    if(Gateway.getSync(&sync)) {
        unsigned int newLoopPeriode;
        size_t count = DataFile.lineCounter();
        log_d("target periode sync[%d] = %u sec", sync.mode, sync.periods[sync.mode]);
        log_d("Data points left: %u", count);        
        if(count > BATCH_SIZE) { // lots of data not synced, sync again soon
            newLoopPeriode = sync.periods[SHORT] * 1000; // sync loop period in milliseconds
        } else { // synced most of data, set according to received settings
            newLoopPeriode = sync.periods[sync.mode] * 1000;
        }
        if(newLoopPeriode != networkLoopPeriode) {
            networkLoopPeriode = newLoopPeriode;
            log_i("Updated loop periode to %u", networkLoopPeriode);
        }
    }

    } while(0); // Gateway no longer needed, clear

    // Clear Gateway:
    Gateway.clear();

    } while(0); // single-iteration-loop

    // Exit This Task:
    xTaskNotifyGive(networkLoopHandle); // notfiy sync loop task
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

/**
 * Often network interaction requires a lot of heap memory. This function implements the a
 * heap-watcher that makes sure not two heap-heavy tasks run at the same time. It is implemented
 * as a loop with a variable loop-periode length defined by TRANSMISSION_PERIODE (currently once per hour). Every two hours the
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
void networkLoop(void* parameter) {
    // Initalize Task:
    TickType_t xFrequency = SYNCHRONIZATION_PERIOD / portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    size_t lastFreeHeapSize = -1; // unsigned -1 = unsigned max value
    int inspection = -1; // hour of last inspection
    
    // Periodic Loop:
    log_d("Created networkLoop{periode %u sec} on Core %d", xFrequency/1000,xPortGetCoreID());
    while (1) {
        // Get Hour:
        tm timeinfo = Time.getTime();
        if(timeinfo.tm_hour != inspection) { // check if no recent inspection
            // Hour of Inspection:
            inspection = timeinfo.tm_hour;

            if(timeinfo.tm_hour == 0) { // inspection only at midnight
                // Check Heap Size:
                size_t freeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
                if(freeHeapSize < lastFreeHeapSize) {
                    LogFile.log(DEBUG, "Largest region currently free in heap at "+std::to_string(freeHeapSize)+" bytes.");
                    lastFreeHeapSize = freeHeapSize;
                }
                
                // Create Task for Requesting Weather:
                // ulTaskNotifyTake(pdTRUE, (180*1000)/portTICK_PERIOD_MS); // blocking wait for notification up to 180 seconds
                // xTaskCreate(requestWeatherDataTask,"requestWeatherDataTask",2*DEFAULT_STACK_SIZE,NULL,0,NULL); // priority 0 (same as idle task) to prevent idle task from starvation
            
                // Create Task for Sending Mail:
                // ulTaskNotifyTake(pdTRUE, (180*1000)/portTICK_PERIOD_MS); // blocking wait for notification up to 180 seconds
                // xTaskCreate(sendMailTask,"sendMailTask",2*DEFAULT_STACK_SIZE,NULL,0,NULL); // priority 0 (same as idle task) to prevent idle task from starvation
            }
        }

        // Check Heap Size:
        size_t freeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
        log_d("minimum free heap size: %u byte",freeHeapSize);

        // Start Sync Task:
        xTaskCreate(synchronizationTask,"synchronizationTask",2*DEFAULT_STACK_SIZE,NULL,0,NULL); // priority 0 (same as idle task) to prevent idle task from starvation
        ulTaskNotifyTake(pdTRUE, (180*1000)/portTICK_PERIOD_MS); // blocking wait for synchronizationTask to finish (get notified)

        // Set Sync Periode:
        xFrequency = networkLoopPeriode / portTICK_PERIOD_MS;
        log_d("loop periode %u sec", xFrequency/1000);
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking
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
    log_d("Created serviceTask{periode %u sec} on Core %d", xFrequency/1000,xPortGetCoreID());
    
    // Periodic Loop:
    while (1) {
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking        
        int waterlevel = Sensors.getWaterLevel();
        if(Pump.scheduler(waterlevel)) {
            log_d("Pump toggled by schedule");
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
    log_d("Created measurementTask{periode %u sec} on Core %d", xFrequency/1000,xPortGetCoreID());
    
    // Periodic Loop:
    while (1) {
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle,         
        Sensors.read();
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

    // Initalize Log File:
    if(!LogFile.begin()) {
        log_e("Failed to initialize wlan module");
        return;
    }
    if(!Wlan.init()) {
        log_e("Failed to initialize wlan module");
        return;
    }
    if(!Wlan.connect()) {
        log_e("Could not connect to network");
        return;
    }
    if(!Time.begin()) {
        log_e("Failed to initialize system time");
        return;
    }
    Wlan.disconnect();

    // Initalize Data File:
    if(!DataFile.begin()) {
        LogFile.log(ERROR, "Failed to setup filesystem for data file");
        return;
    }
    std::string filename = "/data_"+Time.toDateString()+".txt";
    if(!DataFile.init(filename)) {
        LogFile.log(ERROR, "Failed to initialize data file");
        return;
    }
    
    // Enable Button Handler:
    xTaskCreate(buttonHandlerTask, "buttonHandlerTask", DEFAULT_STACK_SIZE, NULL, 1, &buttonHandlerHandle);
    configASSERT(buttonHandlerHandle);
    Button.begin(buttonHandlerHandle);

    // Enable Sensors:
    Sensors.begin();

    // Read Config from Flash Memory:
    log_i("[INFO] Intervals:");
    std::vector<interval_t> intervals;
    intervals.reserve(MAX_INTERVALLS);
    Config.loadPumpIntervals(intervals);
    for(interval_t interval : intervals) {
        //Read out preferences from flash:
        std::string start = TimeManager::toTimeString(interval.start);
        std::string stop = TimeManager::toTimeString(interval.stop);
        log_i("%s - %s {%X}", start.c_str(), stop.c_str(), interval.wday);
    }
    Pump.scheduleIntervals(intervals);
    Pump.setThreshold(0);

    // Initialize Gateway:
    Gateway.load();

    // Initialize Web Server User Interface:
    if(!UserInterface.enable()) {
        LogFile.log(ERROR, "Failed to enable ui");
        return;
    }

    // Create and Start Scheduled Tasks:
    xTaskCreate(measurementTask,"measurementTask",DEFAULT_STACK_SIZE,NULL,1,NULL);
    xTaskCreate(serviceTask,"serviceTask",DEFAULT_STACK_SIZE,NULL,1,NULL);
    xTaskCreate(networkLoop,"networkLoop",DEFAULT_STACK_SIZE,NULL,1,&networkLoopHandle);

    // Finish Setup:
    LogFile.log(INFO, "Device setup.");
    vTaskDelete(NULL); // delete this task to prevent busy idling in empty loop()
}

void loop() {}