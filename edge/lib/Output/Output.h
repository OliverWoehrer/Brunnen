#ifndef OUTPUT_H
#define OUTPUT_H

#include "Arduino.h"

namespace Output {

class Analog {
private:
    uint8_t pin;
public:
    Analog(uint8_t pin);
    void set(int value);
};

class Digital {
private:
    uint8_t pin;
public:
    Digital(uint8_t pin);
    void on(void);
    void off(void);
    bool toggle(void);
};

class Runtime {
private:
    Digital output;
public:
    Runtime(Digital& output);
    ~Runtime();
};

}

#endif /* OUTPUT_H */