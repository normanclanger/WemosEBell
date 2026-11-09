#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

constexpr int EEPROM_SIZE = 512;

constexpr char CONFIG_MAGIC[9] = "eBells  ";
constexpr uint16_t CONFIG_VERSION = 4;


struct BellConfig
{
    char magic[9];
    uint16_t version;

    char wifiSsid[64];
    char wifiPassword[64];

    char serverAddress[64];
    uint16_t serverPort;

    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;

    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;

    int16_t strikeh;
    int16_t strikeb;
};


extern BellConfig config;

void beginConfig();
bool loadConfig();
void saveConfig();
void printConfig();
void createTestConfig();

#endif