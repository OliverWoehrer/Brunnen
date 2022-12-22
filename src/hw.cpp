/**
 * @author Oliver Woehrer
 * @date 17.08.2022
 * @file hw.cpp
 * This modul [Hardware.cpp] provides functions to handle the hardware which is connected to the
 * unit and is placed on the pcb itself. It allows to read out the sensor values when they are
 * ready. Due to the water level sensor having a weak up delay of approximately 360 ms the values
 * have to be requested an can be read when they are ready (after aprox. 360 ms). 
 */
#include <Preferences.h>
#include "Arduino.h"
#include "hw.h"

namespace Hardware {

//===============================================================================================
// LED
//===============================================================================================
namespace Leds {
    /**
     * Initializes the pins of the color LEDs and performes a short blinking animation.
     */
    void init() {
        pinMode(LED_RED, OUTPUT);
        pinMode(LED_YELLOW, OUTPUT);
        pinMode(LED_GREEN, OUTPUT);
        pinMode(LED_BLUE, OUTPUT); // set pin mode of onboard led
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_YELLOW, LOW);
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_BLUE, LOW); // turn all leds off
    }

    /**
     * Turns on the led given in the param
     * @param color color of the led to be turned on
     */
    void turnOn(color_t color) {
        switch (color) {
        case RED:
            digitalWrite(LED_RED, HIGH);
            break;
        case YELLOW:
            digitalWrite(LED_YELLOW, HIGH);
            break;
        case GREEN:
            digitalWrite(LED_GREEN, HIGH);
            break;
        case BLUE:
            digitalWrite(LED_BLUE, HIGH);
            break;
        default:
            break;
        }
    }
    
    /**
     * Turns off the led given in the param
     * @param color color of the led to be turned off
     */
    void turnOff(color_t color) {
        switch (color) {
        case RED:
            digitalWrite(LED_RED, LOW);
            break;
        case YELLOW:
            digitalWrite(LED_YELLOW, LOW);
            break;
        case GREEN:
            digitalWrite(LED_GREEN, LOW);
            break;
        case BLUE:
            digitalWrite(LED_BLUE, LOW);
            break;
        default:
            break;
        }
    }

    /**
     * Toggles the led given in the param, e.g turns off the led when it is currently on
     * and otherwise
     * @param color color of the led to be toggled
     */
    void toggle(color_t color) {
        switch (color) {
        case RED:
            digitalWrite(LED_RED, !digitalRead(LED_RED));
            break;
        case YELLOW:
            digitalWrite(LED_YELLOW, !digitalRead(LED_YELLOW));
            break;
        case GREEN:
            digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
            break;
        case BLUE:
            digitalWrite(LED_BLUE, !digitalRead(LED_BLUE));
            break;
        default:
            break;
        }
    }
}

//===============================================================================================
// SENSORS
//===============================================================================================
namespace Sensors {
    unsigned int flowMinThreshold = 0;
    unsigned int flowMaxThreshold = 7000;
    unsigned int pressureMinThreshold = 200;
    unsigned int pressureMaxThreshold = 4000;
    unsigned int levelMinThreshold = 1000;
    unsigned int levelMaxThreshold = 4000;
    unsigned int edgeCounter = 0; // used for counting edges produced by the water flow sensor
    unsigned int waterFlow = 0;
    int waterPressure = 0;
    int waterLevel = 0;
    unsigned long now = 0; // holds time in ms, used to wait for sensor weak up delay 
    char valueString[VALUE_STRING_LENGTH] = "";

    /**
     * Interrupt service routine gets called every time a rising edge gets detected at the
     * output of the water flow sensor. This variable is then used and reset at sensor readout.
     */
    void edgeCounterISR() {
        edgeCounter += 1;
    }

