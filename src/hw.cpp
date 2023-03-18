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
#include <SPIFFS.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
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
// FILE SYSTEM
//===============================================================================================
namespace FileSystem {
    char fileNameBuffer[FILE_NAME_LENGTH]; // format[21]: "/data_YYYY-MM-DD.txt"

    /**
     * Uses the serial interface to print the contents of the given directory
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param dirname name of the directory (e.g "/")
     * @param levels the number of hierachy levels to print
     */
    void listDirectory(fs::FS &fs, const char *dirname, uint8_t levels) {
        Serial.printf("Listing directory: %s\r\n", dirname);
        File root = fs.open(dirname);
        if (!root) {
            Serial.println("Failed to open directory");
            return;
        }
        if (!root.isDirectory()) {
            Serial.println("Not a directory");
            return;
        }
        File file = root.openNextFile();
        while (file) {
            if (file.isDirectory()) {
                Serial.print("  DIR : ");
                Serial.println(file.name());
                if (levels) listDirectory(fs, file.name(), levels - 1);
            } else {
                Serial.print("  FILE: ");
                Serial.print(file.name());
                Serial.print("  SIZE: ");
                Serial.println(file.size());
            }
            file = root.openNextFile();
        }
    }

    /**
     * Creats a new directory (=sub-folder) in the given directory
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     */
    void createDir(fs::FS &fs, const char *path) {
        Serial.printf("Creating Dir: %s\r\n", path);
        if (fs.mkdir(path)) Serial.println("Dir created");
        else Serial.println("mkdir failed");
    }

    /**
     * Removes the directory in the given oath
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     */
    void removeDir(fs::FS &fs, const char *path) {
        Serial.printf("Removing Dir: %s\r\n", path);
        if (fs.rmdir(path)) Serial.println("Dir removed");
        else Serial.println("rmdir failed");
    }

    /**
     * Reads the content of the file at the given path
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     */
    void readFile(fs::FS &fs, const char *path) {
        Serial.printf("Reading file: %s\r\n", path);
        File file = fs.open(path);
        if (!file) {
            Serial.println("Failed to open file for reading");
            return;
        }
        Serial.print("Read from file: ");
        while (file.available()) Serial.write(file.read());
        file.close();
    }

    /**
     * Writes the given message to the beginning of the file at the given path
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     * @param message text to be written into file
     */
    void writeFile(fs::FS &fs, const char *path, const char *message) {
        Serial.printf("Writing file: %s\r\n", path);
        File file = fs.open(path, FILE_WRITE);
        if (!file) {
            Serial.println("Failed to open file for writing");
            return;
        }
        if (file.print(message)) Serial.println("File written");
        else Serial.println("Write failed");
        file.close();
    }

    /**
     * Writes the given message to the end of the file at the given path and 
     * therefor appending the given data to the existing file
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     * @param message text to be appended into file
     */
    void appendFile(fs::FS &fs, const char *path, const char *message) {
        if (!fs.exists("/")) return;

        File file = fs.open(path, FILE_APPEND);
        if (!file) {
            Serial.println("Failed to open file for appending");
            return;
        }

        if (!file.print(message)) Serial.println("Append failed");
        file.close();
    }

    /**
     * Delets the file at the given path
     * @param fs address of the file system in use (e.g. SD or SPIFFS)
     * @param path name of the directory (e.g "/")
     */
    void deleteFile(fs::FS &fs, const char *path) {
        Serial.printf("Deleting file: %s\r\n", path);
        if (fs.remove(path)) Serial.println("File deleted");
        else Serial.println("Delete failed");
    }

