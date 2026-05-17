/**
 * This component implements a time picker. The time value can be incremented and decremented with
 * buttons or manually entered. The is always a confirmed time and a selected time. The selected
 * time is the last time, the user put in. The selected time becomes confirmed when the user clicks
 * on the 'confirm' button. If the user clicks on the 'cancel' button the selected time gets reset
 * to the last confirmed time. This allows the user to discard any changes they made setting a new time selecting a new time.
 *  
 * [Attributes]
 * Attributes are used to set or change the default behavior of the component
 * "time":              The initial date that is displayed when the component is loaded. Has to be
 *                      a string in ISO format YYYY-MM-DDThh:mm:ss. If the initial date is not set, the
 *                      current date is used.
 *
 * 
 * [Events]
 * The component emits events every time the user selects, confirms or discards(=reset) a time. Use
 * the 'select' and 'reset' event to update other parts of the UI, to reflect visual changes. Use
 * the 'confirm' event to update functional parts of the application.
 * 
 * Here is a list of all events:
 * "select":    Emitted when the user sets a time or the user discards any selection.
 * "confirmed": Emitted when a time is confirmed. This usually means the user has decided on a time
 *              and clicked on the 'confirm' element.
 * "reset":     Emitted when the user discards any changes. This means the user clicked the
 *              'cancel' element.
 * 
 * 
 * [Slots]
 * The component offers control elements for example to increment hours, confirm a selection, etc.
 * when they are clicked. These control elements can be slotted, which means custom component
 * libraries can be used to implement click-able elements. The available slots and what happens
 * when they are clicked by the user, can be seen below. If no element is slotted, a default
 * <button> element is created instead.
 * 
 * All available slots are:
 * "supporting-text":   Text to be displayed at the top, to help the user
 * "headline":          Element to display currently selected time
 * "hours":             Input element to display the selected hours
 * "inc-hours":         The hours are incremented when this element is clicked
 * "dec-hours":         The hours are decremented when this element is clicked
 * "minutes":           Input element to display the selected minutes
 * "inc-minutes":       The minutes are incremented when this element is clicked
 * "dec-minutes":       The minutes are decremented when this element is clicked
 * "seconds":           Input element to display the selected seconds
 * "inc-seconds":       The seconds are incremented when this element is clicked
 * "dec-seconds":       The seconds are decremented when this element is clicked
 * "millis":            Input element to display the selected milliseconds
 * "inc-millis":        The milliseconds are incremented when this element is clicked
 * "dec-millis":        The milliseconds are decremented when this element is clicked
 * "cancel-btn":        The selected time is reset to the last confirmed time, when this element is
 *                      clicked. The 'reset' event is emitted.
 * "confirm-btn"        The selected time is confirmed when this element is clicked. The 'confirm'
 *                      event is emitted.
 * 
 * 
 * 
 *                      ╔═══════════════════════════════════════════════╗
 *                      ║                                               ║
 *                      ║  Select a time ◀──slot=supporting-text        ║
 *                      ║  13:37:01.101 ◀───slot=headline               ║
 *                      ║                                               ║
 *                      ║  Hours:          Minutes:      Seconds:       ║
 *                      ║  ┌────────┐┌─┐  ┌────────┐┌─┐  ┌────────┐┌─┐  ║
 * slot=hours───────────║─▶│        ││^│  │        ││^│  │        ││^│◀─║──slot=inc-seconds
 *                      ║  │13      │└─┘  │37      │└─┘  │01      │└─┘  ║
 *                      ║  │        │┌─┐  │        │┌─┐  │        │┌─┐  ║
 *                      ║  │        ││v│  │        ││v│  │        ││v│◀─║──slot=dec-seconds
 *                      ║  └────────┘└─┘  └────────┘└─┘  └────────┘└─┘  ║
 *                      ║  Milliseconds:                                ║
 *                      ║  ┌──────────────────────────────────────┐┌─┐  ║
 *                      ║  │                                      ││^│◀─║──slot=inc-millis
 *                      ║  │101                                   │└─┘  ║
 *                      ║  │                                      │┌─┐  ║
 *                      ║  │                                      ││v│◀─║──slot=dec-millis
 *                      ║  └──────────────────────────────────────┘└─┘  ║
 *                      ║                       ┌────────┐ ┌─────────┐  ║
 * slot=cancel-btn──────║──────────────────────▶│ Cancel │ │ Confirm │◀─║──slot=confirm-button
 *                      ║                       └────────┘ └─────────┘  ║
 *                      ╚═══════════════════════════════════════════════╝
 * 
 * [Usage]
 * This shows example usage of this component. If you dont want to use slotted elements, just use
 * an empty element tag (<time-picker time="1970-01-01T13:37:01.101"><time-picker/>):
 *  <html>
 *  <head>
 *      <script defer src="TimePicker.js"></script>
 *  </head>
 *  <body>
 *      <time-picker time="1970-01-01T13:37:01.101">
 *          <my-button slot="inc-hours">Up</my-button>
 *          <my-button slot="dec-hours">Down</my-button>
 *          <my-button slot="cancel-btn">Cancel</my-button>
 *          <my-button slot="confirm-btn">OK</my-button>
 *      </time-picker>
 *  </body>
 *  <script>
 *      window.onload = () => {
 *          const timePicker = document.querySelector("time-picker");
 *          if(timePicker) {
 *              timePicker.addEventListener("select", () => { console.log("onselect"+timePicker.selectedDateObj); });
 *              timePicker.addEventListener("confirm", () => { console.log("onconfirm"+timePicker.confirmedDateObj); });
 *              timePicker.addEventListener("reset", () => { console.log("onreset"); });
 *          }
 *      }
 *  </script>
 *  <html>
 * 
 * 
 */
