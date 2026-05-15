#include "StateManager.h"

namespace StateManager {

static volatile system_state_t state = COLD;
static system_state_t requestedStates[MAX_TASKS];
static SemaphoreHandle_t semaphore;
static EventGroupHandle_t eventGroup;

bool init(void) {
    semaphore = xSemaphoreCreateMutex();
    if(semaphore == NULL) {
        log_e("Not enough heap to use state semaphore");
        return false;
    }

    eventGroup = xEventGroupCreate();
    if(eventGroup == NULL) {
        log_e("Not enough heap to use state event group");
        return false;
    }

    for(size_t i = 0; i < MAX_TASKS; i++) {
        requestedStates[i] = COLD;
    }
    return true;
}

/**
 * @brief Reserves a place in the state management for a new task and returns its task id. The
 * maximum number of tasks that can be registered is set by MAX_TASKS. The task id is used to
 * identify this task in other function call. It should be used with setState().
 * @return task id of the registered task or -1 if the maximum number of registered tasks is
 * reached
 */
int registerTask(void) {
    static int taskID = 0;
    if(taskID >= MAX_TASKS) {
        return -1;
    }
    return taskID++; // increment on return
}

/**
 * @brief Request a state change from a task with the given ID. Keep in mind that a successful
 * request does not mean a state was actually updated. It means the request was submitted
 * succesfully not the state changed successfully. If other tasks requested a "hotter" state,
 * their request wins. But your request was still successful. If all tasks can agree on a
 * "cooler" state, the state gets updated to this one. 
 * @param taskID ID of the task requesting an update, as returned by registerTask()
 * @param newState requested state
 * @return true if the request was submitted successfully, false otherwise
 */
bool requestStateChange(size_t taskID, system_state_t newState) {
    // Enter Critical Section:
    if(!xSemaphoreTake(semaphore, portMAX_DELAY)) {
        log_w("Failed to take state semaphore");
        return false;
    }

    // Update Requested State:
    requestedStates[taskID] = newState;

    // Update Actual State:
    EventBits_t bitmask = 0x00;
    system_state_t hottest_state = COLD;
    for(int i = 0; i < MAX_TASKS; i++) {
        bitmask = bitmask | (0x01 << i);
        if(requestedStates[i] > hottest_state) {
            hottest_state = requestedStates[i];
        }
    }
    if(hottest_state != state) {
        state = hottest_state;
        xEventGroupSetBits(eventGroup, bitmask);
        log_d("System state updated to %d", (int)state);
    }

    // Exit Critical Section:
    xSemaphoreGive(semaphore);
    return true;
}

/**
 * @brief Blocking wait until the state changes for a task with the given ID. If any change
 * occured for the task it returns true. If the blocking wait timedout and waited longer then
 * the given frequency, it returns false. If it returns true once, the change bit for the given
 * task is cleared and will return false until it is set again.
 * @param taskID ID of the task waiting got change, as returned by registerTask()
 * @param xFrequency The maximum amount of time (specified in 'ticks') to wait to become set
 * @return true if the change happend before a timeout, false otherwise
 */
EventBits_t waitForChange(size_t taskID, TickType_t xFrequency) {
    /**
     * [INFO]
     * The system call xEventGroupWaitBits() blocks and waits for the STATE_CHANGED_BIT of the
     * event group to be set. It waits for maximum time of xFrequency ticks. If the tasks
     * resumes its either because of two things:
     * 1. The event bit was set, meaning the system state changed
     * 2. It timedout, meaning the state did not change in the meantime
     */
    EventBits_t bitmask = 0x01 << taskID; // used to select bits in event group
    log_v("Task with bitmask 0x%02X waiting", bitmask);
    EventBits_t uxBits = xEventGroupWaitBits(eventGroup, bitmask, pdTRUE, pdFALSE, xFrequency);
    return uxBits & bitmask;
}

/**
 * @brief Get the current system state. This is the "coldest" state all tasks can agree on. If
 * the current state could not be read. 
 * @return Current state, returns the "hottest" state as a fallback in case of an error 
 */
system_state_t getState(void) {
    // Enter Critical Section:
    if(!xSemaphoreTake(semaphore, portMAX_DELAY)) {
        log_w("Failed to take state semaphore");
        log_w("Returning hottest state as fallback");
        return HOT; // return hottest state as fallback
    }

    // Read State:
    system_state_t current_state = state;

    // Exit Critical Section:
    xSemaphoreGive(semaphore);

    return current_state;
}

}