    /**
     * Initializes the sensors connected to the esp32 board. The sensors being for water flow,
     * water pressure and water level.
     */
    void init() {
        //Initialze sensor values:
        edgeCounter = 0;
        waterFlow = 0;
        waterPressure = 0;
        waterLevel = 0;

        //Initilize string buffer:
        for (unsigned int i = 0; i < VALUE_STRING_LENGTH; i++) {
            valueString[i] = 0;
        }

        //Set up pin mode:
        pinMode(WATERFLOW_SENSOR, INPUT); 
        pinMode(WATER_PRESSURE_SENSOR, INPUT);
        pinMode(WATER_LEVEL_SENSOR, INPUT);
        pinMode(SENSOR_SWITCH, OUTPUT); // set pin as output
        attachInterrupt(WATERFLOW_SENSOR, edgeCounterISR, RISING); // interrupt on change
    }

    /**
     * This function switches the power supply of the water level sensor to ON and enables
     * the sensor by weaking it up.
     */
    void requestValues() {
        digitalWrite(SENSOR_SWITCH, HIGH);
        now = millis();
    }

    /**
     * Checks if the water level sensor is already up and running. This is
     * the case when the requestValues() function has been called at least 360 milliseconds in
     * advance, because the sensor has a weak up delay of approxemately 360 milliseconds.
     * @return true, if the water level sensor is up and running
     */
    bool hasValuesReady() {
        return millis() > now+360;
    }

    /**
     * The values of all sensors are read and/or stored in a local variable (sensor readout).
     * Afterwards the water level sensor is being disabled again. The values are all read at the
     * same time to ensure data consistency. For further use, the values are written to the valueString
     * buffer as a string.
     * @return false, if a sensor is out of its nominal range
     */
    void readValues() { 
        waterFlow = edgeCounter;
        edgeCounter = 0; // reset edge counter
        waterPressure = analogRead(WATER_PRESSURE_SENSOR);
        waterLevel = analogRead(WATER_LEVEL_SENSOR);
        digitalWrite(SENSOR_SWITCH, LOW); // disable water level sensor again
        sprintf(valueString, "%d,%d,%d", waterFlow, waterPressure, waterLevel);
    }

    /**
     * Checks if all sensor values are in between their min/max threaholds
     * @return true, if all sensors are in nominal ranges
     */
    bool hasNominalValues() {
        if ((flowMinThreshold <= edgeCounter && edgeCounter <= flowMaxThreshold)
            && (pressureMinThreshold <= waterPressure && waterPressure <= pressureMaxThreshold)
            && (levelMinThreshold <= waterLevel && waterLevel <= levelMaxThreshold))
            return SUCCESS;
        else
            return FAILURE;
    }

    /**
     * Get the water level updated at the last sensor read out
     * @return water level raw value
     */
    int hasMinWaterLevel() {
        return waterLevel > levelMinThreshold;
    }

    /**
     * Returns the string representation of the sensor values from the last sensor readout 
     * @return valueString with max length VALUE_STRING_LENGTH
     */
    char* toString() {
        return valueString;
    }
}

//===============================================================================================
// BUTTON
//===============================================================================================
namespace Button {
    unsigned int cnt = 0;
    indicator_t indicator;
    TaskHandle_t btnHandler = NULL;
    hw_timer_t *btnTimer = NULL;

    /**
     * Once the button is pressed this interrupt service routine get called peroidically
     * every BTN_SAMPLING_RATE and checks if the button is stilled pressed to determine if it
     * is a long button press or just a single button tap. Every button press lasting shorter
     * than BTN_SAMPLING_RATE is not detected.
     * @note IRAM_ATTR prefix so the code gets placed in IRAM and is faster loaded when needed
     */
    void static IRAM_ATTR periodicButton() { //static
        if (digitalRead(BUTTON) == HIGH) {
            cnt++;
            if (cnt == 30) { // do not use (>), to prevent resetting the flag multiple times!
                indicator.shortPressed = false;
                indicator.longPressed = true;
                xTaskResumeFromISR(btnHandler);
            }
        } else if (btnTimer) { // button not pressed anymore
            if (1 < cnt && cnt < 30) {
                indicator.shortPressed = true;
                indicator.longPressed = false;
                xTaskResumeFromISR(btnHandler);
            }
            
            // timerEnd(btnTimer); // stop button the sampling
            // btnTimer = NULL;
            timerDetachInterrupt(btnTimer);
            cnt = 0;
        } else { /* should not be reached !*/ }
    }

