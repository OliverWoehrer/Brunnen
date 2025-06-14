#include "Button.h"

/**
 * @brief Initalizes the pin defined by BUTTON. It gets sampled periodically and resumes the
 * FreeRTOS task given to setTask(), if there was a button press (short or long). 
 * @param handlerFunction function to call if the button was pressed
 */
ButtonClass::ButtonClass() : pin(BUTTON, ButtonClass::isr, ONHIGH) {
    this->cnt = 0;
    this->indicator = NO_PRESS;
    this->timer = timerBegin(1, 80, true);
    timerAlarmWrite(this->timer, BTN_SAMPLING_RATE, true);
    timerAlarmEnable(this->timer);
}

/**
 * @brief Takes the task handle and connects it to the physical button pin. This mean that the task
 * with the given button 
 * @param task 
 */
void ButtonClass::begin(TaskHandle_t task) {
    this->task = task;
    this->pin.enable();
}

/**
 * @brief Disables any further interrupts and starts periodic sampling on the button.
 */
void ButtonClass::interrupt() {
    this->pin.disable(); // disable further interrupts
    timerAttachInterrupt(this->timer, ButtonClass::periodicSampling, false); // enable periodic sampling
}

/**
 * @brief Checks if the button is still pressed. It is either a long press or just a single tap
 * (button released early). Every button press lasting shorter than BTN_SAMPLING_RATE is not
 * detected.
 */
void ButtonClass::sample() {
    if(this->pin.read()) { // button is pressed
        this->cnt++;
        if(this->cnt >= 30 && this->indicator == NO_PRESS) { // check if logn press and no indicator (=no recent button press) 
            this->indicator = LONG_PRESS;
            xTaskResumeFromISR(this->task);
        } // button not pressed long enough or recent button press
    
    } else { // button not pressed anymore (-> don't sample any longer)
        if(0 < this->cnt && this->cnt < 30) { // check if short press
            this->indicator = SHORT_PRESS;
            xTaskResumeFromISR(this->task);
        } else { // not pressed long enough or previously long press (already indicated above)
            this->indicator = NO_PRESS; // either way: no press anymore
        }
                
        // Disable Periodic Sampling:
        timerDetachInterrupt(this->timer); // disable periodic sampling
        this->pin.enable(); // re-enable interrupt on button
        
        this->cnt = 0;
    }
}

/**
 * @brief Return the button indicator
 * @return indicator indicating if/how button was pressed (not, short or long pressed)
 */
indicator_t ButtonClass::getIndicator() {
    return this->indicator;
}

/**
 * @brief Clears the button indicator to NONE (= button not pressed)
 */
void ButtonClass::resetIndicator() {
    this->indicator = NO_PRESS;
}

/**
 * @brief Gets called if there is an interupt at the button pin (rising edge)
 * @note IRAM_ATTR prefix so the code gets placed in IRAM and is faster loaded when needed 
 */
void IRAM_ATTR ButtonClass::isr() {
    Button.interrupt();
}

/**
 * @brief Once the button is pressed this interrupt service routine get called periodically at the
 * BTN_SAMPLING_RATE.
 * @note IRAM_ATTR prefix so the code gets placed in IRAM and is faster loaded when needed
 */
void IRAM_ATTR ButtonClass::periodicSampling() {
    Button.sample();
}

ButtonClass Button = ButtonClass();