    /**
     * Mounts the SD card and looks for a file named "fileName". This is the file currently
     * used to store the data. If there is non found, a new file is created with the headers in
     * place. This way multiple initalizations/reboots of the system do not lead to multiple files.
     * Last the free storage space on the SD card is checked.
     * @return SUCCESS is the file was found or created, FAILURE otherwise
     */
    int init(const char* fName) {
        if (!SD.begin(SPI_CD)) {
            Serial.printf("Failed to mount SD card!\r\n");
            return FAILURE; // error code: no card shield found
        }

        //Create a file on the SD card if does not exist:
        File file = SD.open(fName);
        if(!file) {
            writeFile(SD, fName, "Timestamp,Flow,Pressure,Level\r\n");
        }
        file.close();

        //Check SD card size:
        unsigned long long usedBytes = SD.usedBytes() / (1024 * 1024);
        Serial.printf("Mounted SD card with %llu MB used.\r\n", usedBytes);
        strncpy(fileNameBuffer, fName, FILE_NAME_LENGTH-1); // set file name currently used
        return SUCCESS;
    }

    /**
     * Returns the name of the file currently used.
     * @return file name in format[21]: "/data_YYYY-MM-DD.txt"
     */
    char* getFileName() {
        return fileNameBuffer;
    }
}

//===============================================================================================
// HARDWARE
//===============================================================================================
/**
 * Initializes the I/O ports and operational modes to the connected hardware modules
 */
int init(TaskHandle_t* buttonHandler,const char* fileName) {
    Leds::init();
    Sensors::init();
    Button::init(buttonHandler);
    Relais::init();
    if(FileSystem::init(fileName)) {
        Serial.printf("Failed to initialize file system.\r\n");
        return FAILURE;
    }

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
void setErrorLed(char value) {
    if(value) Leds::turnOn(Leds::RED);
    else Leds::turnOff(Leds::RED);
}

/**
 * This function reads out the sensor values when the sensors are ready. This is done by switching the power supply of the
 * water level sensor to ON and weaking it up. Afterwards it checks if the water level sensor is ready (delay of approx.
 * 360ms). The values of all sensors are then read and/or stored in a local variable (sensor readout). Then the water
 * level sensor is being disabled again. The values are all read at the same time to ensure data consistency. Afterwards
 * the sensor values together with the given timestring are written to the (currently active) data file. The index led is
 * also switched on to indicate data sampling.
 * @param timeString string timestamp to write to data file
 * @return false, if a sensor is out of its nominal range
 */
void sampleSensorValues(const char* timeString) {
    // Indicate Start of Sensor Sampling:
    Leds::turnOn(Leds::BLUE); 

    // Read Sensor Values:
    Sensors::requestValues();
    vTaskDelay((360+10) / portTICK_PERIOD_MS); // wait for sensor to weak up
    if (Sensors::hasValuesReady()) {
        Sensors::readValues();
    }

    // Save Sensor Values:
    size_t len = strlen(timeString);
    char dataString[len+1+VALUE_STRING_LENGTH+3];
    sprintf(dataString,"%s,%s\r\n",timeString,Sensors::toString());
    FileSystem::appendFile(SD, FileSystem::getFileName(), dataString);

    // Indicate End of Sensor Sampling:
    Leds::turnOff(Leds::BLUE);
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
int getPumpOperatingLevel() {
    return Relais::getOperatingLevel();
}

/**
 * Set the threshold rain level for pump operation. The level is also stored into flash memory
 * @param level rain threashold in mm
 */
void setPumpOperatingLevel(int level) {
    Relais::setOperatingLevel(level);
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

/**
 * Loads the file name of the file currently used to store the sensor data
 * @return file name of file currently used
 */
char* loadActiveDataFileName() {
    return FileSystem::getFileName();
}

/**
 * Deletes the file currently used to store the sensor data
 */
void deleteActiveDataFile() {
    FileSystem::deleteFile(SD, FileSystem::getFileName());
}

/**
 * Sets the file name of the file currently used to store data. Similar to createCurrentDataFile()
 * @param fName file name to set (name of the file to be used now)
 * @return SUCCESS if the file was created/opened correcly, FAILURE otherwise.
 */
int setActiveDataFile(const char* fName) {
    return FileSystem::init(fName);
}


}