    /**
     * Initializes the button pin and attaches the ISR to handle the rising edge on
     * the button input pin
     */
    void init(TaskHandle_t* buttonHandler) {
        pinMode(BUTTON, INPUT);
        btnHandler = *buttonHandler;
        btnTimer = timerBegin(1, 80, true); // initialize timer1
        attachInterrupt(BUTTON, ISR, RISING); // interrupt on change
    }

    /**
     * Called when a rising edge is detected in the button input pin. This means that the
     * button is pressed and therefore the periodic button sampling is stated. The sampling ISR
     * done by calling the periodicButton() function every BTN_SAMPLING_RATE and checking the
     * state of the button input pin.
     * @note IRAM_ATTR prefix so the code gets placed in IRAM and is faster loaded when needed
     */
    void IRAM_ATTR ISR() {
        // btnTimer = timerBegin(1, 80, true); // initialize timer1
        timerAttachInterrupt(btnTimer, &periodicButton, true);
        timerAlarmWrite(btnTimer, BTN_SAMPLING_RATE, true);
        timerAlarmEnable(btnTimer);
    }

    /**
     * Indicates if a button press occured. The indicator is a struct holding two boolean
     * variables: shortPressed and longPressed which are set when the button was previously pressed
     * for a long/short time.
     * @return indicator struct, holding the button flags
     */
    indicator_t getIndicator() {
        return indicator;
    }

    /**
     * Reset the flags indicating that a button press occured
     */
    void resetIndicator() {
        indicator.shortPressed = false;
        indicator.longPressed = false;
    }
}

//===============================================================================================
// RELAIS
//===============================================================================================
namespace Relais {
    op_mode_t operatingMode = SCHEDULED;
    op_mode_t cachedOperatingMode = SCHEDULED;
    bool operating = true;
    int operatingLevel = 0;
    interval_t intervals[MAX_INTERVALLS];

    /**
     * Initializes the relais pin to controll the connected N-FET and switch the relais.
     */
    void init() {
        pinMode(RELAIS, OUTPUT);
        digitalWrite(RELAIS, LOW);
    }

    /**
     * Turns the relais output pin on
     */
    void turnOn() {
        digitalWrite(RELAIS, HIGH);
    }

    /**
     * Turns the relais output pin off
     */
    void turnOff() {
        digitalWrite(RELAIS, LOW);
    }

    /**
     * Sets the operating mode of the relais to the given param, the previous operating
     * mode gets cached and can be reset by calling resetOpMode.
     * @param mode operating mode to be set
     */
    void setOpMode(op_mode_t mode) {
        if (mode != operatingMode) { // only set mode if it is new
            cachedOperatingMode = operatingMode;
            operatingMode = mode;
        } // else: do nothing
    }

    /**
     * Resets the operating mode of the relais to the cached mode
     */
    void resetOpMode() {
        operatingMode = cachedOperatingMode;
    }

    /**
     * Pauses the (scheduled) operation of the relias, manual mode still works
     */
    void pauseOperation() {
        operating = false;
    }

    /**
     * Resumes the (scheduled) operation of the relias, manual mode still works
     */
    void resumeOperation() {
        operating = true;
    }

    /**
     * Tells if the relais is on (scheduled) operation or paused
     */
    bool isOperating() {
        return operating;
    }

    /**
     * Get the current threshold rain level for pump operation
     * @return rain threashold in mm
     */
    int getOperatingLevel() {
        return operatingLevel;
    }

    /**
     * Set the threshold rain level for pump operation
     * @param level rain threashold in mm
     */
    void setOperatingLevel(int level) {
        operatingLevel = level;
    }

    /**
     * Get the current operating mode of the relais
     * @return operating mode of the relais
     */
    op_mode_t getOpMode() {
        return operatingMode;
    }

    /**
     * Sets the given interval at the given index
     * @param interval interval to be set at the timed scheduled
     * @param i index of the interval array
     */
    void setInterval(interval_t interval, unsigned int i) {
        intervals[i] = interval;
    }

    /**
     * Getter method for intervals of relais
     * @param i index of the interval to be returned
     * @return interval with the given index
     */
    interval_t getInterval(unsigned int i) {
        return intervals[i];
    }