class TimePicker extends HTMLElement {
    //////////////////////////////////////////////////////////////////////////////////////////
    // Static Template:
    //////////////////////////////////////////////////////////////////////////////////////////
    static observedAttributes = ["time", "confirm-on-select"];
    static templateString = `
    <style>
        :host {
            --grid-margin: 10px;
            display: block;
            width: 100%;
            max-width: 100%;
        }
        header {
            margin-bottom: 12px;
        }
        main {
            align-items: center;
            box-sizing: border-box;
            display: flex;
            flex-flow: row wrap;
            gap: var(--grid-margin);
            justify-content: space-between;
            width: 100%;
        }
        main > div {
            width: 31%;
        }
        input {
            box-sizing: border-box;
            max-width: vw;
            padding: 0;
            width: 100%;
        }
        input::-webkit-outer-spin-button,
        input::-webkit-inner-spin-button { /* hide default buttons of number input */
            -webkit-appearance: none;
            margin: 0;
        }
        input[type=number] { /* hide default buttons of number input */
            -moz-appearance: textfield;
        }
        footer {
            display: flex;
            gap: var(--grid-margin);
            margin: var(--grid-margin) 0;
            justify-content: flex-end;
        }
        .flex-row {
            align-items: stretch;
            display: flex;
            flex-flow: row nowrap;
            gap: var(--grid-margin);
            height: 100%;
            justify-content: space-between;
        }
        .flex-column {
            align-items: center;
            flex-flow: column nowrap;
            display: flex;
            gap: var(--grid-margin);
            justify-content: space-between;
        }
    </style>
    <header>
        <div style="opacity: 0.4">
            <slot name="supporting-text">Select Time</slot>
        </div>
        <div>
            <slot name=headline><span></span></slot>
        </div>
    </header>
    <main>
        <div>
            <div>Hours:</div>
            <div class="flex-row">
                <slot name="hours"><input id="hours" type="number" value="0"></slot>
                <div class="flex-column">
                    <slot name="inc-hours"><button>&#129169;</button></slot>
                    <slot name="dec-hours"><button>&#129171;</button></slot>
                </div>
            </div>
        </div>
        <div>
            <div>Minutes:</div>
            <div class="flex-row">
                <slot name="minutes"><input id="minutes" type="number" value="0"></slot>
                <div class="flex-column">
                    <slot name="inc-minutes"><button>&#129169;</button></slot>
                    <slot name="dec-minutes"><button>&#129171;</button></slot>
                </div>
            </div>
        </div>
        <div>
            <div>Seconds:</div>
            <div class="flex-row">
                <slot name="seconds"><input id="seconds" type="number" value="0"></slot>
                <div class="flex-column">
                    <slot name="inc-seconds"><button>&#129169;</button></slot>
                    <slot name="dec-seconds"><button>&#129171;</button></slot>
                </div>
            </div>
        </div>
        <div style="width: 100%;">
            <div>Milliseconds:</div>
            <div class="flex-row">
                <slot name="millis"><input id="millis" type="number" value="0"></slot>
                <div class="flex-column">
                    <slot name="inc-millis"><button>&#129169;</button></slot>
                    <slot name="dec-millis"><button>&#129171;</button></slot>
                </div>
            </div>
        </div>
    </main>
    <footer>
        <slot name="cancel-btn"><button>Cancel</button></slot>
        <slot name="confirm-btn"><button>OK</button></slot>
    </footer>
    `

