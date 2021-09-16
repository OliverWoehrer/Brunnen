/**
 * @author Oliver Woehrer
 * @date 17.08.2021
 * @file hw.cpp
 * This modul [Hardware.cpp] provides functions to handle the hardware which is connected to the
 * unit and is placed on the pcb itself. It allows to read out the sensor values when they are
 * ready. Due to the water level sensor having a weak up delay of approximately 360 ms the values
 * have to be requested an can be read when they are ready (after aprox. 360 ms). 
 */
#include "Arduino.h"
#include "hw.h"


//===============================================================================================
// LED
//===============================================================================================
namespace Led {
    /**
     * @brief Initializes the pins of the color LEDs and performes a short blinking animation.
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

        //Run animation:
        const unsigned int DELAY = 500;
        turnOn(RED);
        delay(DELAY);
        turnOff(RED);
        turnOn(YELLOW);
        delay(DELAY);
        turnOff(YELLOW);
        turnOn(GREEN);
        delay(DELAY);
        turnOff(GREEN);
        turnOn(BLUE);
        delay(DELAY);
        turnOff(BLUE);
    }

    /**
     * @brief Turns on the led given in the param
     * @param color color of the led to be turned on
     */
    void turnOn(led_color_t color) {
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
     * @brief Turns off the led given in the param
     * @param color color of the led to be turned off
     */
    void turnOff(led_color_t color) {
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
     * @brief Toggles the led given in the param, e.g turns off the led when it is currently on
     * and otherwise
     * @param color color of the led to be toggled
     */
    void toggle(led_color_t color) {
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
     * @brief Interrupt service routine gets called every time a rising edge gets detected at the
     * output of the water flow sensor. This variable is then used and reset at sensor readout.
     */
    void edgeCounterISR() {
        edgeCounter += 1;
    }

    /**
     * @brief initializes the sensors connected to the esp32 board. The sensors being for
     * water flow, water pressure and water level.
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
     * @brief This function switches the power supply of the water level sensor to ON and enables
     * the sensor by weaking it up.
     */
    void requestValues() {
        digitalWrite(SENSOR_SWITCH, HIGH);
        now = millis();
    }

    /**
     * @brief Checks if the water level sensor is already up and running. This is
     * the case when the requestValues() function has been called at least 360 milliseconds in
     * advance, because the sensor has a weak up delay of approxemately 360 milliseconds.
     * @return true, if the water level sensor is up and running
     */
    bool hasValuesReady() {
        return millis() > now+360;
    }

    /**
     * @brief The values of all sensors are read and/or stored in a local variable (sensor readout).
     * Afterwards the water level sensor is being disabled again. The values are all read at the
     * same time to ensure data consistency. For further use, the values are written to the valueString
     * buffer as a string.
     * @return false, if a sensor is out of its nominal range
     */
    bool readValues() { 
        waterFlow = edgeCounter;
        edgeCounter = 0; // reset edge counter
        waterPressure = analogRead(WATER_PRESSURE_SENSOR);
        waterLevel = analogRead(WATER_LEVEL_SENSOR);
        digitalWrite(SENSOR_SWITCH, LOW); // disable water level sensor again
        sprintf(valueString, "%d, %d, %d", waterFlow, waterPressure, waterLevel);
        if ((edgeCounter < 0 || 10000 < edgeCounter) // check sensor values; nominal range
            || (waterPressure <= 400 || 4000 < waterPressure)
            || (waterLevel < 800 || 4000 < waterLevel))
            return FAILURE;
        else
            return SUCCESS;
    }

    /**
     * @brief get the water level updated at the last sensor read out
     * @return water level raw value
     */
    int getWaterLevel() {
        return waterLevel;
    }

    /**
     * @brief returns the string representation of the sensor values from the last sensor readout 
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
    volatile bool shortPressed = false; // gets set true when button was pressed shortly
    volatile bool longPressed = false; // true when button was pressed for at least 3 seconds
    hw_timer_t *btnTimer = NULL;

    /**
     * @brief Once the button is pressed this interrupt service routine get called peroidically
     * every BTN_SAMPLING_RATE and checks if the button is stilled pressed to determine if it
     * is a long button press or just a single button tap. Every button press lasting shorter
     * than BTN_SAMPLING_RATE is not detected. 
     */
    static void IRAM_ATTR periodicButton() {
        if (digitalRead(BUTTON) == HIGH) {
            cnt++;
            if (cnt == 30) longPressed = true;
            // programming hint: use equal-operator (==) and not bigger-operator (>)
            // to prevent resetting the flag multiple times!
        } else if (btnTimer) { // button not pressed anymore
            if (1 < cnt && cnt < 30) shortPressed = true;
            
            timerEnd(btnTimer); // stop button the sampling
            btnTimer = NULL;
            cnt = 0;
        } else { /* should not be reached !*/ }
    }

    /**
     * @brief initializes the button pin and attaches the ISR to handle the rising edge on
     * the button input pin
     */
    void init() {
        pinMode(BUTTON, INPUT);
        attachInterrupt(BUTTON, ISR, RISING); // interrupt on change
    }

    /**
     * @brief called when a rising edge is detected in the button input pin. This means that the
     * button is pressed and therefore the periodic button sampling is stated. The sampling ISR
     * done by calling the periodicButton() function every BTN_SAMPLING_RATE and checking the
     * state of the button input pin.
     */
    void ISR() {
        btnTimer = timerBegin(1, 80, true); // initialize timer1
        timerAttachInterrupt(btnTimer, &periodicButton, true);
        timerAlarmWrite(btnTimer, BTN_SAMPLING_RATE, true);
        timerAlarmEnable(btnTimer);
    }

    /**
     * @brief indicates if a short press occured
     * @return true if a button short press was detected
     */
    bool isShortPressed() {
        return shortPressed;
    }

    /**
     * @brief indicates if a long press occured
     * @return true if a button long press was detected
     */
    bool isLongPressed() {
        return longPressed;
    }

    void resetShortFlag() {
        shortPressed = false;
    }

    void resetLongFlag() {
        longPressed = false;
    }
}


//===============================================================================================
// RELAIS
//===============================================================================================
namespace Relais {
    op_mode_t operatingMode = SCHEDULED;
    op_mode_t operatingModeChached = SCHEDULED;
    interval_t intervals[MAX_INTERVALLS];

    /**
     * @brief Initializes the relais pin to controll the connected N-FET and switch the relais.
     */
    void init() {
        pinMode(RELAIS, OUTPUT);
        digitalWrite(RELAIS, LOW);
    }

    /**
     * @brief turns the relais output pin on
     */
    void turnOn() {
        digitalWrite(RELAIS, HIGH);
        Led::turnOn(Led::YELLOW);
    }

    /**
     * @brief turns the relais output pin off
     */
    void turnOff() {
        digitalWrite(RELAIS, LOW);
        Led::turnOff(Led::YELLOW);
    }

    /**
     * @brief Toggles the relais output pin as well as the operating mode
     */
    void toggle() {
        digitalWrite(RELAIS, !digitalRead(RELAIS));
        Led::toggle(Led::YELLOW);
        if (operatingMode == MANUAL) { // reset operating mode to original mode
            operatingMode = operatingModeChached;
        } else { // chache original mode for later reset
            operatingModeChached = operatingMode;
            operatingMode = MANUAL;
        }
    }

    /**
     * @brief sets the operating mode of the relais to the given param
     * @param mode operating mode to be set
     */
    void setOpMode(op_mode_t mode) {
        operatingMode = mode;
    }

    /**
     * @brief get the current operating mode of the relais
     * @return operating mode of the relais
     */
    op_mode_t getOpMode() {
        return operatingMode;
    }

    /**
     * @brief sets the given interval at the given index
     * @param interval interval to be set at the timed scheduled
     * @param i index of the interval array
     */
    void setInterval(interval_t interval, unsigned int i) {
        intervals[i] = interval;
    }

    /**
     * @brief Getter method for intervals of relais
     * @param i index of the interval to be returned
     * @return interval with the given index
     */
    interval_t getInterval(unsigned int i) {
        return intervals[i];
    }

    /**
     * @brief checks weather the time passed is inside an interval
     * @param timeinfo time to check for intervals
     * @return true, when the given time is inside an interval
     */
    bool checkIntervals(tm timeinfo) {
        for (unsigned int i = 0; i < MAX_INTERVALLS; i++) {
            if (timeinfo.tm_hour >= intervals[i].start.tm_hour && timeinfo.tm_min >= intervals[i].start.tm_min
                && timeinfo.tm_hour <= intervals[i].stop.tm_hour && timeinfo.tm_min <= intervals[i].stop.tm_min) {
                if (intervals[i].wday & (1 << timeinfo.tm_wday)) {
                    return true;
                }
            }
        }
        return false;
    }
}