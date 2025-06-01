#include "Sensors.h"
#include "DataFile.h"
#include <string>

/**
 * Constructor initalizes all member sensor objects with the pin number defined in "Sensors.h"
 */
SensorClass::SensorClass() : sensorSwitch(SENSOR_SWITCH), waterPressure(WATER_PRESSURE_SENSOR), waterLevel(WATER_LEVEL_SENSOR), waterFlow(WATER_FLOW_SENSOR, SensorClass::edgeCounterISR, RISING) {
    this->edgeCounter = 0;
    TimeManager::fromDateTimeString("1970-01-01T00:00:00", this->data.timestamp);
    this->data.flow = 0;
    this->data.pressure = 0;
    this->data.level = 0;
}

/**
 * @brief Enables the continues sampling of edges of the water flow sensor
 */
void SensorClass::begin() {
    this->waterFlow.enable();
}

/**
 * This function reads out the sensor values when the sensors are ready. This is done by switching the power supply of the
 * water level sensor to ON and weaking it up. Afterwards it checks if the water level sensor is ready (delay of approx.
 * 360ms). The values of all sensors are then read and/or stored in a local variable (sensor readout). Then the water
 * level sensor is being disabled again. The values are all read at the same time to ensure data consistency. Afterwards
 * the sensor values together with the given timestring are written to the (currently active) data file.
 */
void SensorClass::read() {
    // Enable Sensors:
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

void SensorClass::edgeCounterISR() {
    Sensors.countEdge();
}

SensorClass Sensors = SensorClass();