    //////////////////////////////////////////////////////////////////////////////////////////
    // Web Component Lifecycle Hooks:
    //////////////////////////////////////////////////////////////////////////////////////////


    constructor() {
        super()
        // Set Default Properties:
        this.initDate = null; // use current datetime
        this.confirmOnSelect = false;

        // Setup Shadow DOM:
        const template = document.createElement("template");
        if(!TimePicker.templateString) { throw new Error("No template found"); }
        template.innerHTML = TimePicker.templateString;
        this.shadow = this.attachShadow({ mode:"closed" });
        this.shadow.append(template.content.cloneNode(true));
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if(name === "time") {
            if(!this.#isValidDatetimeString(newValue)) { //  check valid datetime string
                throw new Error(`Attribute '${name}' has an invalid string format: "${newValue}", use ISO format YYYY-MM-DDThh:mm:ss.zzz`);
            }
            this.confirmedDateObj = new Date(newValue);
        } else if(name === "confirm-on-select") {
            this.confirmOnSelect = true;
        }
    }

    connectedCallback() {
        // Initialize Slotted Elements:
        const headlineSlot = this.shadow.querySelector("slot[name='headline']");
        this.headlineText = headlineSlot.assignedNodes({ flatten:true })[0];

        const hoursSlot = this.shadow.querySelector("slot[name='hours']");
        this.hoursInput = hoursSlot.assignedNodes({ flatten:true })[0];
        this.hoursInput.oninput = () => { this.#parseHours() };

        const incHoursSlot = this.shadow.querySelector("slot[name='inc-hours']");
        const incHoursButton = incHoursSlot.assignedNodes({ flatten:true })[0];
        incHoursButton.onclick = () => { this.#updateHours(+1) };

        const decHoursSlot = this.shadow.querySelector("slot[name='dec-hours']");
        const decHoursButton = decHoursSlot.assignedNodes({ flatten:true })[0];
        decHoursButton.onclick = () => { this.#updateHours(-1) };

        const minutesSlot = this.shadow.querySelector("slot[name='minutes']");
        this.minutesInput = minutesSlot.assignedNodes({ flatten:true })[0];
        this.minutesInput.oninput = () => { this.#parseMinutes() };

        const incMinutesSlot = this.shadow.querySelector("slot[name='inc-minutes']");
        const incMinutesButton = incMinutesSlot.assignedNodes({ flatten:true })[0];
        incMinutesButton.onclick = () => { this.#updateMinutes(+1) };

        const decMinutesSlot = this.shadow.querySelector("slot[name='dec-minutes']");
        const decMinutesButton = decMinutesSlot.assignedNodes({ flatten:true })[0];
        decMinutesButton.onclick = () => { this.#updateMinutes(-1) };

        const secondsSlot = this.shadow.querySelector("slot[name='seconds']");
        this.secondsInput = secondsSlot.assignedNodes({ flatten:true })[0];
        this.secondsInput.oninput = () => { this.#parseSeconds() };

        const incSecondsSlot = this.shadow.querySelector("slot[name='inc-seconds']");
        const incSecondsButton = incSecondsSlot.assignedNodes({ flatten:true })[0];
        incSecondsButton.onclick = () => { this.#updateSeconds(+1) };

        const decSecondsSlot = this.shadow.querySelector("slot[name='dec-seconds']");
        const decSecondsButton = decSecondsSlot.assignedNodes({ flatten:true })[0];
        decSecondsButton.onclick = () => { this.#updateSeconds(-1) };

        const millisSlot = this.shadow.querySelector("slot[name='millis']");
        this.millisInput = millisSlot.assignedNodes({ flatten:true })[0];
        this.millisInput.oninput = () => { this.#parseMillis() };

        const incMillisSlot = this.shadow.querySelector("slot[name='inc-millis']");
        const incMillisButton = incMillisSlot.assignedNodes({ flatten:true })[0];
        incMillisButton.onclick = () => { this.#updateMillis(+1) };

        const decMillisSlot = this.shadow.querySelector("slot[name='dec-millis']");
        const decMillisButton = decMillisSlot.assignedNodes({ flatten:true })[0];
        decMillisButton.onclick = () => { this.#updateMillis(-1) };

        const cancelSlot = this.shadow.querySelector("slot[name='cancel-btn']");
        const cancelButton = cancelSlot.assignedNodes({ flatten:true })[0];
        cancelButton.onclick = () => { this.#resetDateValue() };

        const confirmSlot = this.shadow.querySelector("slot[name='confirm-btn']");
        const confirmButton = confirmSlot.assignedNodes({ flatten:true })[0];
        confirmButton.onclick = () => { this.#confirmDateValue() };

        // Initialize Date Object:
        const time = this.getAttribute("time");
        this.selectedDate = time ? new Date(time) : new Date();
        this.confirmedDateObj = time ? new Date(time) : new Date(); // use setter function to trigger rendering of headline
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Getters and Setters:
    //////////////////////////////////////////////////////////////////////////////////////////

    /**
     * Returns the currently selected date
     * @returns Date() object
     */
    get selectedDateObj() {
        return this.selectedDate;
    }

    /**
     * Updates the currenlty selected date. The 'select' event is emitted depending on the given
     * setting of 'fireSelectEvent'.
     * @param {Date} date selected object to update
     */
    set selectedDateObj(date) {
        console.assert(date instanceof Date, "Given date has to be of type 'Date'");
        this.#selectDateValue(date.getHours(), date.getMinutes(), date.getSeconds(), date.getMilliseconds());
    }

    /**
     * Returns the currently confirmed date
     * @returns Date() object
     */
    get confirmedDateObj() {
        return this.confirmedDate;
    }

    /**
     * Updates the currenlty confirmed date. Updating the confirmed date automatically updates the
     * selected date. The events 'select' and 'confirm' are not emitted.
     * @param {Date} date confirmedDate date object
     */
    set confirmedDateObj(date) {
        console.assert(date instanceof Date, "Given date has to be of type 'Date'");
        this.selectedDate = new Date(date); // set selected date directly
        this.#selectDateValue(date.getHours(), date.getMinutes(), date.getSeconds(), date.getMilliseconds(), false); // call this function to update UI
        this.#confirmDateValue(false); // dont emit 'confirm' event
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Private Methods:
    //////////////////////////////////////////////////////////////////////////////////////////

    /**
     * Takes the given date parameters and sets the selected date object
     * @param {Number} hours hours
     * @param {Number} minutes minutes
     * @param {Number} seconds seconds
     * @param {Number} millis milliseconds
     * @param {Boolean} fireSelectEvent if true the 'select' event is emitted, otherwise not
     */
    #selectDateValue(hours, minutes, seconds, millis=0, fireSelectEvent=true) {
        if(this.selectedDate) {
            if(hours !== null) { this.selectedDate.setHours(hours); }
            if(minutes !== null) { this.selectedDate.setMinutes(minutes); }
            if(seconds !== null) { this.selectedDate.setSeconds(seconds); }
            if(millis !== null) { this.selectedDate.setMilliseconds(millis); }
        }

        // Update UI:
        this.#renderDisplay();

        // Emit Event:
        if(fireSelectEvent) {
            this.dispatchEvent(new CustomEvent("select"));
        }
    }

    /**
     * This function takes the currently selected date and confirms it. A confirmed date cannot be
     * reset to any previous date. See 'resetDateValue()' for more details. The 'confirm' event is
     * emitted depending on the given parameter.
     * @param {Boolean} fireConfirmEvent if true the 'confirm' event is emitted, otherwise not
     * @info Call this function if the confirm button is clicked
     */
    #confirmDateValue(fireConfirmEvent = true) {
        this.confirmedDate = new Date(this.selectedDate);
        if(fireConfirmEvent) {
            this.dispatchEvent(new CustomEvent("confirm"));
        }
    }

    /**
     * This function resets the currently selected date to the last confirmed date. A selected but
     * not yet confirmed date can be reset to the las confirmed date. The 'select' event is
     * emitted because the selection is updated.
     * @param {Boolean} fireResetEvent if true the 'reset' event is emitted, otherwise not
     * @info Call this function if the 'Cancel' button is clicked
     */
    #resetDateValue(fireResetEvent = true) {
        this.selectedDate = new Date(this.confirmedDate); // reset to prev confirmed date
        this.#selectDateValue(this.confirmedDate.getHours(), this.confirmedDate.getMinutes(), this.confirmedDate.getSeconds(), this.confirmedDate.getMilliseconds());
        if(fireResetEvent) {
            this.dispatchEvent(new CustomEvent("reset"));
        }
    }

    #updateHours(step) {
        const newDate = new Date(this.selectedDate.getTime() + (step*60*60*1000));
        this.#selectDateValue(newDate.getHours(), null, null, null);
    }

    #updateMinutes(step) {
        const newDate = new Date(this.selectedDate.getTime() + (step*60*1000));
        this.#selectDateValue(null, newDate.getMinutes(), null, null);
    }

    #updateSeconds(step) {
        const newDate = new Date(this.selectedDate.getTime() + (step*1000));
        this.#selectDateValue(null, null, newDate.getSeconds(), null);
    }

    #updateMillis(step) {
        const newDate = new Date(this.selectedDate.getTime() + (step));
        this.#selectDateValue(null, null, null, newDate.getMilliseconds());
    }

    #renderDisplay() {
        if(this.headlineText) {
            this.headlineText.textContent = this.#toString(this.selectedDate);
        }
        if(this.hoursInput) {
            this.hoursInput.value = this.selectedDate.getHours();
        }
        if(this.minutesInput) {
            this.minutesInput.value = this.selectedDate.getMinutes();
        }
        if(this.secondsInput) {
            this.secondsInput.value = this.selectedDate.getSeconds();
        }
        if(this.millisInput) {
            this.millisInput.value = this.selectedDate.getMilliseconds();
        }
    }

    #showErrorVisuals(elem) {
        if(elem) {
            elem.style.borderStyle = "solid";
            elem.style.borderColor = "red";
        }
    }

    #hideErrorVisuals(elem) {
        if(elem) {
            elem.style.borderStyle = "";
            elem.style.borderColor = "";
        }
    }

    #parseHours() {
        const value = this.hoursInput.value;
        if(!this.#isNumber(value)) {
            this.#showErrorVisuals(this.hoursInput);
            return;
        }
        if(!((0 <= value) && (value <= 23))) {
            this.#showErrorVisuals(this.hoursInput);
            return;
        }
        this.#hideErrorVisuals(this.hoursInput);
        this.#selectDateValue(value, null, null, null);
    }

    #parseMinutes() {
        const value = this.minutesInput.value;
        if(!this.#isNumber(value)) {
            this.#showErrorVisuals(this.minutesInput);
            return;
        }
        if(!((0 <= value) && (value <= 59))) {
            this.#showErrorVisuals(this.minutesInput);
            return;
        }
        this.#hideErrorVisuals(this.minutesInput);
        this.#selectDateValue(null, value, null, null);
    }

    #parseSeconds() {
        const value = this.secondsInput.value;
        if(!this.#isNumber(value)) {
            this.#showErrorVisuals(this.secondsInput);
            return;
        }
        if(!((0 <= value) && (value <= 59))) {
            this.#showErrorVisuals(this.secondsInput);
            return;
        }
        this.#hideErrorVisuals(this.secondsInput);
        this.#selectDateValue(null, null, value, null);
    }

    #parseMillis() {
        const value = this.millisInput.value;
        if(!this.#isNumber(value)) {
            this.#showErrorVisuals(this.millisInput);
            return;
        }
        if(!((0 <= value) && (value <= 999))) {
            this.#showErrorVisuals(this.millisInput);
            return;
        }
        this.#hideErrorVisuals(this.millisInput);
        this.#selectDateValue(null, null, null, value);
    }

    /**
     * Tells if the given string is a time or datetime in ISO format. Valid is either the short
     * format (only time, hh:mm:ss), long-format (with milliseconds hh:mm:ss.zzz) or standard
     * format (date and time with or without milliseconds, YYYY-MM-DDThh:mm:ss).
     * @param {String} str String to check
     * @returns 'true' if the string is a valid format
     */
    #isValidDatetimeString(str) {
        // regex format: YYYY[-DD[-MM[Thh[:mm[:ss[.ssssss]]]]]   [Z | [+-hh:mm]]
        const ISO_FORMAT = /^\d{4}(-\d{2}(-\d{2})?)?(T\d{2}:\d{2}(:\d{2}(\.\d{1,6})?)?)?(([Z])|([+-]\d{2}:\d{2}))?$/;
        return ISO_FORMAT.test(str);
    }

    /**
     * Checks if the given string is an integer number
     * @param {String} str string to check
     * @returns true if the string represents an integer, false, otherwise
     */
    #isNumber(str) {
        if(typeof str != "string") { return false; }
        if(str === "") { return false; }
        return !isNaN(str);
    }

    /**
     * Extracts a readable string from the the given date. Use the different style sizes
     * 'short','medium','long' or 'full' for the time style.
     * @param {Date} date date object to stringify
     * @returns string in format "de-AT"
     */
    #toString(date) {
        console.assert(date instanceof Date, "Given parameter has to be of type 'Date'");
        return date.toLocaleString("de-AT", { timeStyle:"short" });
    }
}

// Define the new component
customElements.define("time-picker", TimePicker);