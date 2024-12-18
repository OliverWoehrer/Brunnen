#ifndef SENSORS_H
#define SENSORS_H

#include "Input.h"
#include "Output.h"
#include "TimeManager.h"

// Pin Definitions:
#define LED_BLUE 2
#define SENSOR_SWITCH 25
#define WATER_PRESSURE_SENSOR 32
#define WATER_LEVEL_SENSOR 33
#define WATER_FLOW_SENSOR 22

typedef struct {
    tm timestamp;
    int flow;
    int pressure;
    int level;
} sensor_data_t;

class SensorClass {
public:
    SensorClass();
    void begin();
    void read();
    void countEdge();
    int getWaterLevel();
private:
    Output::Digital led;
    Output::Digital sensorSwitch;
    Input::Analog waterPressure;
    Input::Analog waterLevel;
    Input::Interrupted waterFlow;
    unsigned int edgeCounter;
    sensor_data_t data;

    static void edgeCounterISR();
};

extern SensorClass Sensors;

#endif /* SENSORS_H */