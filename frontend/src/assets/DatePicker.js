/**
 * This component implements a calendar like date picker. There is always a confirmed date and a
 * selected date. The selected date is the last date, the user clicked on. The selected date
 * becomes confirmed when the user clicks on the 'confirm' button. If the user clicks on the
 * 'cancel' button the selected date gets reset to the last confirmed date. This allows the user
 * to discard any changes they made when selecting a new date.
 *  
 * [Attributes]
 * Attributes are used to set or change the default behavior of the component
 * "date":              The initial date that is displayed when the component is loaded. Has to be
 *                      a string in ISO format YYYY-MM-DD. If the initial date is not set, the
 *                      current date is used.
 * "confirm-on-select": If this attribute is true, the selected date gets immediately confirmed
 *                      when the user selects a date and both events 'select' and 'confirm' get
 *                      emitted. If the attribute is false, the date only gets confirmed, when
 *                      the user clicks the 'confirm' element. This allows the user to reset any
 *                      selection previously made, by clicking the 'cancel' element.
 *
 * 
 * [Events]
 * The component emits events every time the user selects, confirms or discards(=reset) a date. Use
 * the 'select' and 'reset' event to update other parts of the UI, to reflect visual changes. Use
 * the 'confirm' event to update functional parts of the application. If the attribute
 * 'confirm-on-select' is true, both events 'select' and 'confirm' are emitted.
 * 
 * Here is a list of all events:
 * "select":    Emitted when the user selects a date or the user discards any selection.
 * "confirmed": Emitted when a date is confirmed. This usually means the user has decided on a date
 *              and clicked on the 'confirm' element. Otherwise the user selected a date and the
 *              'confirm-on-select' attribute is true.
 * "reset":     Emitted when the user discards any changes. This means the user clicked the
 *              'cancel' element.
 * 
 * 
 * [Slots]
 * The component offers control elements for example to switch months, confirm a selection, etc.
 * when they are clicked. These control elements can be slotted, which means custom component
 * libraries can be used to implement click-able elements. The available slots and what happens
 * when they are clicked by the user, can be seen below. If no element is slotted, a default
 * <button> element is created instead.
 * 
 * All available slots are:
 * "supporting-text":   Text to be displayed at the top, to help the user
 * "headline":          Element to display the currently selected date
 * "prev-month-btn":    The previous month is displayed when this element is clicked
 * "next-month-btn":    The next month is displayed when this element is clicked
 * "cancel-btn":        The selected date is reset to the last confirmed date, when this element is
 *                      clicked. The 'reset' event is emitted.
 * "confirm-btn"        The selected date is confirmed when this element is clicked. The 'confirm'
 *                      event is emitted.
 * 
 * 
 * [Style]
 * The style of the calendar grid itself can be customized, by using the available ::part()
 * selectors in the style section.
 * 
 * All part-selector:
 * "label":     Selects all weekday label elements (mon,tue,...)
 * "day":       Selects all day elements of the main calendar grid. The calendar grid shows days of
 *              the previous month, the current month and next month.
 * "active":    Selects all days of the current month. Every 'active' part is also a 'day' part.
 * "inactive":  Selects all day elements that are not active, so from the previous and next month.
 *              Every 'inactive' part is also a 'day' part.
 * "selected":  Selects the day element that is currently selected by the user
 * 
 *                      ╔═══════════════════════════════════════════════╗
 *                      ║                                               ║
 *                      ║  Select a date <──slot=supporting-text        ║
 *                      ║  18. Oct 2025 <───slot=headline               ║
 *                      ║                                               ║
 *                      ║                                     ┌─┐  ┌─┐  ║
 *                      ║  Oct 2025    slot=prev-month-btn───>│<│  │>│<─║──slot=next-month-btn
 *                      ║                                     └─┘  └─┘  ║
 *                      ║  ┌─────────────────────────────────────────┐  ║
 *                      ║  │ Mon   Tue   Wed   Thu   Fri   Sat   Sun │<─║──part:label
 *                      ║  ├────────────┬────────────────────────────┤  ║
 *       part:inactive──║─>│  29    30  │  1     2     3     4     5 │<─║──part:day
 *                      ║  ├────────────┘                            │  ║
 *         part:active──║─>│   6     7     8     9    10    11    12 │  ║
 *                      ║  │                              ┌────┐<────│──║──part:selected
 *                      ║  │  13    14    15    16    17  │ 18 │  19 │  ║
 *                      ║  │                              └────┘     │  ║
 *                      ║  │  20    21    21    23    24    25    26 │  ║
 *                      ║  │                             ┌───────────┤  ║
 *                      ║  |  27    28    29    30    31 │   1     2 │  ║
 *                      ║  ├─────────────────────────────┘           │  ║
 *       part:inactive──║─>|   3     4     5     6     7     8     9 │  ║
 *                      ║  └─────────────────────────────────────────┘  ║
 *                      ║                       ┌────────┐ ┌─────────┐  ║
 * slot=cancel-btn──────║──────────────────────>│ Cancel │ │ Confirm │<─║──slot=confirm-button
 *                      ║                       └────────┘ └─────────┘  ║
 *                      ╚═══════════════════════════════════════════════╝
 * 
 * [Usage]
 * This shows example usage of this component:
 *  <html>
 *  <head>
 *      <script defer src="DatePicker.js"></script>
 *      <style>
 *          date-picker {
 *              --color-selected: darkblue;
 *              --color-on-selected: white;
 *              --shape-corner: 1000rem;
 *          }
 *          date-picker::part(day) {
 *              border-radius: var(--shape-corner);
 *          }
 *          date-picker::part(inactive) {
 *              color: green;
 *          }
 *          date-picker::part(selected) {
 *              background-color: rgb(var(--color-selected));
 *              border-color: rgb(var(--color-selected));
 *              color: rgb(var(--color-on-selected));
 *          }
 *          date-picker::part(active):hover {
 *              border-color: rgb(var(--color-selected));
 *              border-style: solid;
 *              border-width: 1px;
 *          }
 *          date-picker::part(selected):hover {
 *              background-color: inherit;
 *              color: inherit;
 *          }
 *      </style>
 *  </head>
 *  <body>
 *      <date-picker date="2025-10-18" confirm-on-select>
 *          <my-button slot="prev-month-btn">Prev Month</my-button>
 *          <my-button slot="next-month-btn">Next Month</my-button>
 *          <my-button slot="cancel-btn">Cancel</my-button>
 *          <my-button slot="confirm-btn">OK</my-button>
 *      </date-picker>
 *  </body>
 *  <script>
 *      window.onload = () => {
 *          const datePicker = document.querySelector("date-picker");
 *          if(datePicker) {
 *              datePicker.addEventListener("select", () => { console.log("onselect"+datePicker.selectedDateObj); });
 *              datePicker.addEventListener("confirm", () => { console.log("onconfirm"+datePicker.confirmedDateObj); });
 *              datePicker.addEventListener("reset", () => { console.log("onreset"); });
 *          }
 *      }
 *  </script>
 *  <html>
 * 
 * 
 * 
 * This implementation is based on <wc-datepicker> from https://github.com/vanillawc/wc-datepicker
 */
