/**
 * @author Oliver Woehrer
 * @date September 2024
 * 
 * This file provides general functionality for the entire page (mainly UI).
 * 
 */

/**
 * Inserts the given list of data into the table element. Use the columns list to specify which
 * properties to use of the data object and in which order.
 * @param {HTMLElement} tableElem DOM element (<table>) to insert the rows
 * @param {Array} columnsList list of column names (key names of data object)
 * @param {Array} dataList list of data objects (json with keys matching the column names)
 */
function fillTable(tableElem, columnsList, dataList) {
    // Clear Exisitng Rows:
    while(tableElem.rows.length > 0) {
        tableElem.deleteRow(0);
    }

    // Insert Table Header:
    let row = tableElem.insertRow(0);
    for(let i = 0; i < columnsList.length; i++) {
        let cell = row.insertCell(i);
        cell.outerHTML = "<th>"+columnsList[i]+"</th>";
    }

    // Insert Table Rows:
    for(const data of dataList) {
        let row = tableElem.insertRow(1);
        for(let i = 0; i < columnsList.length; i++) {
            let cell = row.insertCell(i);
            let value = data[i];
            if(Number.isInteger(value) && value > 10E6) {
                value = toLocalDateTime(value);
            }
            cell.innerHTML = value;
        }
    }
}

/**
 * This function makes the DOM element (with the given id) visible again
 * @param {String} elementID id of the DOM element
 */
function showElement(elementID) {
    document.getElementById(elementID).style.display = "block";
}

/**
 * This function hides the DOM element (with the given id).
 * @param {String} elementID id of the DOM element
 */
function hideElement(elementID) {
    document.getElementById(elementID).style.display = "none";
}




//===============================================
// Helper Functions
//===============================================

/**
 * This function takes the given timestamp and returns the readable formated string of date and
 * time.
 * @description Helper Function
 * @param {int} timestamp UNIX timestamp in milliseconds
 * @returns string of local date and time
 */
function toLocalDateTime(timestamp) {
    let date = toLocalDate(timestamp);
    let time = toLocalTime(timestamp);
    return ""+date+", "+time;
}

/**
 * This function takes the given timestamp and returns the readable formated string of date.
 * @description Helper Function
 * @param {int} timestamp UNIX timestamp in milliseconds
 * @returns string of local date
 */
function toLocalDate(timestamp) {
    date = new Date(timestamp);
    const options = {
        year: "2-digit",
        month: "2-digit",
        day: "2-digit",
    };
    return date.toLocaleDateString("de-AT", options);
}

/**
 * This function takes the given timestamp and returns the readable formated string of time.
 * @description Helper Function
 * @param {int} timestamp UNIX timestamp in milliseconds
 * @returns string of local time
 */
function toLocalTime(timestamp) {
    date = new Date(timestamp);
    const options = {
        hour: "2-digit",
        minute: "2-digit"
    };
    return date.toLocaleTimeString("de-AT", options);
}




//===============================================
// Tab Controls
//===============================================
/**
 * [Tab Controlls]
 * Open and close tabs by hidding and showing them when clicked. There is a seperation between
 * Links and Tabs for maximum flexibility. Links are always visible (e.g. a bar). Tabs are
 * displayed if the matching link is clicked.
 * 
 * Link Spaces are containers for links, use CSS class "linkspace" for those elements. Use the CSS
 * class "link" for Links. Tab Spaces are containers for tabs, use CSS class "tabspace" for those
 * elements. Use the CSS class "tab" for Tabs.
 * 
 */
/**
 * Changes the visuals of the given element to indicate, that this tab is currently active
 * @param {HTMLElement} link element to style
 */
function styleSelected(link) {
    link.style.borderBottomStyle = "solid";
}

/**
 * Changes the visuals of the given element to a default style, if this tab is currently not active
 * @param {HTMLElement} link element to style
 */
function styleDefault(link) {
    link.style.borderBottomStyle = "none";
}

/**
 * This function makes the tab (with the given id) visible and updates the styles of the links
 * accordingly, so only the clicked link is visualized active and others. 
 * @param {Event} event passed by the "onclick" event, if a tab-link is clicked
 * @param {String} tabName id of the DOM element to make visible (open given tab)
 */
function openTab(event, tabName) {
    // Find Other Links:
    currentElement = event.currentTarget
    isLink = currentElement.classList.contains("link");
    while(!isLink) { // climb up the element hierarchy until element is a link
        currentElement = currentElement.parentElement;
        isLink = currentElement.classList.contains("link");
    }
    
    // Look For Sibling Elements (Links):
    linkspace = currentElement.parentElement;
    const links = linkspace.getElementsByClassName("link");

    // Style All Links (Default):
    for(const link of links) {
        styleDefault(link);
    }

    // Style Target Link:
    targetLink = currentElement;
    styleSelected(targetLink);

    // Find Other Tabs:
    targetTab = document.getElementById(tabName);
    tabspace = targetTab;
    isTabSpace = false;
    while(!isTabSpace) { // climb up the element hierarchy until element is a tabspace
        tabspace = tabspace.parentElement;
        isTabSpace = tabspace.classList.contains("tabspace");
    }

    // Hide Other Tabs (Default):
    const tabs = tabspace.getElementsByClassName("tab");
    for(const tab of tabs) {
        tab.style.display = "none";
    }

    // Show Target Tab:
    targetTab.style.display = "block";
}

/**
 * This function initializes all Link Spaces and Tab Spaces on the page. Always the first link
 * element and a Link Space and the first tab in a Tab Space will be made visible, others are
 * hidden. If you want other behaviour, you can hide the elements with HTML inline stylings and
 * manually call the style functions above.
 * @see styleSelected for visualizing a link as selected
 * @see styleDefault for visualizing a link as not selected (default)
 * @param {Event} event passed by the "onclick" event, if a tab-link is clicked
 * @param {String} tabName id of the DOM element to make visible (open given tab)
 */
function initTabs() {
    // Initalize Link Spaces:
    const linkspaces = document.getElementsByClassName("linkspace");
    for(const linkspace of linkspaces) {
        // Get Links in Linkspace:
        const links = linkspace.getElementsByClassName("link");
        
        // Style All Tab Links (Default Style):
        for(const link of links) {
            styleDefault(link);
        }

        // Style First Tab Link (Selected Style):
        styleSelected(links[0])
    }

    // Initalize Tab Spaces:
    const tabspaces = document.getElementsByClassName("tabspace");
    for(const tabspace of tabspaces) {
        // Get Tabs in Tabspace:
        const tabs = tabspace.getElementsByClassName("tab");
        
        // Hide All Tab (Default):
        for(const tab of tabs) {
            tab.style.display = "none";
        }

        // Show First Tab (Selected):
        tabs[0].style.display = "block";
    }
}
