#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "calibration.h"

#include "config.h"
#include "webInterface.h"


ESP8266WebServer server(80);


// --------------------------------------------------
// Create the setup SSID from the ESP8266 MAC address
// --------------------------------------------------

String getSetupSSID()
{
    String mac = WiFi.macAddress();

    String deviceId = mac.substring(9);
    deviceId.replace(":", "");

    return "eBell-" + deviceId;
}


// --------------------------------------------------
// Handle the main configuration page
// --------------------------------------------------

void handleRoot()
{
    String html;

    html += "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>eBell Configuration</title>";
    html += "</head>";

    html += "<body>";

    html += "<h1>eBell Configuration</h1>";

    html += "<h2>After configuration</h2>";
    html += "<p>";
    html += "Power cycle bell to return to normal operation";    
    html += "</p>";

    // Device information

    html += "<h2>Device</h2>";

    html += "<p>";
    html += "<strong>MAC address:</strong> ";
    html += WiFi.macAddress();
    html += "</p>";

    html += "<p>";
    html += "<strong>Setup SSID:</strong> ";
    html += getSetupSSID();
    html += "</p>";


    // Configuration form

    html += "<form method='POST' action='/save'>";

    // Configuration form

    html += "<form method='POST' action='/save'>";

    html += "<h2>eBell network</h2>";

    // Extract the last six characters from the stored SSID

    String wifiSsid = String(config.wifiSsid);
    String bellId = "";

    if (wifiSsid.length() >= 6)
    {
        bellId = wifiSsid.substring(wifiSsid.length() - 6);
    }

    html += "<label>eBell server:</label><br>";
    html += "<input type='text' value='eBell-server-' disabled>";

    html += "<br>";

    html += "<label>Server ID:</label><br>";
    html += "<input type='text' "
            "name='bellId' "
            "value='";
    html += bellId;
    html += "' "
            "maxlength='6' "
            "pattern='[A-Fa-f0-9]{6}' "
            "required>";

    html += "<br><br>";

    html += "<p>";
    html += "Wi-Fi password is fixed.";
    html += "</p>";


    html += "<h2>Central server</h2>";

    html += "<p>";
    html += "<strong>Address:</strong> 10.42.0.1";
    html += "</p>";

    html += "<p>";
    html += "<strong>Port:</strong> 5005";
    html += "</p>";


    html += "<input type='submit' value='Save Configuration'>";

    html += "</form>";

    // MPU6050 information

    html += "<h2>MPU6050 calibration</h2>";

    html += "<p><strong>Accel X:</strong> ";
    html += String(config.accelX);
    html += "</p>";

    html += "<p><strong>Accel Y:</strong> ";
    html += String(config.accelY);
    html += "</p>";

    html += "<p><strong>Accel Z:</strong> ";
    html += String(config.accelZ);
    html += "</p>";

    html += "<p><strong>Gyro X:</strong> ";
    html += String(config.gyroX);
    html += "</p>";

    html += "<p><strong>Gyro Y:</strong> ";
    html += String(config.gyroY);
    html += "</p>";

    html += "<p><strong>Gyro Z:</strong> ";
    html += String(config.gyroZ);
    html += "</p>";

    html += "<h2>MPU6050 Calibration</h2>";

    html += "<p>IMPORTANT: Before starting calibration:</p>";

    html += "<ul>";
    html += "<li>Place the bell upside down in a horizontal position.</li>";
    html += "<li>(Or if MPU visible, keep the it horizontal and make sure the package is facing upwards.)  </li>";
    html += "<li>DO NOT move the bell during calibration.</li>";
    html += "</ul>";

    html += "<form method='POST' action='/calibrate'>";
    html += "<button type='submit'>Start Calibration</button>";
    html += "</form>";

    html += "<p id='calibrationStatus'></p>";


    html += "<h2>Strike thresholds</h2>";

    html += "<p><strong>Handstroke:</strong> ";
    html += String(config.strikeh);
    html += "</p>";

    html += "<p><strong>Backstroke:</strong> ";
    html += String(config.strikeb);
    html += "</p>";

    html += R"(
    <script>
    function checkCalibration()
    {
        fetch('/calibration-status')
            .then(response => response.text())
            .then(status => {

                const element =
                    document.getElementById('calibrationStatus');

                if (status === 'running')
                {
                    element.innerHTML =
                        'Calibration in progress... Please keep the bell completely still.';
                }
                else if (status === 'complete')
                {
                    element.innerHTML =
                        'Calibration successful. The new calibration values have been saved.';
                }
                else if (status === 'failed')
                {
                    element.innerHTML =
                        'Calibration failed. Please check the MPU6050 connection and try again.';
                }
                else
                {
                    element.innerHTML = '';
                }
            });
    }

    setInterval(checkCalibration, 2000);
    checkCalibration();
    </script>
    )";

    html += "</body>";

    html += "</html>";

    server.send(200, "text/html", html);
}