class DatePicker extends HTMLElement {
    //////////////////////////////////////////////////////////////////////////////////////////
    // Static Template:
    //////////////////////////////////////////////////////////////////////////////////////////
    static observedAttributes = ["date", "confirm-on-select"];
    static templateString = `
    <style>
        :host {
            --grid-margin: 10px;
            height: 100%;
            width: 100%;

            display: flex;
            align-items: center;
            flex-direction: column;
            justify-content: center;
            overflow: hidden;
        }
        header {
            flex-shrink: 0;
            margin-bottom: var(--grid-margin);
            width: 100%;
        }
        footer {
            flex-shrink: 0;
            margin-top: var(--grid-margin);
            width: 100%;

            display: flex;
            gap: var(--grid-margin);
            justify-content: flex-end;
        }
        main {
            // flex-grow: 1;
            width: 100%;

            display: flex;
            justify-content: center;
            align-items: center;
            overflow: hidden;
        }
        .flex-row {
            align-items: center;
            display: flex;
            gap: var(--grid-margin);
            justify-content: space-between;
        }
        .grid {
            height: 100%;
            max-height: 100%;
            width: 100%;
            aspect-ratio: 1 / 1;

            display: grid;
            grid-gap: var(--grid-margin);
            grid-template-columns: repeat(7, 1fr);
        }
        .grid-item {
            aspect-ratio: 1 / 1;
            box-sizing: border-box;
            cursor: default;
            display: grid;
            height: auto;
            place-items: center;
        }
        [part~="selected"] {
            border-style: solid;
            border-width: 1px;   
        }
        [part~="inactive"] {
            opacity: 0.4;
        }
    </style>
    <header>
        <div>
            <div style="opacity: 0.4">
                <slot name="supporting-text"></slot>
            </div>
            <div>
                <slot name=headline><span></span></slot>
            </div>
        </div>
        <div class="flex-row">
            <div>
                <span id="month">Month</span> <span id="year">Year</span>
            </div>
            <div>
                <slot name="prev-month-btn"><button>&#11207;</button></slot>
                <slot name="next-month-btn"><button>&#11208;</button></slot>
            </div>
        </div>
    </header>
    <main>
        <div class="grid">
            <!-- filled by'createGridItems()' on initialization -->
        </div>
    </main>
    <footer>
        <slot name="cancel-btn"><button>Cancel</button></slot>
        <slot name="confirm-btn"><button>OK</button></slot>
    </footer>
    `;

