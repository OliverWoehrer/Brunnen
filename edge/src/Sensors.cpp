#include "Sensors.h"

void edgeCounterISR() {
    Sensors.countEdge();
}

/**
 * Constructor initalizes all member sensor objects with the pin number defined in "Sensors.h"
 */
SensorClass::SensorClass() : led(LED_BLUE), sensorSwitch(SENSOR_SWITCH), waterPressure(WATER_PRESSURE_SENSOR), waterLevel(WATER_LEVEL_SENSOR), waterFlow(WATER_FLOW_SENSOR, edgeCounterISR, RISING) {
    this->edgeCounter = 0;
    this->data = {
        .timestamp = Time.getTime(),
        .flow = 0,
        .pressure = 0,
        .level = 0
    };
}

/**
 * This function reads out the sensor values when the sensors are ready. This is done by switching the power supply of the
 * water level sensor to ON and weaking it up. Afterwards it checks if the water level sensor is ready (delay of approx.
 * 360ms). The values of all sensors are then read and/or stored in a local variable (sensor readout). Then the water
 * level sensor is being disabled again. The values are all read at the same time to ensure data consistency. Afterwards
 * the sensor values together with the given timestring are written to the (currently active) data file. The index led is
 * also switched on to indicate data sampling.
 */
void SensorClass::read() {
    // Enable Sensors:
    this->led.on();
    this->sensorSwitch.on(); // power on water level sensor
    
    // Wait Sensor Wake Up:
    vTaskDelay((360+10) / portTICK_PERIOD_MS); // water level sensor needs approx. 360ms
    
    // Read Sensor Values:
    this->data.timestamp = Time.getTime();
    this->data.flow = this->edgeCounter;
    this->edgeCounter = 0; // reset edge counter
    this->data.pressure = this->waterPressure.read();
    this->data.level = this->waterLevel.read();

    // Disable Sensors:
    this->led.off();
    this->sensorSwitch.off(); // disable water level sensor again
    
    // Store Sensor Values:
    DataFile.store(this->data);
}

/**
 * Increments the edge counter of 'this' by one. This is usually called from the ISR, attached to
 * the input of the flow sensors. SO if an rising edge is detected, the edge counter is incremented
 */
void SensorClass::countEdge() {
    this->edgeCounter += 1;
}

/**
 * Return the water level from the last sensor read out
 * @return raw sensor value, expected to be between 0...1024
 */
int SensorClass::getWaterLevel() {
    return this->data.level;
}

SensorClass Sensors = SensorClass();