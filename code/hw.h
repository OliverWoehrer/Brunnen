#ifndef HW_H
#define HW_H

//Global return value:
#define SUCCESS 0
#define FAILURE 1

//LED pin definitions:
#define LED_RED 4
#define LED_YELLOW 17
#define LED_GREEN 16
#define LED_BLUE 2

//Sensors
#define WATERFLOW_SENSOR 22
#define WATER_PRESSURE_SENSOR 32
#define SENSOR_SWITCH 25
#define WATER_LEVEL_SENSOR 33

//Button
#define BUTTON 15
#define BTN_SAMPLING_RATE 100000

//Relais
#define RELAIS 13
#define MAX_INTERVALLS 8
#define VALUE_STRING_LENGTH 40


//===============================================================================================
// LED
//===============================================================================================
namespace Led {
    typedef enum {RED, YELLOW, GREEN, BLUE} led_color_t;
    void init();
    void turnOn(led_color_t color);
    void turnOff(led_color_t color);
    void toggle(led_color_t color);
}


//===============================================================================================
// SENSORS
//===============================================================================================
namespace Sensors {
    void init();
    void requestValues();
    bool hasValuesReady();
    bool readValues();
    int getWaterLevel();
    char* toString();
};


//===============================================================================================
// BUTTON
//===============================================================================================
namespace Button {
    extern volatile bool shortPressed; // gets set true when button was pressed shortly
    extern volatile bool longPressed; // true when button was pressed for at least 3 seconds
    void init();
    void ISR();
    bool isShortPressed();
    bool isLongPressed();
    void resetShortFlag();
    void resetLongFlag();
}


//===============================================================================================
// RELAIS
//===============================================================================================
namespace Relais {
    typedef struct {
        struct tm start;
        struct tm stop;
        unsigned char wday;
    } interval_t;

    typedef enum {
        MANUAL,     // toggle relais only when toggle() function is called
        SCHEDULED,  // turn on relais during scheduled intervals
        AUTOMATIC   // turn on relais during scheduled intervals only if there is enough water
    } op_mode_t;

    void init();
    void turnOn();
    void turnOff();
    void toggle();
    void setOpMode(op_mode_t mode);
    op_mode_t getOpMode();
    void setInterval(interval_t interval, unsigned int i);
    interval_t getInterval(unsigned int i);
    bool checkIntervals(tm timeinfo);
}

#endif /* HW_H */