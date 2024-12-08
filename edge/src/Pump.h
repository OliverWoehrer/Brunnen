#ifndef PUMP_H
#define PUMP_H

#include "TimeManager.h"
#include "Output.h"
#include <list>

// Pin Definitions:
#define RELAIS 13
#define LED_YELLOW 17

// Intervals:
#define MAX_INTERVALLS 8

typedef struct {
    tm start;
    tm stop;
    uint8_t wday;
} interval_t;

typedef enum {  // operation mode of pump
    MANUAL,     // switch pump manually
    SCHEDULED,  // switch pump on for scheduled intervals
    AUTOMATIC   // switch pump on for scheduled intervals if there is enough water
} op_mode_t;

class Pump {
public:
    Pump();
    void togglePump();
    void pauseSchedule();
    void resumeSchedule();
    int getThreshold();
    void setThreshold(int level);
    void addInterval(interval_t interval);
    bool removeInterval(size_t i);
    void scheduleIntervals(std::list<interval_t>& intervals);
    bool scheduler(int waterlevel);
    
    static interval_t defaultInterval();
private:
    Output::Digital relais;
    Output::Digital led;
    op_mode_t operatingMode = SCHEDULED;
    op_mode_t cachedOperatingMode = SCHEDULED;
    bool scheduledState;
    int threshold = 0;
    std::list<interval_t> intervals;
};

#endif /* PUMP_H */