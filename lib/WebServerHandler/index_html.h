#ifndef INDEX_HTML_H
#define INDEX_HTML_H

constexpr char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Image Upload</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://cdn.jsdelivr.net/npm/beercss@3.5.1/dist/cdn/beer.min.css" rel="stylesheet">
    <script type="module" src="https://cdn.jsdelivr.net/npm/beercss@3.5.1/dist/cdn/beer.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body {
            font-family: Arial;
            text-align: center;
            margin: 40px;
            background-color: #f4f4f4;
        }

        .container {
            max-width: 600px;
            margin: auto;
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }

        img {
            max-width: 90%;
            height: auto;
            border: 1px solid #ccc;
            margin-top: 20px;
            border-radius: 5px;
        }

        .diagnostic {
            margin-top: 40px;
            padding: 20px;
            border-top: 2px solid #eee;
            text-align: left;
        }

        .error {
            color: red;
            font-weight: bold;
        }

        .success {
            color: green;
        }

        pre {
            background: #eee;
            padding: 10px;
            overflow-x: auto;
            border-radius: 5px;
        }

        button, .button {
            cursor: pointer;
        }

        #list-lights-button {
            margin-top: 8px;
            margin-bottom: 8px;
        }

        #status, #error-log, #internal-status {
            margin-top: 20px;
        }

        #status > span,#internal-status > span{
            padding: 10px;
        }

        .chart-container {
            position: relative;
            height: 200px;
            width: 100%;
            margin-top: 20px;
        }
    </style>
</head>
<body>
<div class="container">
    <h4>Upload PNG Image</h4>

    <form method="POST" action="/upload" enctype="multipart/form-data" class="grid cricle">
        <div class="field label prefix border s7">
            <i>attach_file</i>
            <input type="file" name="image" accept="image/jpeg">
            <input type="text">
            <label>Select image</label>
        </div>
        <input class="button s5" type="submit" value="Upload">
    </form>

    <h4>Stored Image</h4>
    <img src="/image.jpg">

    <div class="diagnostic">
        <h4>Diagnostic Section</h4>
        <div id="status">Checking connection...</div>
        <div id="internal-status">Checking status...</div>
        <div id="error-log" class="error"></div>

        <h5>Hue Lights Test</h5>
        <button id="list-lights-button" onclick="listLights()">List Lights</button>
        <pre id="lights-list">Click the button to fetch lights...</pre>

        <h5>Last 10 Logs</h5>
        <pre id="logs-list">Fetching logs...</pre>

        <h5>Memory Allocation Graphs</h5>
        <div class="chart-container">
            <canvas id="memoryChart"></canvas>
        </div>
    </div>

</div>

