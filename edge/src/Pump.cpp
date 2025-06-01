#include "Pump.h"

/**
 * Constructor initalizes all outputs with the pin numbers defined in "Pump.h" and sets all
 * intervals to default values.
 */
PumpClass::PumpClass() : relais(RELAIS), led(LED_YELLOW) {
    this->relais.off();
    this->led.off();
    this->operatingMode = SCHEDULED;
    this->cachedOperatingMode = SCHEDULED;
    this->scheduledState = false;
    this->threshold = 0;
}

/**
 * @brief Toggles the state of the waterpump
 */
void PumpClass::toggle() {
    bool state = this->relais.toggle();
    if(state) {
        this->led.on();
    } else {
        this->led.off();
    }
}

/**
 * @brief Pauses the (scheduled) operating pump, manual mode instead
 */
void PumpClass::pauseSchedule() {
    if (operatingMode != MANUAL) { // only set mode if it is not paused already
        this->cachedOperatingMode = this->operatingMode;
        this->operatingMode = MANUAL;
    }
}

/**
 * @brief Resumes the (scheduled) operating pump
 */
void PumpClass::resumeSchedule() {
    this->operatingMode = this->cachedOperatingMode;
}

/**
 * @brief Return the current threshold level of this pump
 * @return sensor threshold level
 */
int PumpClass::getThreshold() {
    return this->threshold;
}

/**
 * @brief Update the current threshold of this pump
 * @param level sensor threshold level
 */
void PumpClass::setThreshold(int level) {
    this->threshold = level;
}

/**
 * @brief Adds a new schedule interval at the given index
 * @param interval interval to add to schedule
 * @param i index to add
 */
void PumpClass::addInterval(interval_t interval) {
    this->intervals.push_back(interval);
}

/**
 * @brief Delete schedule interval at the given index
 * @param i index to delete from
 * @return true on success, false otherwise
 */
bool PumpClass::removeInterval(size_t i) {
    if(i >= this->intervals.size()) {
        return false;
    }
    std::vector<interval_t>::iterator iter = intervals.begin(); // iterator to first element
    std::next(iter, i); // advance by i steps
    this->intervals.erase(iter);
    return true;
}

/**
 * @brief Clears all exisiting intervals and adds the given intervals instead
 * @param intervals array of intervals to add
 * @return true on success, false otherwise
 */
void PumpClass::scheduleIntervals(std::vector<interval_t>& intervals) {
    this->intervals.clear();
    this->intervals = intervals;
}

/**
 * The scheduler checks if the pump is scheduled to run. If an interval is currently scheduled, it
 * switches the pump on. In AUTOMATIC mode, it checks if the water level is above the threshold. It
 * does not run the pump if there is too little water. In MANUAL mode the scheduler is disabled.
 * @param waterlevel water level from sensors
 * @return true if the pump changed state, false if no change happened
 */
bool PumpClass::scheduler(int waterlevel) {
    // Check MANUAL Mode:
    if(this->operatingMode == MANUAL) {
        return false; // no schedule in manual mode
    }

    // Get Current Time:
    tm timeinfo = Time.getTime();
    int now =  timeinfo.tm_min + 60*timeinfo.tm_hour; // current time in minutes
    
    // Get Scheduled State by Checking Intervals:
    bool newState = false; // shows which state is scheduled, maybe different from actual state
    for (interval_t interval : this->intervals) {
        int start = interval.start.tm_min + 60*interval.start.tm_hour;
        int stop = interval.stop.tm_min + 60*interval.stop.tm_hour;
        if (start <= now && now < stop) {
            if (interval.wday & (1 << timeinfo.tm_wday)) {
                newState = true;
            }
        }
    }

    // Check Scheduled State Update:
    if(newState == this->scheduledState) { // state has updated
        return false;
    }

    // Set Pump According to Updated State:
    this->scheduledState = newState;
    if(newState && (this->operatingMode != AUTOMATIC || waterlevel >= this->threshold)) {
        this->relais.on();
        this->led.on();
    } else {
        this->relais.off();
        this->led.off();
    }

    return true;
}

/**
 * @brief Create a default interval (00:00 - 00:00, no days enabled)
 * @return created interval
 */
interval_t PumpClass::defaultInterval() {
    tm start;
    start.tm_hour = 0;
    start.tm_min = 0;
    start.tm_sec = 0;
    tm stop;
    stop.tm_hour = 0;
    stop.tm_min = 0;
    stop.tm_sec = 0;
    interval_t interval = { .start = start, .stop = stop, .wday = 0 };
    return interval;
}

PumpClass Pump = PumpClass();