    /**
     * Checks if the time passed is inside an interval
     * @param timeinfo time to check for intervals
     * @return true, when the given time is inside an interval
     */
    bool checkIntervals(tm timeinfo) {
        unsigned int nowEpoch = (timeinfo.tm_hour * 60) + timeinfo.tm_min;
        for (unsigned int i = 0; i < MAX_INTERVALLS; i++) {
            unsigned int startEpoch = (intervals[i].start.tm_hour *  60) + intervals[i].start.tm_min;
            unsigned int stopEpoch = (intervals[i].stop.tm_hour *  60) + intervals[i].stop.tm_min;
            if (startEpoch <= nowEpoch && nowEpoch <= stopEpoch) {
                if (intervals[i].wday & (1 << timeinfo.tm_wday)) {
                    return true;
                }
            }
        }
        return false;
    }
}

//===============================================================================================
// PREFERENCES
//===============================================================================================
namespace Pref {
    Preferences preferences; // format[20]: "/data_YYYY-MM-DD.txt"
    char fileNameBuffer[FILE_NAME_LENGTH];

    /**
     * Initalizes the preferences and mounts the flash memory
     */
    int init() {
        bool ret = preferences.begin("brunnen", false);
        preferences.end();
        return ret ? SUCCESS : FAILURE;
    }

    /**
     * Writes the the values for the starting time for intervall i into preferences
     * @param start time struct holding starting time
     * @param i index of intervall
     */
    void setStartTime(tm start, unsigned int i) {
        char startHrString[14] = "start_hour_XX";
        char startMinString[13] = "start_min_XX";
        sprintf(startHrString, "start_hour_%02d", i);
        sprintf(startMinString, "start_min_%02d", i);
        preferences.begin("brunnen", false);
        preferences.putUInt(startHrString, start.tm_hour);
        preferences.putUInt(startMinString, start.tm_min);
        preferences.end();
    }

    /**
     * Reads the the values for the starting time for intervall i from preferences
     * @param i index of intervall
     * @return time struct holding the start time
     */
    tm getStartTime(unsigned int i) {
        char startHrString[14] = "start_hour_XX";
        char startMinString[13] = "start_min_XX";
        sprintf(startHrString, "start_hour_%02d", i);
        sprintf(startMinString, "start_min_%02d", i);

        struct tm start;
        preferences.begin("brunnen", false);
        start.tm_hour = preferences.getUInt(startHrString);
        start.tm_min = preferences.getUInt(startMinString);
        start.tm_sec = 0;
        preferences.end();
        return start;
    }

    /**
     * Writes the the values for the stop time for intervall i into preferences
     * @param stop time struct holding stop time
     * @param i index of intervall
     */
    void setStopTime(tm stop, unsigned int i) {
        char stopHrString[13] = "stop_hour_XX";
        char stopMinString[12] = "stop_min_XX";
        sprintf(stopHrString, "stop_hour_%02d", i);
        sprintf(stopMinString, "stop_min_%02d", i);
        preferences.begin("brunnen", false);
        preferences.putUInt(stopHrString, stop.tm_hour);
        preferences.putUInt(stopMinString, stop.tm_min);
        preferences.end();
    }

    /**
     * Reads the the values for the stop time for intervall i from preferences
     * @param i index of intervall
     * @return time struct holding the stop time
     */
    tm getStopTime(unsigned int i) {
        char stopHrString[13] = "stop_hour_XX";
        char stopMinString[12] = "stop_min_XX";
        sprintf(stopHrString, "stop_hour_%02d", i);
        sprintf(stopMinString, "stop_min_%02d", i);

        struct tm stop;
        preferences.begin("brunnen", false);
        stop.tm_hour = preferences.getUInt(stopHrString);
        stop.tm_min = preferences.getUInt(stopMinString);
        stop.tm_sec = 0;
        preferences.end();
        return stop;
    }

    /**
     * Writes the the value for the week day for intervall i into preferences
     * @param wday value holding week days
     * @param i index of intervall
     */
    void setWeekDay(unsigned char wday, unsigned int i) {
        char wdayString[8] = "wday_XX";
        sprintf(wdayString, "wday_%02d", i);      
        preferences.begin("brunnen", false);
        preferences.putUChar(wdayString, wday);
        preferences.end();
    }

