#ifndef CONFIG_H
#define CONFIG_H

#include "time.h"
#include "Pump.h"
#include "Preferences.h"
#include <vector>

#define CONFIG_NAME "brunnen"

class ConfigClass {
public:
    ConfigClass();

    void storePumpInterval(interval_t interval, size_t index);
    void storePumpIntervals(std::vector<interval_t>& intervals);
    interval_t loadPumpInterval(size_t index);
    void loadPumpIntervals(std::vector<interval_t>& intervals);
    void deletePumpInterval(size_t index);

    void storeJobLength(size_t jobLength);
    size_t loadJobLength();
    void storeJob(const char* fileName, size_t index);
    std::string loadJob(size_t index);
    void deleteJob(size_t index);
    
    void storeRainThresholdLevel(uint8_t level);
    uint8_t loadRainThresholdLevel();
    
    void storeMailAddress(const char* address);
    std::string loadMailAddress();
    void storeMailPassword(const char* pw);
    std::string loadMailPassword();

    void storeAPIHost(const char* name);
    std::string loadAPIHost();
    void storeAPIPort(size_t port);
    size_t loadAPIPort();
    void storeAPIPath(const char* name);
    std::string loadAPIPath();
    void storeAPIUsername(const char* name);
    std::string loadAPIUsername();
    void storeAPIPassword(const char* pw);
    std::string loadAPIPassword();

    void storeFirmwareVersion(const char* version);
    std::string loadFirmwareVersion();
private:
    Preferences preferences;
    SemaphoreHandle_t semaphore;
};

extern ConfigClass Config;

#endif /* CONFIG_H */