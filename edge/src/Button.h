#ifndef BUTTON_H
#define BUTTON_H

#include "Arduino.h"
#include "Input.h"

// Pin Definitions:
#define BUTTON 15

// Configuration:
#define BTN_SAMPLING_RATE 100000 // in microseconds

typedef enum {
    NO_PRESS,   // was not pressed
    SHORT_PRESS,  // was pressed shortly, between BTN_SAMPLING_RATE and 30*BTN_SAMPLING_RATE
    LONG_PRESS    // was pressed at least 30*BTN_SAMPLING_RATE
} indicator_t;

class ButtonClass {
public:
    ButtonClass();
    void begin(TaskHandle_t task);
    void interrupt();
    void sample();
    indicator_t getIndicator();
    void resetIndicator();
private:
    Input::Interrupted pin;
    TaskHandle_t task;
    hw_timer_t *timer;
    size_t cnt = 0;
    indicator_t indicator;

    static void IRAM_ATTR isr();
    static void IRAM_ATTR periodicSampling();
};

extern ButtonClass Button;

#endif /* BUTTON_H */