    /**
     * Reads the the value for the week day for intervall i from preferences
     * @param i index of intervall
     * @return value for week days
     */
    unsigned char getWeekDay(unsigned int i) {
        char wdayString[8] = "wday_XX";
        sprintf(wdayString, "wday_%02d", i);
        preferences.begin("brunnen", false);
        unsigned char wd = preferences.getUChar(wdayString);
        preferences.end();
        return wd;
    }

    /**
     * Writes the number of jobs to be done into preferences
     * @param jobLength number to set
     */
    void setJobLength(uint8_t jobLength) {
        preferences.begin("brunnen", false);
        preferences.putUChar("jobLength", jobLength);
        preferences.end();
    }

    /**
     * Reads the number of jobs to be done from preferences
     * @return number of jobs to be done
     */
    unsigned char getJobLength() {
        preferences.begin("brunnen", false);
        unsigned char jl = preferences.getUChar("jobLength", 0);
        preferences.end();
        return jl;
    }

    /**
     * Takes the fileName and stores it into flash memory at the position/index given by jobNumber
     * @param jobNumber position/index in job list to write to
     * @param fileName name of the data file to save to
     */
    void setJob(unsigned char jobNumber, const char* fileName) {
        //Convert filename into hash integer (DDMMYYYY as integer number)
        unsigned char i = 0; // format for fileName "/data_2022-06-23.txt"
        unsigned char offset = 0;
        while(fileName[i] != '_') {
            i++;
        }
        i++;

        char yearString[5] = "XXXX";
        offset = 0;
        while(fileName[i] != '-') {
            yearString[offset] = fileName[i];
            i++;
            offset++;
        }
        i++;
        
        char monthString[3] = "XX";
        offset = 0;
        while(fileName[i] != '-') {
            monthString[offset] = fileName[i];
            i++;
            offset++;
        }
        i++;

        char dayString[3] = "XX";
        offset = 0;
        while(fileName[i] != '.') {
            dayString[offset] = fileName[i];
            i++;
            offset++;
        }
        i++;
        
        long int year = strtol(yearString,NULL,10);
        long int month = strtol(monthString,NULL,10);
        long int day = strtol(dayString,NULL,10);
        unsigned int hash = year*10000 + month*100 + day; // hash integer (YYYYMMDD as integer number)

        char jobKey[7] = "job_XX"; // build string for preferences key
        sprintf(jobKey,"job_%02d",jobNumber);

        preferences.begin("brunnen", false);
        preferences.putUInt(jobKey, hash);
        preferences.end();
    }
    
    /**
     * Loads the file name from flash memory from position/index given by jobNumber
     * @param jobNumber position/index in job list to load from
     * @return name of data file loaded from job list
     */
    const char* getJob(unsigned char jobNumber) {
        char jobKey[7] = "job_XX"; // build string for preferences key
        sprintf(jobKey,"job_%02d",jobNumber);

        preferences.begin("brunnen", false);
        const unsigned int hash = preferences.getUInt(jobKey); // hash integer (YYYYMMDD as integer number)
        preferences.end();
        
        const unsigned int year = (hash/10000) % 10000;
        const unsigned int month = (hash/100) % 100;
        const unsigned int day = hash % 100;

        sprintf(fileNameBuffer, "/data_%04d-%02d-%02d.txt",year,month,day);
        return fileNameBuffer;
    }

    /**
     * Delets the job with the given number from preferences memory
     * @param jobNumber number (=index) of the number to delete
     */
    void removeJob(unsigned char jobNumber) {
        char jobKey[7] = "job_XX"; // build string for preferences key
        sprintf(jobKey,"job_%02d",jobNumber);

        preferences.begin("brunnen", false);
        preferences.remove(jobKey);
        preferences.end();
    }

    /**
     * Loads the threshold level from flash memory
     * @return threshold level from memory
     */
    int getThreshold() {
        preferences.begin("brunnen", false);
        int threshold = preferences.getInt("threshold");
        preferences.end();
        return threshold;
    }

