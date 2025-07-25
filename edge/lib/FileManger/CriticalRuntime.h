#ifndef CRITICAL_RUNTIME
#define CRITICAL_RUNTIME

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class CriticalRuntime {
private:
    SemaphoreHandle_t semaphore;
    bool valid;
public:
    CriticalRuntime(SemaphoreHandle_t semaphore);
    ~CriticalRuntime();
    bool isValid();
};

#endif /* CRITICAL_RUNTIME */