<script>
    const MAX_DATA_POINTS = 50;
    let memoryChart;

    function initChart() {
        const ctx = document.getElementById('memoryChart').getContext('2d');
        memoryChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [
                    {
                        label: 'PSRAM (MB)',
                        borderColor: 'rgb(75, 192, 192)',
                        data: [],
                        fill: false,
                        tension: 0.1
                    },
                    {
                        label: 'LittleFS (KB)',
                        borderColor: 'rgb(255, 99, 132)',
                        data: [],
                        fill: false,
                        tension: 0.1
                    },
                    {
                        label: 'Heap (KB)',
                        borderColor: 'rgb(54, 162, 235)',
                        data: [],
                        fill: false,
                        tension: 0.1
                    }
                ]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                scales: {
                    x: {
                        display: false
                    },
                    y: {
                        beginAtZero: true
                    }
                },
                animation: {
                    duration: 0
                }
            }
        });
    }

    function addDataPoint(label, psram, fs, heap) {
        if (!memoryChart) return;

        if (memoryChart.data.labels.length >= MAX_DATA_POINTS) {
            memoryChart.data.labels.shift();
            memoryChart.data.datasets[0].data.shift();
            memoryChart.data.datasets[1].data.shift();
            memoryChart.data.datasets[2].data.shift();
        }

        memoryChart.data.labels.push(label);
        memoryChart.data.datasets[0].data.push(psram / (1024 * 1024));
        memoryChart.data.datasets[1].data.push(fs / 1024);
        memoryChart.data.datasets[2].data.push(heap / 1024);

        memoryChart.update();
    }

    function listLights() {
        const listElement = document.getElementById('lights-list');
        const errorElement = document.getElementById('error-log');
        listElement.innerText = "Fetching lights...";
        errorElement.innerText = "";

        fetch('/lights')
            .then(response => {
                if (!response.ok) {
                    throw new Error('HTTP error ' + response.status);
                }
                return response.json();
            })
            .then(data => {
                if (data.errors && data.errors.length > 0) {
                    errorElement.innerText = "Hue Errors: " + JSON.stringify(data.errors, null, 2);
                    listElement.innerText = "Failed to fetch lights.";
                } else {
                    listElement.innerText = JSON.stringify(data.lights, null, 2);
                }
            })
            .catch(error => {
                errorElement.innerText = "Fetch Error: " + error.message;
                listElement.innerText = "An error occurred.";
            });
    }

    function updateStatus() {
        fetch('/logs')
            .then(response => response.json())
            .then(data => {
                const logsElement = document.getElementById('logs-list');
                if (Array.isArray(data)) {
                    logsElement.innerText = data.join('\n');
                } else {
                    logsElement.innerText = "Error fetching logs";
                }
            })
            .catch(error => {
                console.error('Error fetching logs:', error);
                document.getElementById('logs-list').innerText = "Failed to fetch logs";
            });

        fetch('/status')
            .then(response => response.json())
            .then(data => {
                const errorLogElement = document.getElementById('error-log');
                const statusElement = document.getElementById('status');
                if (data.authenticated) {
                    statusElement.innerHTML = '<span class="success">Connected to Hue Bridge</span> (User: ' + data.username + ')';
                } else {
                    statusElement.innerHTML = '<span class="error">Not authenticated with Hue Bridge</span>. Press the link button on the bridge.';
                }


                if (data.totalBytes !== undefined && data.usedBytes !== undefined) {
                    const free = data.totalBytes - data.usedBytes;
                    const percent = (data.totalBytes > 0) ? (data.usedBytes / data.totalBytes * 100).toFixed(1) : 0;
                    document.getElementById('internal-status').innerHTML =
                        `PSRAM: ${data.usedBytes} / ${data.totalBytes} bytes used (${percent}%) - <b>${free} bytes free</b>`;
                }

                if (data.fsTotal !== undefined && data.fsUsed !== undefined) {
                    const fsFree = data.fsTotal - data.fsUsed;
                    const fsPercent = (data.fsTotal > 0) ? (data.fsUsed / data.fsTotal * 100).toFixed(1) : 0;
                    const fsStatusId = 'fs-status-detail';
                    let fsStatus = document.getElementById(fsStatusId);
                    if (!fsStatus) {
                        fsStatus = document.createElement('div');
                        fsStatus.id = fsStatusId;
                        document.getElementById('internal-status').appendChild(fsStatus);
                    }
                    fsStatus.innerHTML = `LittleFS: ${data.fsUsed} / ${data.fsTotal} bytes used (${fsPercent}%) - <b>${fsFree} bytes free</b>`;
                }

                if (data.heapTotal !== undefined && data.heapFree !== undefined) {
                    const heapFree = data.heapFree;
                    const heapTotal = data.heapTotal;
                    const heapUsed = heapTotal - heapFree;
                    const heapPercent = (heapTotal > 0) ? (heapUsed / heapTotal * 100).toFixed(1) : 0;
                    const heapStatusId = 'heap-status-detail';
                    let fsStatus = document.getElementById(heapStatusId);
                    if (!fsStatus) {
                        fsStatus = document.createElement('div');
                        fsStatus.id = heapStatusId;
                        document.getElementById('internal-status').appendChild(fsStatus);
                    }
                    fsStatus.innerHTML = `Heap: ${heapUsed} / ${heapTotal} bytes used (${heapPercent}%) - <b>${heapFree} bytes free</b>`;
                }

                addDataPoint(new Date().toLocaleTimeString(), data.usedBytes || 0, data.fsUsed || 0, (data.heapTotal - data.heapFree) || 0);

                if (data.error && data.error.length > 0) {
                    errorLogElement.innerText = "Error: " + data.error +"\n";
                } else {
                    errorLogElement.innerText = "";
                }
            })
            .catch(error => {
                console.error('Error fetching status:', error);
                document.getElementById('status').innerHTML = '<span class="error">Failed to get status</span>';
            });
    }

    function tail(element, maxLines) {
        const lines = element.innerText.split('\n');
        if (lines.length > maxLines) {
            element.innerText = lines.slice(lines.length - maxLines).join('\n');
        }
    }

    setInterval(updateStatus, 5000);
    initChart();
    updateStatus();
</script>

</body>
</html>
)rawliteral";

#endif // INDEX_HTML_H