void handleSave()
{
    if (server.hasArg("bellId"))
    {
        String bellId = server.arg("bellId");

        bellId.trim();

        // Only accept exactly six hexadecimal characters

        if (bellId.length() != 6)
        {
            server.send(
                400,
                "text/html",
                "<h1>Invalid Bell ID</h1>"
                "<p>The Bell ID must contain exactly six hexadecimal characters.</p>"
                "<p><a href='/'>Return to configuration</a></p>"
            );

            return;
        }

        // Check that all characters are hexadecimal

        for (unsigned int i = 0; i < bellId.length(); i++)
        {
            char c = bellId[i];

            bool valid =
                (c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'F') ||
                (c >= 'a' && c <= 'f');

            if (!valid)
            {
                server.send(
                    400,
                    "text/html",
                    "<h1>Invalid Bell ID</h1>"
                    "<p>The Bell ID must contain only hexadecimal characters (0-9, A-F).</p>"
                    "<p><a href='/'>Return to configuration</a></p>"
                );

                return;
            }
        }

        // Build the complete eBell server SSID

        String wifiSsid = "eBell-server-" + bellId;

        wifiSsid.toCharArray(
            config.wifiSsid,
            sizeof(config.wifiSsid)
        );
    }


    // Fixed password for the eBell server network

    String password = "eBellServer123";

    password.toCharArray(
        config.wifiPassword,
        sizeof(config.wifiPassword)
    );


    // Fixed central server address

    String serverAddress = "10.42.0.1";

    serverAddress.toCharArray(
        config.serverAddress,
        sizeof(config.serverAddress)
    );


    // Fixed central server port

    config.serverPort = 5005;


    // Save configuration to EEPROM

    saveConfig();


    server.send(
        200,
        "text/html",
        "<h1>Configuration saved</h1>"
        "<p>Network configuration has been saved.</p>"
        "<p>Power cycle the bell to return to normal operation.</p>"
        "<p><a href='/'>Return to configuration</a></p>"
    );
}


// Calibration handlers

void handleCalibrate()
{
    if (getCalibrationState() == CALIBRATION_RUNNING)
    {
        server.send(
            409,
            "text/html",
            "<h1>Calibration already running</h1>"
            "<p>A calibration is already in progress.</p>"
            "<p><a href='/'>Return to configuration</a></p>"
        );
        return;
    }

    calibrate();

    server.send(
        200,
        "text/html",
        "<h1>Calibration started</h1>"
        "<p>Calibration is now in progress.</p>"
        "<p>Please keep the bell completely still.</p>"
        "<p>You can return to the configuration page to check progress.</p>"
        "<p><a href='/'>Return to configuration</a></p>"
    );
}


void handleCalibrationStatus()
{
    if (getCalibrationState() == CALIBRATION_RUNNING)
    {
        server.send(
            200,
            "text/plain",
            "running"
        );
    }
    else if (getCalibrationState() == CALIBRATION_COMPLETE)
    {
        server.send(
            200,
            "text/plain",
            "complete"
        );
    }
    else if (getCalibrationState() == CALIBRATION_FAILED)
    {
        server.send(
            200,
            "text/plain",
            "failed"
        );
    }
    else
    {
        server.send(
            200,
            "text/plain",
            "idle"
        );
    }
}




// --------------------------------------------------
// Start configuration mode
// --------------------------------------------------

void startWebInterface()
{
    Serial.println();
    Serial.println("Starting configuration mode...");

    digitalWrite(LED_BUILTIN,LOW);  // turn the internal LED on


    // Generate setup SSID

    String ssid = getSetupSSID();

    // WPA password must be at least 8 characters

    String password = "12341234";


    Serial.print("Setup SSID: ");
    Serial.println(ssid);


    // Start access point

    Serial.println("Configuring access point...");

    WiFi.mode(WIFI_AP);

    WiFi.softAP(
        ssid.c_str(),
        password.c_str()
    );


    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());


    // Start web server

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/calibrate", HTTP_POST, handleCalibrate);
    server.on("/calibration-status", HTTP_GET, handleCalibrationStatus);    

    server.begin();

    Serial.println("HTTP server started");
}


// --------------------------------------------------
// Web server processing
// --------------------------------------------------

void webInterfaceLoop()
{
    server.handleClient();
}