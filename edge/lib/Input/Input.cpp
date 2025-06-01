#include "Input.h"

namespace Input {

Analog::Analog(uint8_t pin) {
    this->pin = pin;
    pinMode(pin, INPUT);
}

uint16_t Analog::read(void) {
    return analogRead(this->pin);
}

Digital::Digital(uint8_t pin) {
    this->pin = pin;
    pinMode(pin, INPUT);
}

bool Digital::read(void) {
    return digitalRead(this->pin) != 0;
}

Interrupted::Interrupted(uint8_t pin, VoidFunctionPointer_t isr, int mode) {
    this->pin = pin;
    this->isr = isr;
    this->mode = mode;
    pinMode(pin, INPUT);
}

void Interrupted::enable(void) {
    attachInterrupt(this->pin, this->isr, this->mode);
}

void Interrupted::disable(void) {
    detachInterrupt(this->pin);
}

bool Interrupted::read(void) {
    return digitalRead(this->pin) != 0;
}

}
