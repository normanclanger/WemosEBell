#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "config.h"
#include "network.h"
#include "led.h"

// --------------------------------------------------
// tracking the status of network registration
// --------------------------------------------------

enum NetworkState
{
    NETWORK_START,
    NETWORK_WIFI_CONNECTING,
    NETWORK_REGISTERING,
    NETWORK_READY
};

NetworkState networkState = NETWORK_START;  // got to start by initiating

unsigned long wifiStartTime = 0;
unsigned long lastWifiMessage = 0;

const unsigned long WIFI_TIMEOUT = 30000;

// --------------------------------------------------
// Network state
// --------------------------------------------------

WiFiUDP udp;

bool registered = false;
uint8_t bellNumber = 0;

unsigned long lastRegistrationAttempt = 0;

const unsigned long REGISTRATION_RETRY_INTERVAL = 3000;

const uint16_t LOCAL_UDP_PORT = 5006;

// --------------------------------------------------
// Protocol
// --------------------------------------------------

const uint8_t PROTOCOL_VERSION = 1;

const uint8_t MESSAGE_REGISTER = 1;
const uint8_t MESSAGE_REGISTERED = 2;
const uint8_t MESSAGE_STRIKE = 3;
const uint8_t MESSAGE_REJECTED = 4;

const uint8_t REGISTER_PACKET_SIZE = 8;
const uint8_t RESPONSE_PACKET_SIZE = 3;

// --------------------------------------------------
// Helpers
// --------------------------------------------------

void sendRegistration()
{
    uint8_t packet[REGISTER_PACKET_SIZE];

    // Protocol version
    packet[0] = PROTOCOL_VERSION;

    // Message type
    packet[1] = MESSAGE_REGISTER;

    // MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);

    memcpy(&packet[2], mac, 6);

    IPAddress serverIP;
    if (!serverIP.fromString(config.serverAddress))
    {
        Serial.println("ERROR: Invalid server address");
        return;
    }

    udp.beginPacket(serverIP, config.serverPort);
    udp.write(packet, sizeof(packet));
    udp.endPacket();

    Serial.println("REGISTER sent");
}

// --------------------------------------------------
// Main network loop - depending on networkState, start network in a non-blocking manner
// --------------------------------------------------

void networkLoop()
{
    // ------------------------------------------------
    // Start Wi-Fi connection
    // ------------------------------------------------

    if (networkState == NETWORK_START)
    {
        registered = false;
        bellNumber = 0;

        LEDFlashOn();  // start the LED flashing

        Serial.println();
        Serial.println("Network initialisation");
        Serial.println("======================");

        Serial.print("Connecting to Wi-Fi: ");
        Serial.println(config.wifiSsid);

        WiFi.mode(WIFI_STA);

        WiFi.begin(
            config.wifiSsid,
            config.wifiPassword
        );

        wifiStartTime = millis();
        lastWifiMessage = millis();

        networkState = NETWORK_WIFI_CONNECTING;

        return;
    }

    // ------------------------------------------------
    // Wait for Wi-Fi
    // ------------------------------------------------

    if (networkState == NETWORK_WIFI_CONNECTING)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println();
            Serial.println("Wi-Fi connected.");

            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            Serial.print("Server: ");
            Serial.print(config.serverAddress);
            Serial.print(":");
            Serial.println(config.serverPort);

            if (udp.begin(LOCAL_UDP_PORT))
            {
                Serial.print("UDP listening on port ");
                Serial.println(LOCAL_UDP_PORT);

                networkState = NETWORK_REGISTERING;

                sendRegistration();
                lastRegistrationAttempt = millis();
            }
            else
            {
                Serial.println("ERROR: UDP begin failed");

                // Start again
                networkState = NETWORK_START;
            }

            return;
        }

        // Give some visible indication that we are still waiting
        if (millis() - lastWifiMessage >= 1000)
        {
            Serial.print(".");
            lastWifiMessage = millis();
        }

        // Wi-Fi timeout
        if (millis() - wifiStartTime >= WIFI_TIMEOUT)
        {
            Serial.println();
            Serial.println("ERROR: Wi-Fi connection timeout");

            WiFi.disconnect();

            // Try again
            networkState = NETWORK_START;
        }

        return;
    }

    // ------------------------------------------------
    // Registration / ready
    // ------------------------------------------------

    if (networkState == NETWORK_REGISTERING ||
        networkState == NETWORK_READY)
    {



        // ------------------------------------------------
        // Check for server response
        // ------------------------------------------------

        int packetSize = udp.parsePacket();

        if (packetSize > 0)
        {
            uint8_t packet[32];

            int length = udp.read(
                packet,
                sizeof(packet)
            );

            if (length < 2)
            {
                Serial.println("Invalid server response");
                return;
            }

            if (packet[0] != PROTOCOL_VERSION)
            {
                Serial.println("Unsupported protocol version");
                return;
            }

            if (packet[1] == MESSAGE_REGISTERED)
            {
                if (length != RESPONSE_PACKET_SIZE)
                {
                    Serial.println("Invalid REGISTERED response");
                    return;
                }

                bellNumber = packet[2];
                registered = true;

                Serial.print("Registered as Bell ");
                Serial.println(bellNumber);

                LEDRegister(bellNumber);  // ask the LED loop to send the signal

                networkState = NETWORK_READY;

                return;
            }
            else if (packet[1] == MESSAGE_REJECTED)
            {
                Serial.println("Registration rejected");
            }
            else
            {
                Serial.print("Unknown server message: ");
                Serial.println(packet[1]);
            }
        }

        // ------------------------------------------------
        // Retry registration
        // ------------------------------------------------

        if (!registered)
        {
            if (
                millis() - lastRegistrationAttempt
                >= REGISTRATION_RETRY_INTERVAL
            )
            {
                sendRegistration();
                lastRegistrationAttempt = millis();
            }
        }

        return;
    }
}

// --------------------------------------------------
// Status
// --------------------------------------------------

bool isRegistered()
{
    return registered;
}

uint8_t getBellNumber()
{
    return bellNumber;
}

void sendStrike(uint8_t stroke)
{
    if (!registered)
    {
        return;
    }

    uint8_t packet[9];

    packet[0] = PROTOCOL_VERSION;
    packet[1] = MESSAGE_STRIKE;

    uint8_t mac[6];
    WiFi.macAddress(mac);

    memcpy(&packet[2], mac, 6);

    packet[8] = stroke;

    IPAddress serverIP;

    if (!serverIP.fromString(config.serverAddress))
    {
        Serial.println("ERROR: Invalid server address");
        return;
    }

    udp.beginPacket(serverIP, config.serverPort);
    udp.write(packet, sizeof(packet));
    udp.endPacket();
}