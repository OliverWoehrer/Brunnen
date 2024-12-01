/**
 * @author Oliver Woehrer
 * @date 17.08.2022
 * @file hw.cpp
 * This modul [Hardware.cpp] provides functions to handle the hardware which is connected to the
 * unit and is placed on the pcb itself. It allows to read out the sensor values when they are
 * ready. Due to the water level sensor having a weak up delay of approximately 360 ms the values
 * have to be requested an can be read when they are ready (after aprox. 360 ms). 
 */
#include <string>
#include <sstream>
#include <vector>
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
     * Get the water level updated at the last sensor read out
     * @return water level raw value
     */
    int getWaterLevel() {
        return waterLevel;
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
    void IRAM_ATTR periodicButton() { //static
        if (digitalRead(BUTTON) == HIGH) {
            cnt++;
            if (cnt == 30) { // max cnt 30: long press
                indicator.shortPressed = false;
                indicator.longPressed = true;
                xTaskResumeFromISR(btnHandler);
            }
        } else { // button not pressed anymore
            if (1 < cnt && cnt < 30) {
                indicator.shortPressed = true;
                indicator.longPressed = false;
                xTaskResumeFromISR(btnHandler);
            }
            
            // Disable Periodic Btn Sampling:
            cnt = 0;
            attachInterrupt(BUTTON, ISR, ONHIGH); // interrupt on "high level", "rising edge" not supported by arduino-esp32 framework
            timerDetachInterrupt(btnTimer);
        }
    }

    /**
     * Called when a rising edge is detected in the button input pin. This means that the
     * button is pressed and therefore the periodic button sampling is stated. The sampling ISR
     * done by calling the periodicButton() function every BTN_SAMPLING_RATE and checking the
     * state of the button input pin.
     * @note IRAM_ATTR prefix so the code gets placed in IRAM and is faster loaded when needed
     */
    void IRAM_ATTR ISR() {
        detachInterrupt(BUTTON); // disable interrupt
        timerAttachInterrupt(btnTimer, periodicButton, false); // enable periodic btn sampling
    }

    /**
     * Initializes the button pin and attaches the ISR to handle the rising edge on
     * the button input pin
     */
    void init(TaskHandle_t* buttonHandler) {
        pinMode(BUTTON, INPUT);
        btnHandler = *buttonHandler;
        attachInterrupt(BUTTON, ISR, ONHIGH); // interrupt on high level, otherwise RISING
        Serial.printf("Button::init(); -> attachInterrupt(); -> done\r\n");

        // Initalize Btn Sampling Timer:
        btnTimer = timerBegin(1, 80, true); // initialize timer1
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
    void switchOn() {
        digitalWrite(RELAIS, HIGH);
    }

    /**
     * Switches the relais output pin off
     */
    void switchOff() {
        digitalWrite(RELAIS, LOW);
    }

    /**
     * Returns the current state of the relais
     * @return 1 if the relais is switched on, 0 otherwise
    */
    int get() {
        return digitalRead(RELAIS);
    }

    /**
     * Pauses the (scheduled) operation of the relais by setting operating mode to PAUSED
     */
    void pauseOperation() {
        if (operatingMode != PAUSED) { // only set mode if it is not paused already
            cachedOperatingMode = operatingMode;
            operatingMode = PAUSED;
        } // else: do nothing
    }

    /**
     * Resumes the (scheduled) operation of the relias by setting operating mode to previous mode
     */
    void resumeOperation() {
        operatingMode = cachedOperatingMode;
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
     * @return SUCCESS on success, FAILURE otherwise
     */
    int readFile(fs::FS &fs, const char *path, char* buffer, size_t size) {
        File file = fs.open(path);
        if (!file) { return FAILURE; }
        unsigned int used = 0;
        char line[VALUE_STRING_LENGTH] = "";
        char lineIdx = 0;
        while(file.available() && used < size) {
            char byte = file.read();
            if(byte == -1) { return FAILURE; }

            if(lineIdx < VALUE_STRING_LENGTH-1) {
                line[lineIdx] = byte;
                lineIdx++;
            }
            if(byte == '\n') {
                line[lineIdx] = '\0';
                sprintf(&buffer[used], "%s", line);
                used += lineIdx;
                lineIdx = 0;
            }                
        }
        file.close();
        buffer[used] = '\0';
        return SUCCESS;
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
        File file = fs.open(path, FILE_APPEND);
        if (!file) { return; }
        file.print(message);
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
void sampleSensors(const char* timeString) {
    // Indicate Start of Sensor Sampling:
    Leds::turnOn(Leds::BLUE);

    // Read Sensor Values:
    Sensors::requestValues();
    vTaskDelay((360+10) / portTICK_PERIOD_MS); // wait for sensor to weak up
    Sensors::readValues();

    // Save Sensor Values:
    size_t len = strlen(timeString);
    char dataString[len+1+VALUE_STRING_LENGTH+3];
    sprintf(dataString,"%s,%s\r\n",timeString,Sensors::toString());
    FileSystem::appendFile(SD, FileSystem::getFileName(), dataString);

    // Indicate End of Sensor Sampling:
    Leds::turnOff(Leds::BLUE);
}

bool parseCSVLine(const std::string& line, sensor_data_t& data) {
    // Split Line into Tokens:
    std::vector<std::string> tokens;
    const std::string delimiter = ",";
    size_t last = 0;
    size_t next = 0;
    while((next = line.find(delimiter, last)) != std::string::npos) {
        std::string split = line.substr(last, next-last);
        tokens.push_back(split);
        last = next + 1;
    }
    std::string split = line.substr(last);
    tokens.push_back(line);

    // Parse Tokens:
    if(tokens.size() != 4) {
        return false; // error
    }
    data.timestamp = tokens[0];
    int flow;
    if(!(std::stringstream(tokens[1]) >> flow)) {
        return false; // failed to parse integer
    }
    data.flow = flow;
    int pressure;
    if(!(std::stringstream(tokens[1]) >> pressure)) {
        return false;
    }
    data.pressure = flow;
    int level;
    if(!(std::stringstream(tokens[1]) >> level)) {
        return false;
    }
    data.level = level;

    // Return on Success:
    return true;
}

int exportDataFile(sensor_data_t sensor_data[], size_t num) {
    // Read Data File:
    size_t line_length = TIME_STRING_LENGTH + 1 + VALUE_STRING_LENGTH + 4;
    size_t size = num * line_length;
    char buffer[size]; // buffer has maximum possible size for 'num' lines of data file
    sprintf(buffer,"Timestamp,Flow,Pressure,Level\r\n2024-03-31T14:32:19,0,920,2541\r\n2024-03-31T14:32:20,0,930,2539\r\n2024-03-31T14:32:21,0,926,2539\r\n2024-03-31T14:32:22,0,926,2575\r\n2024-03-31T14:32:23,0,923,2544\r\n2024-03-31T14:32:24,0,926,2549\r\n2024-03-31T14:32:25,0,929,2545\r\n2024-03-31T14:32:26,0,927,2559\r\n2024-03-31T14:32:27,0,927,2547\r\n2024-03-31T14:32:28,0,931,2557\r\n2024-03-31T14:32:29,0,924,2543\r\n2024-03-31T14:32:30,0,915,2559\r\n2024-03-31T14:32:31,0,930,2545\r\n2024-03-31T14:32:32,0,925,2551\r\n2024-03-31T14:32:33,0,931,2546\r\n2024-03-31T14:32:34,0,923,2554\r\n2024-03-31T14:32:35,0,923,2533\r\n");
    /*TODO: read file instead of buffer
    if(FileSystem::readFile(SD, FileSystem::getFileName(), buffer, size)) {
        return -1;
    }*/

    // Parse CSV Lines:
    unsigned int bufferIdx = 0;
    char line[MAX_LOG_LENGTH] = "";
    unsigned int lineIdx = 0;
    unsigned int lineCount = 0;
    while(buffer[bufferIdx] != '\0' && lineCount < num) {
        if(lineIdx < line_length-1) { // read single character into line
            line[lineIdx] = buffer[bufferIdx];
            lineIdx++;
        }
        if(buffer[bufferIdx] == '\n') { // found line ending, parse full line
            line[lineIdx] = '\0'; // terminate string
            lineIdx = 0;
            sensor_data_t data = sensor_data_t();
            if(parseCSVLine(line, data)) {
                //TODO: check return value of first line (Header of CSV)
                sensor_data[lineCount] = data;
                lineCount++;
            }
        }
        bufferIdx++;
    }

    return lineCount;
}

int shrinkDataFile(size_t size) {
    
    return FAILURE;
}

/**
 * Returns the string representation of the sensor values from the last sensor readout 
 * @return string with max length VALUE_STRING_LENGTH
 */
char* sensorValuesToString() {
    return Sensors::toString();
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
 * Switch pump according to given value
 * @param value 0 to switch pump off, otherwise pump is switched on
*/
void switchPump(char value) {
    if (value) {
        Relais::switchOn();
        Leds::turnOn(Leds::YELLOW);
    } else {
        Relais::switchOff();
        Leds::turnOff(Leds::YELLOW);
    }
}

/**
 * Toggles the state of the waterpump.
 * If the pump is now switched on the operating mode is set to manual otherwise the operating mode gets
 * reset to the cached mode.
 */
void togglePump() {
    int currentState = Relais::get();
    if (currentState) { // relais currently switched on
        Relais::switchOff();
        Leds::turnOff(Leds::YELLOW);
    } else {
        Relais::switchOn();
        Leds::turnOn(Leds::YELLOW);
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
void setPumpInterval(Hardware::pump_intervall_t interval, unsigned int i) {
    Relais::setInterval(interval, i);
}

void setPumpIntervals(Hardware::pump_intervall_t* intervals) {
    for(size_t i = 0; i < MAX_INTERVALLS; i++) {
        Relais::setInterval(intervals[i], i);
    }
}

Hardware::pump_intervall_t defaultInterval() {
    tm start;
    start.tm_hour = 0;
    start.tm_min = 0;
    start.tm_sec = 0;
    tm stop;
    stop.tm_hour = 0;
    stop.tm_min = 0;
    stop.tm_sec = 0;
    Hardware::pump_intervall_t interval = { .start = start, .stop = stop, .wday = 0 };
    return interval;
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
 * Checks if the water pump is scheduled to be switched on. In case the operational mode is set to
 * PAUSED the pump is in general not scheduled to be switched on. In case the operational mode is set
 * to SCHEDULED, the given timeinfo is checked to be inside an interval (switch ON) or outside
 * (switch OFF). If the operational mode is set to or AUTOMATIC, the water level is checked additionally
 * to see if their is enough water.
 * @param timeinfo time to check for intervals
 * @return true, when the pump is scheduled to run
 */
int getScheduledPumpState(tm timeinfo) {
    switch (Relais::getOpMode()) {
    case Relais::PAUSED:
        return 0;
        break;
    case Relais::SCHEDULED:
        return Relais::checkIntervals(timeinfo); 
        break;
    case Relais::AUTOMATIC:
        return Relais::checkIntervals(timeinfo) && (Sensors::getWaterLevel() >= Relais::getOperatingLevel());
        break;
    default:
        return 0;
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
