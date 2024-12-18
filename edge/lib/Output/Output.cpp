#include "Output.h"

namespace Output {

/**
 * Constructor sets the given pin to output state
 * @param pin pin number to use
 */
Analog::Analog(uint8_t pin) {
    this->pin = pin;
    pinMode(pin, OUTPUT);
}

/**
 * Sets the output level to the given value
 * @param value value correspoding to analog integer
 */
void Analog::set(int value) {
    analogWrite(this->pin, value);
}

/**
 * Constructor sets the given pin to output state
 * @param pin pin number to use
 */
Digital::Digital(uint8_t pin) {
    this->pin = pin;
    pinMode(pin, OUTPUT);
}

/**
 * Sets the output level to HIGH
 */
void Digital::on() {
    digitalWrite(this->pin, HIGH);
}

/**
 * Sets the output level to LOW
 */
void Digital::off() {
    digitalWrite(this->pin, LOW);
}

/**
 * @brief Inverts the current output level. So HIGH becomes LOW and vice versa. Returns the new
 * state after toggle
 * @return true if HIGH, false if LOW
 */
bool Digital::toggle() {
    int state = digitalRead(this->pin); // invert current state
    digitalWrite(this->pin, !state);
    return !state; // return new state
}

}