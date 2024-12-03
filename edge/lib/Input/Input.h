#ifndef INPUTS_H
#define INPUTS_H

#include "Arduino.h"

namespace Input {

class Analog {
private:
    uint8_t pin;
public:
    Analog(uint8_t pin);
    uint16_t read(void);
};

class Digital {
private:
    uint8_t pin;
public:
    Digital(uint8_t pin);
    bool read(void);
};

class Interrupted {
public:
    typedef void(*VoidFunctionPointer_t)();
private:
    uint8_t pin;                // pin number
    VoidFunctionPointer_t isr;  // function pointer to interrupt service routine (ISR)
    int mode;                   // interrupt mode, e.g. ONHIGH. See "Arduino.h" for more details
public:
    Interrupted(uint8_t pin, VoidFunctionPointer_t isr, int mode);
    void enable(void);
    void disable(void);
    bool read(void);
};

}

#endif /* INPUTS_H */