    //////////////////////////////////////////////////////////////////////////////////////////
    // Web Component Lifecycle Hooks:
    //////////////////////////////////////////////////////////////////////////////////////////
    constructor() {
        super()
        // Set Default Properties:
        this.dayNames = ["M","T","W","T","F","S","S"];
        this.monthNames = ["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"];
        this.confirmOnSelect = false;

        // Setup Shadow DOM:
        const template = document.createElement("template");
        if(!DatePicker.templateString) { throw new Error("No template found"); }
        template.innerHTML = DatePicker.templateString;
        this.shadow = this.attachShadow({ mode:"closed" });
        this.shadow.append(template.content.cloneNode(true));
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if(name === "date") {
            if(!this.#isValidDatetimeString(newValue)) { //  check valid datetime string
                throw new Error(`Attribute '${name}' has an invalid string format: "${newValue}", use ISO format YYYY-MM-DDThh:mm:ss.zzz.`);
            }
            this.confirmedDateObj = new Date(newValue);
        } else if(name === "confirm-on-select") {
            this.confirmOnSelect = true;
        }
    }

    connectedCallback() {
        // Build Template Structure:
        this.#createGridItems();

        // Initialize Slotted Elements:
        const headlineSlot = this.shadow.querySelector("slot[name='headline']");
        this.headlineText = headlineSlot.assignedNodes({ flatten:true })[0];

        const prevMonthSlot = this.shadow.querySelector("slot[name='prev-month-btn']");
        const prevMonthButton = prevMonthSlot.assignedNodes({ flatten:true })[0];
        prevMonthButton.onclick = () => { this.#showPrevMonth() };

        const nextMonthSlot = this.shadow.querySelector("slot[name='next-month-btn']");
        const nextMonthButton = nextMonthSlot.assignedNodes({ flatten:true })[0];
        nextMonthButton.onclick = () => { this.#showNextMonth() };

        const cancelSlot = this.shadow.querySelector("slot[name='cancel-btn']");
        const cancelButton = cancelSlot.assignedNodes({ flatten:true })[0];
        cancelButton.onclick = () => { this.#resetDateValue() };

        const confirmSlot = this.shadow.querySelector("slot[name='confirm-btn']");
        const confirmButton = confirmSlot.assignedNodes({ flatten:true })[0];
        confirmButton.onclick = () => { this.#confirmDateValue() };

        // Initialize Date Object (holds picked datetime):
        const date = this.getAttribute("date");
        this.confirmedDateObj = date ? new Date(date) : new Date(); 

        // Initialize Resize Observer:
        const resizeObserver = new ResizeObserver(() => { this.#resize() });
        resizeObserver.observe(this);

        // Initialize Internal State:
        this.displayedMonth = this.selectedDate.getMonth();
        this.displayedYear = this.selectedDate.getFullYear();
        this.#renderCalendar();
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
     * Updates the currently selected date. The 'select' event is not emitted.
     * @param {Date} date selected object to update
     */
    set selectedDateObj(date) {
        console.assert(date instanceof Date, "Given date has to be of type 'Date'");
        this.#selectDateValue(date.getDate(), date.getMonth(), date.getFullYear(), false); // do not emit 'select' event
    }

    /**
     * Returns the currently confirmed date
     * @returns Date() object
     */
    get confirmedDateObj() {
        return this.confirmedDate;
    }

    /**
     * Updates the currently confirmed date. Updating the confirmed date automatically updates the
     * selected date. The events 'select' and 'confirm' are not emitted.
     * @param {Date} date confirmedDate date object
     */
    set confirmedDateObj(date) {
        console.assert(date instanceof Date, "Given date has to be of type 'Date'");
        this.selectedDate = new Date(date); // set selected date directly
        this.#selectDateValue(date.getDate(), date.getMonth(), date.getFullYear(), false); // call this function to update UI
        this.#confirmDateValue(false); // do not emit 'confirm' event
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Private Methods:
    //////////////////////////////////////////////////////////////////////////////////////////

    /**
     * Takes the given date parameters and sets the selected date object
     * @param {Number} day date number
     * @param {Number} month month index
     * @param {Number} year full year number
     * @param {Boolean} fireSelectEvent if true the 'select' event is emitted, otherwise not
     */
    #selectDateValue(day, month, year, fireSelectEvent = true) {
        // Update Internal Date Object:
        if(this.selectedDate) {
            this.selectedDate.setFullYear(year);
            this.selectedDate.setMonth(month, day);
        }
        
        // Update UI:
        if(this.headlineText) {
            this.headlineText.textContent = this.#toString(this.selectedDate);
        }
        this.displayedMonth = month;
        this.displayedYear = year;
        this.#renderCalendar();

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
        this.#selectDateValue(this.confirmedDate.getDate(), this.confirmedDate.getMonth(), this.confirmedDate.getFullYear());
        if(fireResetEvent) {
            this.dispatchEvent(new CustomEvent("reset"));
        }
    }

    /**
     * This function is called when the user clicks on a day element. Updates the selected date
     * object.
     * @param {JSON} date JSON object with {"day":day,"month":month,"year":year}
     */
    #dayClickedEventHandler(date) {
        this.#selectDateValue(date.day, date.month, date.year);
        if(this.confirmOnSelect) {
            this.#confirmDateValue();
        }
    }

    /**
     * This function increments the currently shown month
     * @info Call this function if the 'nextMonth' button is clicked
     */
    #showNextMonth() {
        if(this.displayedMonth === 11) {
            this.displayedMonth = 0;
            this.displayedYear++;
        } else {
            this.displayedMonth++;
        }
        this.#renderCalendar();
    }

    /**
     * This function decrements the currently shown month
     * @info Call this function if the 'prevMonth' button is clicked
     */
    #showPrevMonth() {
        if(this.displayedMonth === 0) {
            this.displayedMonth = 11;
            this.displayedYear--;
        } else {
            this.displayedMonth--;
        }
        this.#renderCalendar();
    }

    /**
     * This function increments the currently shown year
     * @info Call this function if the 'nextYear' button is clicked
     */
    #showNextYear() {
        this.displayedYear++;
        this.#renderCalendar();
    }

    /**
     * This function decrements the currently shown year
     * @info Call this function if the 'prevYear' button is clicked
     */
    #showPrevYear() {
        this.displayedYear--;
        this.#renderCalendar();
    }

    /**
     * Creates the elements of the calendar grid. Only called once during initialization.
     */
    #createGridItems() {
        const grid = this.shadow.querySelector(".grid");
        if(!grid) { throw new Error("Could not query grid element. Template needs an element <div class='grid'>."); }

        // Create Weekday Labels:
        const weekdays = this.dayNames;
        for(const weekday of weekdays) {
            const labelElement = document.createElement("div");
            labelElement.className = "grid-item"; // class 'grid-item' used for grid styling
            labelElement.setAttribute("part", "label");
            labelElement.innerHTML = weekday;
            grid.appendChild(labelElement);
        }

        // Create Days:
        for(let idx = 0; idx < 42; idx++) {
            const dayElement = document.createElement("div");
            dayElement.className = "grid-item"; // class 'grid-item' used for grid styling
            dayElement.setAttribute("part", "day");
            grid.appendChild(dayElement);
        }
    }

    /**
     * Called when the component is resized. Sets the width of the elements dynamically so it
     * always uses maximum width and height, which ever is smaller. Sets the width of header,
     * grid and footer element.
     */
    #resize() {
        // Calculate Unused Height:
        let usedHeight = 0;
        for(const child of this.shadow.children) {
            const computedStyle = window.getComputedStyle(child);
            usedHeight += child.offsetHeight;
            usedHeight += parseFloat(computedStyle.marginTop);
            usedHeight += parseFloat(computedStyle.marginBottom);
        }
        const unusedHeight = this.offsetHeight - usedHeight;

        // Calculate Unused Width:
        const main = this.shadow.querySelector("main");
        const grid = this.shadow.querySelector(".grid");
        const unusedWidth = main.offsetWidth - grid.offsetWidth;

        // Calculate Limit with Unused Pixels:
        const unusedPixels = Math.min(unusedHeight,unusedWidth);
        const limit = grid.offsetHeight + unusedPixels;

        // Set New Limit:
        const header = this.shadow.querySelector("header");
        const footer = this.shadow.querySelector("footer");
        grid.style.width = limit+"px";
        grid.style.height = limit+"px";
        header.style.width = limit+"px";
        footer.style.width = limit+"px";
    }

