#ifndef CONFIG_H
#define CONFIG_H

#include "time.h"
#include "Pump.h"
#include "Preferences.h"
#include <vector>

#define CONFIG_NAME "brunnen"

class Config {
public:
    Config();

    void storePumpInterval(interval_t interval, size_t index);
    void storePumpIntervals(std::vector<interval_t>& intervals);
    interval_t loadPumpInterval(size_t index);

    void storeJobLength(size_t jobLength);
    size_t loadJobLength();
    void storeJob(const char* fileName, size_t index);
    std::string loadJob(size_t index);
    void deleteJob(size_t index);
    
    void storeRainThresholdLevel(uint8_t level);
    uint8_t loadRainThresholdLevel();
    
    void storePassword(const char* pw);
    std::string loadPassword();
    void storeAuthKey(const char* key);
    std::string loadAuthKey();
private:
    Preferences preferences;
    SemaphoreHandle_t semaphore;
};

#endif /* CONFIG_H */