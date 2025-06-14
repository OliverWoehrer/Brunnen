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
TaskHandle_t syncLoopHandle = NULL;

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
 * This function implemenets the updaterTask. It starts to download the firmware binary file and 
 * save the data to the partition. After this, the update is ready to be installed and the device
 * is rebooted.
 * @param parameter Pointer to a parameter struct (unused for now)
 * @note Started by the synchronization task if it detects a new firmware version is available. 
 */
void updaterTask(void* parameter) {
    do {
        // Fetch Firmware File:
        LogFile.log(INFO, "Downloading firmware");
        if(!Gateway.downloadFirmware()) {
            LogFile.log(ERROR, "Failed to download firmware");
            break;
        }

        // Finalize Update:
        LogFile.log(INFO, "Firmware installed. Rebooting...");
        delay(3000);
        ESP.restart();
    } while(0);

    // Exit This Task:
    xTaskNotifyGive(syncLoopHandle); // notfiy sync loop task
    vTaskDelete(NULL); // delete task when done, don't forget this!
}

/**
 * This function implements the synchronizationTask and periodically connects to the backend to
 * synchronize data and settings. It is implemented as a periodic loop with a variable periode
 * length, depending on the amount of data to synchronize. The periode length is recommended by the
 * server in its response, but not mandatory. Recommended periode lengths are followed if there is no
 * data left to sync. If there is still data left to synchronize, the periode is kept at a few
 * seconds to sync again.
 * @param parameter Pointer to a parameter struct (unused for now)
 * @note Loops roughly every couple of seconds or once an hour
 */
void synchronizationTask(void* parameter) {
    // Initalize Task:
    TickType_t xLastWakeTime = xTaskGetTickCount(); // initalize tick time
    unsigned int networkLoopPeriode = SYNCHRONIZATION_PERIOD; // loop periode in milliseconds
    size_t lastFreeHeapSize = -1; // unsigned -1 = unsigned max value
    
    // Periodic Loop:
    while (1) {
        // Clear Gateway:
        Gateway.clear(); // clear any previous data

        // Set Sync Periode:
        log_d("loop periode %u sec", networkLoopPeriode/1000);
        TickType_t xFrequency = networkLoopPeriode / portTICK_PERIOD_MS;
        xTaskDelayUntil(&xLastWakeTime,xFrequency); // wait for the next cycle, blocking

        // Check Heap Size:
        size_t freeHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
        if(freeHeapSize < lastFreeHeapSize) {
            LogFile.log(DEBUG, "Largest region currently free in heap at "+std::to_string(freeHeapSize)+" bytes.");
            lastFreeHeapSize = freeHeapSize;
        }

        // Connect to WiFi:
        if(!Wlan.connect()) {
            LogFile.log(ERROR, "Cannot connect to network.");
            continue;
        }

        // Append Data to JSON:
        std::vector<sensor_data_t> sensorData;
        sensorData.reserve(BATCH_SIZE);
        if(!DataFile.exportData(sensorData)) {
            LogFile.log(ERROR, "Failed to export sensor values");
            continue;
        }
        if(sensorData.size() == 0) { // check if any data got exported
            LogFile.log(WARNING, "No data exported");
            LogFile.log(INFO, "Resetting data file"); // reset file to fix possible broken file
            DataFile.clear();
        }
        if(!Gateway.insertData(sensorData)) {
            LogFile.log(ERROR, "Failed to insert data");
            continue;
        }

        // Append Logs to JSON:
        std::vector<log_message_t> logMessages;
        logMessages.reserve(20);
        if(!LogFile.exportLogs(logMessages)) {
            LogFile.log(ERROR, "Failed to export log messages");
            continue;
        }
        if(!Gateway.insertLogs(logMessages)) {
            LogFile.log(ERROR, "Failed to insert logs");
            continue;
        }

        // Append Firmware Version to JSON:
        std::string version = Config.loadFirmwareVersion();
        if(!Gateway.insertFirmwareVersion(version)) {
            LogFile.log(ERROR, "Failed to insert firmware version");
            continue;
        }

        // Send Sync Request:
        if(!Gateway.synchronize()) {
            LogFile.log(ERROR, "Failed to synchronize.");
            continue;
        }

        // Clear Error Led: sync'ed any error logs
        LogFile.acknowledge();

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
            size_t count = DataFile.itemCount();
            log_d("target periode sync[%d] = %u sec", sync.mode, sync.periods[sync.mode]);
            log_d("Data items left: %u", count);        
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

        // Check for new Firmware Version:
        std::string available_version;
        if(Gateway.getFirmware(available_version)) {
            std::string deployed_version = Config.loadFirmwareVersion();
            log_d("Firmware versions -> Available: %s Deployed: %s",available_version.c_str(), deployed_version.c_str());
            if(deployed_version != available_version) {
                LogFile.log(INFO, "New firmware version available");
                
                // Start Updater Task:
                xTaskCreate(updaterTask,"updaterTask",2*DEFAULT_STACK_SIZE,NULL,0,NULL); // priority 0 (same as idle task) to prevent idle task from starvation
                
                // Wait For Updater to Finish:
                ulTaskNotifyTake(pdTRUE, (180*1000)/portTICK_PERIOD_MS); // blocking wait for notification up to 180 seconds
            }
        }

        // Shrink Data File:
        if(!DataFile.shrink(sensorData.size())) {
            LogFile.log(WARNING, "Failed to shrink data file");
            continue;
        }

        // Shrink Log File:
        if(!LogFile.shrink(logMessages.size())) {
            LogFile.log(WARNING, "Failed to shrink log file");
            continue;
        }
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
        LogFile.log(ERROR, "Failed to initialize data file");
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
    xTaskCreate(synchronizationTask,"synchronizationLoop",2*DEFAULT_STACK_SIZE,NULL,0,&syncLoopHandle); // priority 0 (same as idle task) to prevent idle task from starvation

    // Finish Setup:
    LogFile.log(INFO, "Device setup.");
    vTaskDelete(NULL); // delete this task to prevent busy idling in empty loop()
}

void loop() {}