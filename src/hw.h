#ifndef HW_H
#define HW_H

#include <Preferences.h>

//Global return value:
#define SUCCESS 0
#define FAILURE 1

//LED pin definitions:
#define LED_RED 4
#define LED_YELLOW 17
#define LED_GREEN 16
#define LED_BLUE 2

//Sensors:
#define WATERFLOW_SENSOR 22
#define WATER_PRESSURE_SENSOR 32
#define SENSOR_SWITCH 25
#define WATER_LEVEL_SENSOR 33

//Button:
#define BUTTON 15
#define BTN_SAMPLING_RATE 100000

//Relais:
#define RELAIS 13
#define MAX_INTERVALLS 8
#define VALUE_STRING_LENGTH 40

//File System:
#define FILE_NAME_LENGTH 21
#define SD_CARD
#define SPI_CD 5
#define SPI_MOSI 23
#define SPI_CLK 18
#define SPI_MISO 19

namespace Hardware {

namespace Leds {
    typedef enum {RED, YELLOW, GREEN, BLUE} color_t;
}

namespace Sensors {}

namespace Button {
    typedef struct {
        bool shortPressed; // gets set true when button was pressed shortly
        bool longPressed; // true when button was pressed for at least 3 seconds
    } indicator_t;
    void ISR();
}

namespace Relais {
    typedef enum {
        MANUAL,     // relais switched on manually
        PAUSED,     // pump operation paused
        SCHEDULED,  // turn on relais during scheduled intervals
        AUTOMATIC   // turn on relais during scheduled intervals only if there is enough water
    } op_mode_t;
    typedef struct {
        struct tm start;
        struct tm stop;
        unsigned char wday;
    } interval_t;
}

namespace FileSystem {}

using button_indicator_t = Button::indicator_t;
using pump_intervall_t = Relais::interval_t;
using pump_op_mode_t = Relais::op_mode_t;

int init(TaskHandle_t* buttonHandler,const char* fileName);
void setUILed(char value);
void setIndexLed(char value);
void setErrorLed(char value);

void sampleSensorValues(const char* timeString);
char* sensorValuesToString();
bool hasNominalSensorValues();

button_indicator_t getButtonIndicator();
void resetButtonFlags();

void switchPump(char value);
void togglePump();
void pauseScheduledPumpOperation();
void resumeScheduledPumpOperation();
int getPumpOperatingLevel();
void setPumpOperatingLevel(int level);
void setPumpInterval(Relais::interval_t interval, unsigned int i);
pump_intervall_t getPumpInterval(unsigned int i);
int getScheduledPumpState(tm timeinfo);


int createCurrentDataFile();
char* loadActiveDataFileName();
void deleteActiveDataFile();
int setActiveDataFile(const char* fName);

}

#endif /* HW_H */