    /**
     * Renders the calendar grid with the currently displayed month and year
     */
    #renderCalendar() {
        // Generate Day Array:
        let dates = []; // numbers of dates
        let activeDays = []; // boolean values, if the daynumber with the same index is active or not
        this.#generateDayArray(dates, activeDays);

        const grid = this.shadow.querySelector(".grid");
        if(!grid) { throw new Error("Could not query grid element. Template needs an element <div class='grid'>."); }

        // Style Day Elements:
        const gridElements = grid.querySelectorAll("[part~='day']"); // the '~=' attribute selector matches the specified word delimited by spaces
        for(const [idx,gridElement] of gridElements.entries()) {
            gridElement.innerHTML = dates[idx].day;
            gridElement.removeAttribute("part"); // clean any parts
            gridElement.onclick = null; // remove any event listeners
            gridElement.onclick = () => { this.#dayClickedEventHandler(dates[idx]) };
            let attributeString = "day";
            if(dates[idx].day === this.selectedDate.getDate() && dates[idx].month === this.selectedDate.getMonth() && dates[idx].year === this.selectedDate.getFullYear()) {
                attributeString += " selected";
            }
            if(activeDays[idx]) {
                attributeString += " active";
            } else {
                attributeString += " inactive";
            }
            gridElement.setAttribute("part", attributeString);
        }

        // Update Text:
        const month = this.shadow.querySelector("#month");
        if(month) { month.innerHTML = this.monthNames[this.displayedMonth]; }
        const year = this.shadow.querySelector("#year");
        if(year) { year.innerHTML = this.displayedYear; }
    }

