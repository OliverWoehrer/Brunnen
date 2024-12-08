#include "Button.h"

/**
 * @brief Gets called if there is an interupt at the button pin (rising edge)
 * @note IRAM_ATTR prefix so the code gets placed in IRAM and is faster loaded when needed 
 */
void IRAM_ATTR isr() {
    Button.interrupt();
}

/**
 * @brief Once the button is pressed this interrupt service routine get called periodically at the
 * BTN_SAMPLING_RATE.
 * @note IRAM_ATTR prefix so the code gets placed in IRAM and is faster loaded when needed
 */
void IRAM_ATTR periodicSampling() {
    Button.sample();
}

/**
 * @brief Initalizes the pin defined by BUTTON. It gets sampled periodically and resumes the
 * FreeRTOS task given to setTask(), if there was a button press (short or long). 
 * @param handlerFunction function to call if the button was pressed
 */
ButtonClass::ButtonClass() : pin(BUTTON, isr, ONHIGH) {
    this->cnt = 0;
    this->indicator = NONE;
    this->timer = timerBegin(1, 80000, true);
    timerAlarmWrite(this->timer, BTN_SAMPLING_RATE, true);
    timerAlarmEnable(this->timer);
    // TODO: use Milliseconds as sampling rate, change prescaler to 80k
    this->task = NULL;
}

/**
 * @brief Sets the task handle and enables interrupts to resume the given task.
 * @param task handle to a FreeRTOS task thats resumed if the button was pressed
 */
void ButtonClass::setTask(TaskHandle_t* task) {
    this->task = task;
    this->pin.enable();
}

/**
 * @brief Disables any further interrupts and starts periodic sampling on the button.
 */
void ButtonClass::interrupt() {
    this->pin.disable(); // disable further interrupts
    timerAttachInterrupt(this->timer, periodicSampling, false); // enable periodic sampling
}

/**
 * @brief Checks if the button is still pressed. It is either a long press or just a single tap
 * (button released early). Every button press lasting shorter than BTN_SAMPLING_RATE is not
 * detected.
 */
void ButtonClass::sample() {
    if(this->pin.read()) { // button is pressed
        this->cnt++;
        if(this->cnt < 30) { // check if no long press
            return; // return early if not long pressed
        }
    } // at this point button is not pressed anymore or long pressed (-> don't sample any longer)

    if(this->cnt > 0) {
        this->indicator = this->cnt < 30 ? SHORT : LONG;
        xTaskResumeFromISR(this->task);
    } // else: button not pressed long enough
        
    // Disable Periodic Sampling:
    timerDetachInterrupt(this->timer);
    this->cnt = 0;

    // Re-Enable Interrupt on Button:
    this->pin.enable();
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
    this->indicator = NONE;
}

ButtonClass Button = ButtonClass();