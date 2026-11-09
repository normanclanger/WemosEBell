#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

BellConfig config;

void beginConfig()   // tp be called before any EEPROM function used
{
    EEPROM.begin(EEPROM_SIZE);
}

void printConfig()
{
    Serial.println();
    Serial.println("Current configuration:");
    Serial.println();

    Serial.print("Magic:         ");
    Serial.println(config.magic);

    Serial.print("Version:       ");
    Serial.println(config.version);

    Serial.print("Wi-Fi SSID:    ");
    Serial.println(config.wifiSsid);

    Serial.print("Wi-Fi password:");
    Serial.println(config.wifiPassword);

    Serial.print("Server address:");
    Serial.println(config.serverAddress);

    Serial.print("Server port:   ");
    Serial.println(config.serverPort);

    Serial.println();

    Serial.println("MPU6050 calibration:");

    Serial.print("Accel X:       ");
    Serial.println(config.accelX);

    Serial.print("Accel Y:       ");
    Serial.println(config.accelY);

    Serial.print("Accel Z:       ");
    Serial.println(config.accelZ);

    Serial.print("Gyro X:        ");
    Serial.println(config.gyroX);

    Serial.print("Gyro Y:        ");
    Serial.println(config.gyroY);

    Serial.print("Gyro Z:        ");
    Serial.println(config.gyroZ);

    Serial.println("Strike point calibration:");

    Serial.print("Bstroke strike:");
    Serial.println(config.strikeb);

    Serial.print("Bstroke strike:");
    Serial.println(config.strikeh);

    Serial.println();
}


bool loadConfig()
{
    EEPROM.get(0, config);

    if (memcmp(config.magic, CONFIG_MAGIC, 8) != 0)
    {
        Serial.println("No valid configuration found.");
        return false;
    }

    if (config.version != CONFIG_VERSION)
    {
        Serial.println("Configuration version is not recognised.");
        return false;
    }

    Serial.println("Valid configuration found.");

    return true;
}


void saveConfig()
{
    memcpy(config.magic, CONFIG_MAGIC, sizeof(CONFIG_MAGIC));
    config.version = CONFIG_VERSION;

    EEPROM.put(0, config);

    if (EEPROM.commit())
    {
        Serial.println("Configuration saved.");
    }
    else
    {
        Serial.println("ERROR: EEPROM commit failed.");
    }
}


void createTestConfig()
{
    memset(&config, 0, sizeof(config));

    memcpy(config.magic, CONFIG_MAGIC, sizeof(CONFIG_MAGIC));
    config.version = CONFIG_VERSION;


    // Wi-Fi
    strcpy(config.wifiSsid, "TestNetwork");
    strcpy(config.wifiPassword, "TestPassword");

    // Central server
    strcpy(config.serverAddress, "192.168.1.100");
    config.serverPort = 5000;

    // MPU6050 calibration
    config.accelX = -4475;
    config.accelY = -1425;
    config.accelZ = 902;

    config.gyroX = 151;
    config.gyroY = 83;
    config.gyroZ = -41;

    config.strikeh = 100;
    config.strikeb = -600;

    saveConfig();
}
