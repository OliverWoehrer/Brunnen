/**
 * This function takes the given timestamp and returns the readable formated string.
 * @param {int} timestamp UNIX timestamp in milliseconds
 * @returns string of local time
 */
function toLocalTime(timestamp) {
    date = new Date(timestamp);
    const options = {
        year: "2-digit",
        month: "2-digit",
        day: "2-digit",
        hour: "2-digit",
        minute: "2-digit",
        second: "2-digit"
    };
    return date.toLocaleDateString("de-AT", options);
}

/**
 * Inserts the given list of data into the table element. Use the columns list to specify which
 * properties to use of the data object and in which order.
 * @param {HTMLElement} tableElem DOM element (<table>) to insert the rows
 * @param {Array} columnsList list of column names (key names of data object)
 * @param {Array} dataList list of data objects (json with keys matching the column names)
 */
function fillTable(tableElem, columnsList, dataList) {
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
                value = toLocalTime(value);
            }
            cell.innerHTML = value;
        }
    }
}