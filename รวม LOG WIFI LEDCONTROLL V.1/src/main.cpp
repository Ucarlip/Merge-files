#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi Access Point credentials
const char* ap_ssid = "ESP32-AP";
const char* ap_password = "12345678";

// Login credentials
const char* login_username = "admin";
const char* login_password = "admin123";

// LED Control variables
const int LED_PIN = 5;  // Built-in LED on most ESP32 boards
bool ledState = false;
String ledMode = "OFF";  // "ON", "OFF", "AUTO"
unsigned long previousMillis = 0;
int blinkInterval = 1000;  // Default interval in milliseconds

// Create WebServer object
WebServer server(80);

// Function to get WiFi networks as HTML options
String getWiFiNetworks() {
    String networks = "";
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        networks = "<option value=''>No networks found!</option>";
    } else {
        for (int i = 0; i < n; ++i) {
            networks += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + 
                       " (" + String(WiFi.RSSI(i)) + " dBm)" +
                       (WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? " - Open" : " - Protected") +
                       "</option>";
        }
    }
    return networks;
}

// HTML content for WiFi setup page
const char* wifi_setup_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 WiFi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        h1, h2 {
            color: #333;
            text-align: center;
        }
        .button {
            display: inline-block;
            padding: 10px 20px;
            background-color: #4CAF50;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            margin: 10px;
            border: none;
            cursor: pointer;
        }
        .button:hover {
            background-color: #45a049;
        }
        .form-group {
            margin: 15px 0;
        }
        label {
            display: block;
            margin-bottom: 5px;
        }
        input[type="text"], input[type="password"], select {
            width: 100%;
            padding: 8px;
            margin-bottom: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        select {
            background-color: white;
        }
        .status {
            text-align: center;
            margin: 20px 0;
            padding: 10px;
            border-radius: 5px;
        }
        .connected {
            background-color: #dff0d8;
            color: #3c763d;
        }
        .scan-button {
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 WiFi Setup</h1>
        
        <div class="status connected">
            <p>Access Point: %AP_SSID%</p>
            <p>IP Address: %AP_IP%</p>
            <p>Connected Devices: %CONNECTED_DEVICES%</p>
        </div>
        
        <h2>Connect to WiFi</h2>
        <form action="/connect" method="POST">
            <div class="form-group">
                <label for="ssid">Select WiFi Network:</label>
                <select id="ssid" name="ssid" required>
                    <option value="">Select a network...</option>
                    %WIFI_LIST%
                </select>
            </div>
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password">
            </div>
            <button type="submit" class="button">Connect</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

// HTML content for main page
const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 LED Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        h1, h2 {
            color: #333;
            text-align: center;
        }
        .button {
            display: inline-block;
            padding: 10px 20px;
            background-color: #4CAF50;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            margin: 10px;
            border: none;
            cursor: pointer;
        }
        .button:hover {
            background-color: #45a049;
        }
        .button.active {
            background-color: #2196F3;
        }
        .button.off {
            background-color: #f44336;
        }
        .form-group {
            margin: 15px 0;
        }
        label {
            display: block;
            margin-bottom: 5px;
        }
        input[type="number"] {
            width: 100%;
            padding: 8px;
            margin-bottom: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        .status {
            text-align: center;
            margin: 20px 0;
            padding: 10px;
            border-radius: 5px;
        }
        .led-status {
            font-size: 24px;
            margin: 20px 0;
            text-align: center;
        }
        .mode-buttons {
            text-align: center;
            margin: 20px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 LED Control</h1>
        
        <div class="status connected">
            <p>Access Point: %AP_SSID%</p>
            <p>IP Address: %AP_IP%</p>
            <p>Connected Devices: %CONNECTED_DEVICES%</p>
        </div>

        <div class="led-status">
            Current Mode: <span id="currentMode">%LED_MODE%</span>
        </div>

        <div class="mode-buttons">
            <button onclick="setMode('ON')" class="button" id="btnON">ON</button>
            <button onclick="setMode('OFF')" class="button off" id="btnOFF">OFF</button>
            <button onclick="setMode('AUTO')" class="button" id="btnAUTO">AUTO</button>
        </div>

        <div class="form-group">
            <label for="interval">Blink Interval (milliseconds):</label>
            <input type="number" id="interval" name="interval" min="100" max="10000" value="%BLINK_INTERVAL%">
            <button onclick="setInterval()" class="button">Set Interval</button>
        </div>

        <div style="text-align: center; margin-top: 20px;">
            <a href="/logout" class="button">Logout</a>
        </div>
    </div>

    <script>
        function setMode(mode) {
            fetch('/setMode?mode=' + mode)
                .then(response => response.text())
                .then(data => {
                    updateButtonStates(mode);
                    document.getElementById('currentMode').textContent = mode;
                });
        }

        function setInterval() {
            const interval = document.getElementById('interval').value;
            fetch('/setInterval?interval=' + interval)
                .then(response => response.text())
                .then(data => {
                    alert('Interval updated!');
                });
        }

        function updateButtonStates(activeMode) {
            document.getElementById('btnON').classList.remove('active');
            document.getElementById('btnOFF').classList.remove('active');
            document.getElementById('btnAUTO').classList.remove('active');
            
            if (activeMode === 'ON') {
                document.getElementById('btnON').classList.add('active');
            } else if (activeMode === 'OFF') {
                document.getElementById('btnOFF').classList.add('active');
            } else if (activeMode === 'AUTO') {
                document.getElementById('btnAUTO').classList.add('active');
            }
        }

        // Initial button state
        updateButtonStates('%LED_MODE%');
    </script>
</body>
</html>
)rawliteral";

// HTML content for login page
const char* login_html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Login - ESP32</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 400px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        .form-group {
            margin: 15px 0;
        }
        label {
            display: block;
            margin-bottom: 5px;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 8px;
            margin-bottom: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        .button {
            display: inline-block;
            padding: 10px 20px;
            background-color: #4CAF50;
            color: white;
            text-decoration: none;
            border-radius: 5px;
            margin: 10px 0;
            border: none;
            cursor: pointer;
            width: 100%;
        }
        .button:hover {
            background-color: #45a049;
        }
        .error {
            color: red;
            text-align: center;
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Login</h1>
        <form action="/login" method="POST">
            <div class="form-group">
                <label for="username">Username:</label>
                <input type="text" id="username" name="username" required>
            </div>
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" required>
            </div>
            <button type="submit" class="button">Login</button>
        </form>
    </div>
</body>
</html>
)";

void setup() {
    Serial.begin(115200);
    
    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Set up Access Point
    WiFi.softAP(ap_ssid, ap_password);
    
    Serial.println("Access Point Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
    // Define routes
    server.on("/", HTTP_GET, []() {
        // Show WiFi setup page first
        String htmlContent = String(wifi_setup_html);
        htmlContent.replace("%WIFI_LIST%", getWiFiNetworks());
        htmlContent.replace("%AP_SSID%", ap_ssid);
        htmlContent.replace("%AP_IP%", WiFi.softAPIP().toString());
        htmlContent.replace("%CONNECTED_DEVICES%", String(WiFi.softAPgetStationNum()));
        server.send(200, "text/html", htmlContent);
    });

    server.on("/led-control", HTTP_GET, []() {
        // Show LED control page
        String htmlContent = String(html);
        htmlContent.replace("%AP_SSID%", ap_ssid);
        htmlContent.replace("%AP_IP%", WiFi.softAPIP().toString());
        htmlContent.replace("%CONNECTED_DEVICES%", String(WiFi.softAPgetStationNum()));
        htmlContent.replace("%LED_MODE%", ledMode);
        htmlContent.replace("%BLINK_INTERVAL%", String(blinkInterval));
        server.send(200, "text/html", htmlContent);
    });
    
    server.on("/login", HTTP_GET, []() {
        server.send(200, "text/html", login_html);
    });
    
    server.on("/login", HTTP_POST, []() {
        String username = server.arg("username");
        String password = server.arg("password");
        
        if (username == login_username && password == login_password) {
            // Redirect to LED control page
            server.sendHeader("Location", "/led-control");
            server.send(303);
        } else {
            String error_html = String(login_html);
            error_html.replace("</form>", "<div class='error'>Invalid username or password</div></form>");
            server.send(200, "text/html", error_html);
        }
    });

    server.on("/connect", HTTP_POST, []() {
        String newSSID = server.arg("ssid");
        String newPassword = server.arg("password");
        
        // Disconnect from current WiFi
        WiFi.disconnect();
        
        // Connect to new WiFi
        WiFi.begin(newSSID.c_str(), newPassword.c_str());
        
        // Wait for connection
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }
        
        // Send response
        if (WiFi.status() == WL_CONNECTED) {
            server.sendHeader("Location", "/login");
        } else {
            server.sendHeader("Location", "/");
        }
        server.send(303);
    });

    // Add LED control routes
    server.on("/setMode", HTTP_GET, []() {
        String mode = server.arg("mode");
        if (mode == "ON" || mode == "OFF" || mode == "AUTO") {
            ledMode = mode;
            if (mode == "ON") {
                digitalWrite(LED_PIN, HIGH);
            } else if (mode == "OFF") {
                digitalWrite(LED_PIN, LOW);
            }
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/setInterval", HTTP_GET, []() {
        int newInterval = server.arg("interval").toInt();
        if (newInterval >= 100 && newInterval <= 10000) {
            blinkInterval = newInterval;
        }
        server.send(200, "text/plain", "OK");
    });
    
    // Start server
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();

    // Handle LED blinking in AUTO mode
    if (ledMode == "AUTO") {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= blinkInterval) {
            previousMillis = currentMillis;
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
        }
    }
}