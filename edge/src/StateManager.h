#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "Arduino.h"

namespace StateManager {

#define MAX_TASKS 8

typedef enum {
    COLD = 0,
    WARM = 1,
    HOT = 2
} system_state_t;

bool init(void);
int registerTask(void);
bool requestStateChange(size_t taskID, system_state_t newState);
EventBits_t waitForChange(size_t taskID, TickType_t xFrequency);
system_state_t getState(void);

}

#endif /* STATE_MANAGER_H */
