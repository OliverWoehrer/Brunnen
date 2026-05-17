/**
 * This file provides static helper functions (=stateless logic) across the application.
 */



/**
 * Extracts a readable string from the the given date. Use the different sizes for the date style:
 * 'short'  DD.MM.YY
 * 'medium' DD.MM.YYYYY
 * 'long'   DD. <month-name> YYYY
 * 'full'   <week-day>, DD. <month-name> YYYY
 * @param {Date} date date object to stringify
 * @returns string in format "de-AT"
 */
export function toDateString(date) {
    console.assert(date instanceof Date, "Given parameter has to be of type 'Date'");
    return date.toLocaleString("de-AT", { dateStyle:"short" });
}

/**
 * Extracts a readable string from the the given date. Use the different sizes for the time style:
 * 'short'  hh:mm
 * 'medium' hh:mm:ss
 * 'long'   hh:mm:ss <time-zone-name>
 * 'full'   hh:mm:ss <full-time-zone-name>
 * @param {Date} date date object to stringify
 * @returns string in format "de-AT"
 */
export function toTimeString(date) {
    console.assert(date instanceof Date, "Given parameter has to be of type 'Date'");
    return date.toLocaleString("de-AT", { timeStyle:"short" });
}

/**
 * Extracts a readable string from the the given date. Use the different sizes for the date and
 * time style.
 * Date Style:
 * 'short'  DD.MM.YY
 * 'medium' DD.MM.YYYYY
 * 'long'   DD. <month-name> YYYY
 * Time Style:
 * 'short'  hh:mm
 * 'medium' hh:mm:ss
 * 'long'   hh:mm:ss <time-zone-name>
 * @param {*} date 
 * @returns 
 */
export function toDateTimeString(date) {
    console.assert(date instanceof Date, "Given parameter has to be of type 'Date'");
    return date.toLocaleString("de-AT", { dateStyle:"short", timeStyle:"short" });
}

/**
 * Extracts a readable string from the the given date.
 * @param {Date} date date object to stringify
 * @returns datetime string in ISO format YYYY-MM-DDTHH:mm:ss.sssZ
 */
export function toISOString(date) {
    console.assert(date instanceof Date, "Given parameter has to be of Type 'Date'");
    return date.toISOString();
}

/**
 * Checks if the given object is empty ({})
 * @param {Object} obj object to check
 * @returns true if the given object is empty, false otherwise
 */
export function isEmpty(obj) {
    for(const prop in obj) {
        if(Object.hasOwn(obj, prop)) return false;
    }
    return true;
}