    /**
     * Takes the threshold level and stores it into flash memory
     * @param level threshold level to store
     */
    void setThreshold(int level) {
        preferences.begin("brunnen", false);
        preferences.putInt("threshold", level);
        preferences.end();
    }
}

//===============================================================================================
// HARDWARE
//===============================================================================================
/**
 * Initializes the I/O ports and operational modes to the connected hardware modules
 */
int init(TaskHandle_t* buttonHandler) {
    Leds::init();
    Sensors::init();
    Button::init(buttonHandler);
    Relais::init();
    if(Pref::init()) {
        Serial.printf("Failed to read out preferences from flash memory!\r\n");
        return FAILURE;
    }

    //Read Out Preferences from Flash Memory:
    Serial.printf("[INFO] Intervals:\r\n");
    for (unsigned int i = 0; i < MAX_INTERVALLS; i++) {
        //Read out preferences from flash:
        struct tm start = Pref::getStartTime(i);
        struct tm stop = Pref::getStopTime(i);
        unsigned char wday = Pref::getWeekDay(i);

        //Initialize interval:
        Relais::interval_t interval = {.start = start, .stop = stop, .wday = wday};
        Relais::setInterval(interval, i);
        Serial.printf("(%d) %d:%d - %d:%d {%u}\r\n",i,interval.start.tm_hour,interval.start.tm_min,interval.stop.tm_hour,interval.stop.tm_min, interval.wday);
    }

    //Read Out Rain Threshold Level from Flash Memory:
    int rainThreshold = Pref::getThreshold();
    Relais::setOperatingLevel(rainThreshold);

    return SUCCESS;
}

/**
 * Set the led indicating the status of the UI
 * @param value zero to turn the led off, otherwise the led is turned on
 */
void setUILed(char value) {
    if(value) Leds::turnOn(Leds::GREEN);
    else Leds::turnOff(Leds::GREEN);
}

/**
 * Set the led indicating the status of the UI
 * @param value zero to turn the led off, otherwise the led is turned on
 */
void setIndexLed(char value) {
    if(value) Leds::turnOn(Leds::BLUE);
    else Leds::turnOff(Leds::BLUE);
}

/**
 * Set the led indicating the status of the UI
 * @param value zero to turn the led off, otherwise the led is turned on
 */
void setErrorLed(char value) {
    if(value) Leds::turnOn(Leds::RED);
    else Leds::turnOff(Leds::RED);
}

/**
 * This function reads out the sensor values when the sensors are ready and brings a delay of approx. 360ms. This is
 * done by swithcing the power supply of the water level sensor to ON and weaking it up. Afterwards it checks if the
 * water level sensor is already up and running. This is the case after approxemately 360 milliseconds. The values of all
 * sensors are then read and/or stored in a local variable (sensor readout). Then the water level sensor is being disabled
 * again. The values are all read at the same time to ensure data consistency. For further use, the values are written to
 * the valueString buffer as a string.
 * @return false, if a sensor is out of its nominal range
 */
void readSensorValues() {
    Sensors::requestValues();
    vTaskDelay((360+10) / portTICK_PERIOD_MS); // wait for sensor to weak up
    if (Sensors::hasValuesReady()) {
        Sensors::readValues();
    }
}

/**
 * Returns the string representation of the sensor values from the last sensor readout 
 * @return string with max length VALUE_STRING_LENGTH
 */
char* sensorValuesToString() {
    return Sensors::toString();
}

/**
 * Checks if all sensor values are in between their min/max threaholds
 * @return true, if all sensors are in nominal ranges
 */
bool hasNominalSensorValues() {
    return Sensors::hasNominalValues();
}

/**
 * Indicates if a button press occured. The indicator is a struct holding two boolean
 * variables: shortPressed and longPressed which are set when the button was previously pressed
 * for a long/short time.
 * @return indicator struct, holding the button flags
 */
button_indicator_t getButtonIndicator() {
    return Button::getIndicator();
}

/**
 * Reset the flags indicating that a button press occured
 */
void resetButtonFlags() {
    Button::resetIndicator();
}

/**
 * Toggles the state of the waterpump and sets operating mode according to the new state.
 * If the pump is now switched on the operating mode is set to manual otherwise the operating mode gets
 * reset to the cached mode.
 */