    /**
     * Generates the array of days for the currently displayed month and year. The day array is
     * essentially a list (=Array) starting with the last monday of the previous month. Followed
     * by the days of the current month. Ending with the first few days of the next month.
     * @param {Array} dateArray Array of numbers that holds the date numbers of the days to render
     * @param {Array} activeDays Array of booleans with same length as 'dayArray'. Active days with
     * the same index are set 'true', inactive days are set 'false'.
     */
    #generateDayArray(dateArray, activeDays) {
        // Date Obj For Rendering Calendar Grid:
        const displayedDate = new Date(this.displayedYear, this.displayedMonth);
        displayedDate.setDate(1); // use first day of displayed month

        // Current Date:
        const weekday = displayedDate.getDay();
        const month = displayedDate.getMonth();
        const year = displayedDate.getFullYear();
        let daysInMonth = this.#daysInMonth(month, year);

        // Previous Month:
        const startOfPrevMonth = new Date(displayedDate);
        startOfPrevMonth.setMonth(displayedDate.getMonth() - 1); // decrement date by one month
        const prevMonth = startOfPrevMonth.getMonth();
        const prevYear = startOfPrevMonth.getFullYear();
        const daysInPrevMonth = this.#daysInMonth(prevMonth, prevYear);

        // Next Month:
        const startOfNextMonth = new Date(displayedDate);
        startOfNextMonth.setMonth(displayedDate.getMonth() + 1); // increment date by one month
        const nextMonth = startOfNextMonth.getMonth();
        const nextYear = startOfNextMonth.getFullYear();

