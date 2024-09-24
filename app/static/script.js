let myChart;

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

function plotData(chartCanvas, labels, datasets) {
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
}

function updateChart(myChart, newSize) {
    myChart.resize();
}

/**
 * Resets the zoom of plot canvas to default
 */
function resetZoom() {
    if(myChart) myChart.resetZoom();
}


function loadLatestValues(loadFunction) {
    $.ajax({ 
        url: "/api/web/sync", 
        type: "get",
        accepts: "application/json",
        success: function(res) {
            const lastSync = res["last_sync"];
            const delta = DELTA_DAYS * 24 * 60 * 60 * 1000; // days in milliseconds
            stopTime = new Date(lastSync);
            startTime = new Date (stopTime.getTime() - delta);
            loadFunction(startTime, stopTime);
        },
        error: alertErrorResponse
    });
}

function loadLogs(startTime, stopTime) {
    startString = startTime.toISOString().replace("Z","");
    stopString = stopTime.toISOString().replace("Z","");
    $.ajax({ 
        url: "/api/web/logs",
        type: "get",
        accepts: "application/json",
        data: { start: startString, stop: stopString },
        beforeSend: function() {
            waitingAnimation = document.getElementById("logs_table_waiting");
            waitingAnimation.style.display = "inline-block";
        },
        success: function(res) {
            // Parse Reponse:
            columns = res["columns"];
            data = res["data"];

            // Get DOM Element for Table:
            let logsTable = document.getElementById("logs_table");

            // Hide Waiting Animation:
            waitingAnimation = document.getElementById("logs_table_waiting");
            waitingAnimation.style.display = "none";
            
            // Insert User as Table Rows:
            fillTable(logsTable, columns, data);
        },
        error: alertErrorResponse
    });
}

function loadData(startTime, stopTime) {
    startString = startTime.toISOString().replace("Z","");
    stopString = stopTime.toISOString().replace("Z","");
    $.ajax({ 
        url: "/api/web/data", 
        type: "get",
        accepts: "application/json",
        data: { start: startString, stop: stopString },
        beforeSend: function() {
            waitingAnimation = document.getElementById("myChart_waiting");
            waitingAnimation.style.display = "inline-block";
            if (myChart) myChart.destroy();
        },
        success: function(res) {
            //Declare Global Variables:
            const POINTS = 2000;
            const FLOW_SCALING = 0.002577; // 388 pulses per litre
            const PRESSURE_SCALING = 0.002513; // 398 increments per bar
            const LEVEL_SCALING = 0.00157356; // 635 increments per meter

            // Parse Reponse:
            columnNames = Object.keys(res);

            // Parse Labels from Response:
            let labels;
            if("Time" in res) {
                labels = Object.values(res["Time"]);
            }

            let datasets = new Array();
            if("Flow" in res) {
                values = Object.values(res["Flow"]);
                set = {
                    label: "Flow [L/s]",
                    data: values,
                    borderColor: "rgb(68, 114, 196)"
                }
                datasets.push(set);
            }
            if("Pressure" in res) {
                values = Object.values(res["Pressure"]);
                set = {
                    label: "Pressure [Bar]",
                    data: values,
                    borderColor: "rgb(237, 125, 49)"
                }
                datasets.push(set);
            }
            if("Level" in res) {
                values = Object.values(res["Level"]);
                set = {
                    label: "Level [m]",
                    data: values,
                    borderColor: "rgb(165, 165, 165)"
                }
                datasets.push(set);
            }

            // Hide Waiting Animation:
            waitingAnimation = document.getElementById("myChart_waiting");
            waitingAnimation.style.display = "none";

            // Plot Data:
            let chartCanvas = document.getElementById("myChart");
            plotData(chartCanvas, labels, datasets);
        },
        error: alertErrorResponse
    });
}

function showElement(elementID) {
    document.getElementById(elementID).style.display = "block";
}

function hideElement(elementID) {
    document.getElementById(elementID).style.display = "none";
}

function alertErrorResponse(res) {
    console.log(res.responseText);
    const doc = new DOMParser().parseFromString(res.responseText, "text/html");
    let title = doc.getElementsByTagName("title");
    let nodes = doc.getElementsByTagName("p");
    window.alert("Server Response: ["+title[0].innerText+"] "+nodes[0].innerText);
}