void manuallyToggleWaterPump() {
    pump_op_mode_t currentMode = Relais::getOpMode();
    if (currentMode != Relais::MANUAL) {
        Relais::setOpMode(Relais::MANUAL);
        Relais::turnOn();
        Leds::turnOn(Leds::YELLOW);
    } else {
        Relais::resetOpMode();
        Relais::turnOff();
        Leds::turnOff(Leds::YELLOW);
    }
}

/**
 * Pauses the (scheduled) operating pump, manual mode still works
 */
void pauseScheduledPumpOperation() {
    Relais::pauseOperation();
}

/**
 * Resumes the (scheduled) operating pump, manual mode still works
 */
void resumeScheduledPumpOperation() {
    Relais::resumeOperation();
}

/**
 * Get the current threshold rain level for pump operation
 * @return rain threashold in mm
 */
int getRainThresholdLevel() {
    return Relais::getOperatingLevel();
}

/**
 * Set the threshold rain level for pump operation. The level is also stored into flash memory
 * @param level rain threashold in mm
 */
void setRainThresholdLevel(int level) {
    Relais::setOperatingLevel(level);
    Pref::setThreshold(level);
}

/**
 * Sets the given interval at the given index
 * @param interval interval to be set at the timed scheduled
 * @param i index of the interval array
 */
void setPumpInterval(Relais::interval_t interval, unsigned int i) {
    Relais::setInterval(interval, i);
}

/**
 * Getter method for intervals of water pump
 * @param i index of the interval to be returned
 * @return interval with the given index
 */
pump_intervall_t getPumpInterval(unsigned int i) {
    return Relais::getInterval(i);
}

/**
 * Checks if the water pump should be switched/toggled and does so in case the operational mode
 * is set to SCHEDULED or AUTOMATIC. If the pump is operating on schedul the given timeinfo checked if
 * it is inside an interval (switch ON) or outside (switch OFF). If the pump is operating automatically
 * the water level is checked to see if their is enough water.
 * @param timeinfo time to check for intervals
 * @return true, when the given time is inside an interval
 */
void managePumpIntervals(tm timeinfo) {
    switch (Relais::getOpMode()) {
    case Relais::MANUAL:
        // do not toggel relais
        break;
    case Relais::SCHEDULED:
        if (Relais::isOperating() && Relais::checkIntervals(timeinfo)) {
            Relais::turnOn();
            Leds::turnOn(Leds::YELLOW);
        } else {
            Relais::turnOff();
            Leds::turnOff(Leds::YELLOW);
        }   
        break;
    case Relais::AUTOMATIC:
        if (Relais::isOperating() && Relais::checkIntervals(timeinfo) && Sensors::hasMinWaterLevel()) {
            Relais::turnOn();
            Leds::turnOn(Leds::YELLOW);
        } else {
            Relais::turnOff();
            Leds::turnOff(Leds::YELLOW);
        }
        break;
    default:
        Relais::turnOff();
        Leds::turnOff(Leds::YELLOW);
        break;
    }
}

void saveStartTime(tm start, unsigned int i) {
    Pref::setStartTime(start, i);
}

tm loadStartTime(unsigned int i) {
    return Pref::getStartTime(i);
}

void saveStopTime(tm stop, unsigned int i) {
    Pref::setStopTime(stop, i);
}

tm loadStopTime(unsigned int i) {
    return Pref::getStopTime(i);
}

void saveWeekDay(unsigned char wday, unsigned int i) {
    Pref::setWeekDay(wday, i);
}

unsigned char loadWeekDay(unsigned int i) {
    return Pref::getWeekDay(i);
}

void saveJobLength(unsigned char jobLength) {
    Pref::setJobLength(jobLength);
}

unsigned char loadJobLength() {
    return Pref::getJobLength();
}

void saveJob(unsigned char jobNumber, const char* fileName) {
    Pref::setJob(jobNumber, fileName);
}

const char* loadJob(unsigned char jobNumber) {
    return Pref::getJob(jobNumber);
}

void deleteJob(unsigned char jobNumber) {
    Pref::removeJob(jobNumber);
}
}
