#include <arduino.h>
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include <Wire.h>
#include "config.h"
#include "network.h"
#include "led.h"




// initialise MPU

MPU6050 mpu;

bool dmpReady = false;
uint8_t devStatus;
uint16_t packetSize;
uint8_t fifoBuffer[64];

VectorInt16 aa;

// handbell specific values

bool handstroke = true;


void startStrikeDetection()
{
    delay(2000);
    Serial.println();
    Serial.println("Wemos MPU6050 DMP test");

    // Wemos D1 mini: SDA = D2, SCL = D1
    Serial.println("Before Wire.begin()");
    Wire.begin();
    Serial.println("After Wire.begin()");
    Serial.flush();
    delay(500);

    Serial.println("Before mpu.initialize()");
    Serial.flush();
    mpu.initialize();
    Serial.println("After mpu.initialize()");
    Serial.flush();  


    if (!mpu.testConnection())
    {
        Serial.println("MPU6050 connection failed!");
        return;
    }

    Serial.println("MPU6050 connected.");

    // Initialise DMP
    Serial.println("Initialising DMP...");
    devStatus = mpu.dmpInitialize();

    // Apply calibration offsets loaded from EEPROM

    Serial.println("Applying MPU6050 calibration...");

    mpu.setXAccelOffset(config.accelX);
    mpu.setYAccelOffset(config.accelY);
    mpu.setZAccelOffset(config.accelZ);

    mpu.setXGyroOffset(config.gyroX);
    mpu.setYGyroOffset(config.gyroY);
    mpu.setZGyroOffset(config.gyroZ);

    if (devStatus != 0)
    {
        Serial.print("DMP initialisation failed: ");
        Serial.println(devStatus);
        return;
    }

    // Enable DMP
    mpu.setDMPEnabled(true);

    packetSize = mpu.dmpGetFIFOPacketSize();
    dmpReady = true;

    Serial.println("DMP ready!");
    Serial.println("X\tY\tZ");
}

void strikeDetectionLoop()
{
    if (!dmpReady)
        return;

    // Read the latest available DMP packet
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer))
    {
        mpu.dmpGetAccel(&aa, fifoBuffer);

        //Serial.print("X: ");
        //Serial.print(aa.x);
        //Serial.print("Y: ");
        //Serial.print(aa.y);
        //Serial.print("Z: ");
        //Serial.println(+aa.z);

        if (handstroke)
        {
          if (aa.z > config.strikeh)
          {
            sendStrike(STROKE_HAND);
            LEDStrike();
            Serial.println("Handstroke strike");
            handstroke = false;
          }
        }
        else
        {
          if (aa.z < config.strikeb)
          {
            sendStrike(STROKE_BACK);
            LEDStrike();
            Serial.println("Backstroke strike");
            handstroke = true;
          }
        }
    }
} 