        // Build Day Array:
        const offset = ((weekday == 0) ? 6 : (weekday-1)); // sunday: weekday=0, monday: weekday=1
        const firstMonday = daysInPrevMonth - (offset - 1); // date of the first monday to generate (= last monday of prev month)
        for(let day = firstMonday; day <= daysInPrevMonth; day++) { // append days of prev month (=non-active days)
            const date = { "day":day, "month":prevMonth, "year":prevYear }
            dateArray.push(date);
            activeDays.push(false);
        }
        for(let day = 1; day <= daysInMonth; day++) { // append days of this month (=active-days)
            const date = { "day":day, "month":month, "year":year }
            dateArray.push(date);
            activeDays.push(true);
        }
        const numberOfNextMonthDays = 42 - dateArray.length;
        for(let day = 1; day <= numberOfNextMonthDays; day++) { // append days of next month (=non-active days)
            const date = { "day":day, "month":nextMonth, "year":nextYear }
            dateArray.push(date);
            activeDays.push(false);
        }
    }

    /**
     * Tells if the given year is a leap year
     * @param {Number} year 
     * @returns 'true' if the year is a leap year, false otherwise
     */
    #isLeapYear(year) {
        return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)
    }

    /**
     * Tells how many days there are in the given month of the year
     * @param {Number} month month to inspect (jan=0,...,dec=11)
     * @param {Number} year year of the month
     * @returns Number of days of the given month
     */
    #daysInMonth(month, year) {
        month++;
        if(month === 1 || month === 3 || month === 5 || month === 7 || month === 8 || month === 10 || month === 12) {
            return 31;
        } else if(month === 4 || month === 6 || month === 9 || month === 11) {
            return 30;
        } else if(month === 2 && this.#isLeapYear(year)) {
            return 29;
        } else if(month === 2 && !(this.#isLeapYear(year))) {
            return 28;
        }
    }

    /**
     * Tells if the given string is a date or datetime in ISO format. Valid is either the short
     * format (only date, YYYY-MM-DD), standard format (date and time, YYYY-MM-DDThh:mm:ss) or the
     * long format (with milliseconds, YYYY-MM-DDThh:mm:ss.zzz).
     * @param {String} str String to check
     * @returns 'true' if the string is a valid format
     */
    #isValidDatetimeString(str) {
        // regex format: YYYY[-DD[-MM[Thh[:mm[:ss[.ssssss]]]]]   [Z | [+-hh:mm]]
        const ISO_FORMAT = /^\d{4}(-\d{2}(-\d{2})?)?(T\d{2}:\d{2}(:\d{2}(\.\d{1,6})?)?)?(([Z])|([+-]\d{2}:\d{2}))?$/;
        return ISO_FORMAT.test(str);
    }

    /**
     * Extracts a readable string from the the given date
     * @param {Date} date date object to stringify
     * @returns string in format "18. Oct 2025"
     */
    #toString(date) {
        console.assert(date instanceof Date, "Given parameter has to be of type 'Date'");
        return date.toLocaleString("de-AT", { dateStyle:"long" });
    }
}

// Define the new component
customElements.define("date-picker", DatePicker);