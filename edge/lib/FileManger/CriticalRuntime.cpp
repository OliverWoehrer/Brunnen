#include "CriticalRuntime.h"
#include "Arduino.h"
#include <stdexcept> // For std::runtime_error

#define MUTEX_TIMEOUT (1000/portTICK_PERIOD_MS) // 1000 ms

/**
 * @brief Constructor tries to take the given semaphore. If it succeeds the method "isValid()" will
 * return true afterwards and false otherwise. The semaphore is freed on destruction.
 * @param semaphore_handle Semaphore to take
 */
CriticalRuntime::CriticalRuntime(SemaphoreHandle_t semaphore_handle) : semaphore(semaphore_handle) {
    // Get Mutex Semaphore:
    if(!xSemaphoreTake(this->semaphore, MUTEX_TIMEOUT)) { // blocking wait
        this->valid = false;
        return;
    }
    this->valid = true;
}

/**
 * @brief Destuctor tries to free the semaphore
 */
CriticalRuntime::~CriticalRuntime(void) {
    if(!xSemaphoreGive(this->semaphore)) { // clean up, give back mutex semaphore
        log_d("Failed to give semaphore");
    }
}

/**
 * @brief Tells if the semaphore was successfully taken at construction of the object
 * @return true if the semaphore is taken successfully, false otherwise
 */
bool CriticalRuntime::isValid(void) {
    return this->valid;
}