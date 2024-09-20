/**
 * This function takes the given timestamp and returns the readable formated string of date and
 * time.
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
                value = toLocalDateTime(value);
            }
            cell.innerHTML = value;
        }
    }
}

function plotData(chartCanvas, myChart, labels, datasets) {
    // Build Canvas Title:
    let firstLabel = labels[0];
    let lastLabel = labels[labels.length - 1];
    titleText = toLocalDate(firstLabel)+" - "+toLocalDate(lastLabel);

    // Convert UNIX Labels to String:
    labels = labels.map(function(label) { return toLocalTime(label); });

    //Declare Canvas Variables:
    let config = { // plot canvas config
        type: 'line',
        data: {
            labels: labels,
            datasets: datasets
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            onResize: updateChart,
            plugins: {
                title: {
                    display: true,
                    text: titleText
                },
                tooltip: true,
                zoom: {
                    zoom: {
                        drag: {
                            enabled: true
                        },
                        mode: 'x',
                    }
                }
            }
        }
    };

    // Build Canvas:
    if (myChart) myChart.destroy();
    myChart = new Chart(chartCanvas, config);

    // document.getElementById("controller").innerHTML = '<button onclick="resetZoom()">Reset Zoom</button> <button onclick="clearCanvas()">Clear Canvas</button>';
    // document.getElementById("text").innerHTML = 'Data plotted.';
}

function updateChart(myChart, newSize) {
    myChart.resize();
    // myChart.reset();
    // myChart.render();
}