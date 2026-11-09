#include <Wire.h>
#include <MPU6050.h>
#include "config.h"
#include "calibration.h"

// --------------------------------------------------
// Calibration settings
// --------------------------------------------------

const int BUFFER_SIZE = 1000;
const int ACCEL_DEADZONE = 8;
const int GYRO_DEADZONE = 1;

// --------------------------------------------------
// Calibration state settings - allowing function to be non-blocking
// --------------------------------------------------

CalibrationState calibrationState = CALIBRATION_IDLE;  // public status

CalibrationState getCalibrationState()
{
    return(calibrationState);
}

// internal state tracking

enum CalibrationStage
{
    STAGE_IDLE,
    STAGE_INITIALISE,
    STAGE_WAIT,
    STAGE_INITIAL_MEASURE,
    STAGE_INITIAL_OFFSETS,
    STAGE_APPLY_OFFSETS,
    STAGE_CALIBRATION_MEASURE,
    STAGE_CALIBRATION_CHECK,
    STAGE_COMPLETE
};

CalibrationStage calibrationStage = STAGE_IDLE;  // internal stage

unsigned long calibrationStartTime = 0;

int calibrationIteration = 0;
int calibrationReady = 0;

// Timing stats
unsigned long stageStartTime = 0;

unsigned long stageTimes[9] = {0};

CalibrationStage previousStage = STAGE_IDLE;

unsigned long offsetStart;
unsigned long offsetTime;

// --------------------------------------------------
// MPU6050
// --------------------------------------------------

MPU6050 calibrationmpu;
uint8_t deviceId;

// --------------------------------------------------
// Raw sensor values
// --------------------------------------------------

int16_t ax, ay, az;
int16_t gx, gy, gz;

// --------------------------------------------------
// Calculated means
// --------------------------------------------------

long mean_ax;
long mean_ay;
long mean_az;
long mean_gx;
long mean_gy;
long mean_gz;

// --------------------------------------------------
// Calibration offsets
// --------------------------------------------------

int16_t ax_offset;
int16_t ay_offset;
int16_t az_offset;

int16_t gx_offset;
int16_t gy_offset;
int16_t gz_offset;

// number of readings to take before returning
const int READINGS_PER_LOOP = 10;

// accumlation buffers need to be global to carry status between readings
long buff_ax = 0;
long buff_ay = 0;
long buff_az = 0;
long buff_gx = 0;
long buff_gy = 0;
long buff_gz = 0;

int meanSensorCount = 0;


// --------------------------------------------------
// Read and average the sensors
// --------------------------------------------------

bool meanSensors()
{
    // Start a new measurement
    if (meanSensorCount == 0)
    {
        buff_ax = 0;
        buff_ay = 0;
        buff_az = 0;
        buff_gx = 0;
        buff_gy = 0;
        buff_gz = 0;

        Serial.println("Taking sensor readings...");
    }

    // Take a small number of readings this time
    for (int i = 0; i < READINGS_PER_LOOP; i++)
    {

        unsigned long sensorStart = millis();

        calibrationmpu.getMotion6(
            &ax, &ay, &az,
            &gx, &gy, &gz
        );

        unsigned long sensorTime = millis() - sensorStart;

        //Serial.print("getMotion6 took ");
        //Serial.print(sensorTime);
        //Serial.println(" ms");


        if (meanSensorCount >= 100)
        {
            buff_ax += ax;
            buff_ay += ay;
            buff_az += az;
            buff_gx += gx;
            buff_gy += gy;
            buff_gz += gz;
        }

        meanSensorCount++;

        delay(2);

        // Have we completed all 1100 readings?
        if (meanSensorCount >= BUFFER_SIZE + 100)
        {
            mean_ax = buff_ax / BUFFER_SIZE;
            mean_ay = buff_ay / BUFFER_SIZE;
            mean_az = buff_az / BUFFER_SIZE;
            mean_gx = buff_gx / BUFFER_SIZE;
            mean_gy = buff_gy / BUFFER_SIZE;
            mean_gz = buff_gz / BUFFER_SIZE;

            Serial.println("Mean readings:");
            Serial.print("  Accel: ");
            Serial.print(mean_ax);
            Serial.print("  ");
            Serial.print(mean_ay);
            Serial.print("  ");
            Serial.println(mean_az);

            Serial.print("  Gyro:  ");
            Serial.print(mean_gx);
            Serial.print("  ");
            Serial.print(mean_gy);
            Serial.print("  ");
            Serial.println(mean_gz);

            // Reset ready for the next measurement
            meanSensorCount = 0;

            return true;
        }
    }

    // Not finished yet
    return false;
}

// --------------------------------------------------
// Calculate calibration offsets
// --------------------------------------------------

void calculateCalibration()
{
    // Initial estimate
    ax_offset = -mean_ax / 8;
    ay_offset = -mean_ay / 8;
    az_offset = (16384 - mean_az) / 8;

    gx_offset = -mean_gx / 4;
    gy_offset = -mean_gy / 4;
    gz_offset = -mean_gz / 4;

    int iteration = 0;

    while (true)
    {
        iteration++;

        Serial.print("Calibration iteration ");
        Serial.println(iteration);

        // Apply current offsets

        calibrationmpu.setXAccelOffset(ax_offset);
        calibrationmpu.setYAccelOffset(ay_offset);
        calibrationmpu.setZAccelOffset(az_offset);

        calibrationmpu.setXGyroOffset(gx_offset);
        calibrationmpu.setYGyroOffset(gy_offset);
        calibrationmpu.setZGyroOffset(gz_offset);

        // Measure again

        meanSensors();

        int ready = 0;

        // X acceleration

        if (abs(mean_ax) <= ACCEL_DEADZONE)
        {
            ready++;
        }
        else
        {
            ax_offset =
                ax_offset - mean_ax / ACCEL_DEADZONE;
        }

        // Y acceleration

        if (abs(mean_ay) <= ACCEL_DEADZONE)
        {
            ready++;
        }
        else
        {
            ay_offset =
                ay_offset - mean_ay / ACCEL_DEADZONE;
        }

        // Z acceleration
        // We expect +1g = 16384

        if (abs(16384 - mean_az) <= ACCEL_DEADZONE)
        {
            ready++;
        }
        else
        {
            az_offset =
                az_offset +
                (16384 - mean_az) / ACCEL_DEADZONE;
        }

        // X gyro

        if (abs(mean_gx) <= GYRO_DEADZONE)
        {
            ready++;
        }
        else
        {
            gx_offset =
                gx_offset -
                mean_gx / (GYRO_DEADZONE + 1);
        }

        // Y gyro

        if (abs(mean_gy) <= GYRO_DEADZONE)
        {
            ready++;
        }
        else
        {
            gy_offset =
                gy_offset -
                mean_gy / (GYRO_DEADZONE + 1);
        }

        // Z gyro

        if (abs(mean_gz) <= GYRO_DEADZONE)
        {
            ready++;
        }
        else
        {
            gz_offset =
                gz_offset -
                mean_gz / (GYRO_DEADZONE + 1);
        }

        Serial.print("Ready: ");
        Serial.print(ready);
        Serial.println(" / 6");

        if (ready == 6)
        {
            break;
        }
    }
}


// --------------------------------------------------
// Print final calibration
// --------------------------------------------------

void printCalibration()
{
    Serial.println();
    Serial.println("==============================");
    Serial.println("CALIBRATION COMPLETE");
    Serial.println("==============================");

    Serial.println();

    Serial.println("Final sensor readings:");

    Serial.print("Accel X: ");
    Serial.println(mean_ax);

    Serial.print("Accel Y: ");
    Serial.println(mean_ay);

    Serial.print("Accel Z: ");
    Serial.println(mean_az);

    Serial.print("Gyro X:  ");
    Serial.println(mean_gx);

    Serial.print("Gyro Y:  ");
    Serial.println(mean_gy);

    Serial.print("Gyro Z:  ");
    Serial.println(mean_gz);

    Serial.println();

    Serial.println("Calibration offsets:");

    Serial.print("Accel X: ");
    Serial.println(ax_offset);

    Serial.print("Accel Y: ");
    Serial.println(ay_offset);

    Serial.print("Accel Z: ");
    Serial.println(az_offset);

    Serial.print("Gyro X:  ");
    Serial.println(gx_offset);

    Serial.print("Gyro Y:  ");
    Serial.println(gy_offset);

    Serial.print("Gyro Z:  ");
    Serial.println(gz_offset);

    Serial.println();

    Serial.println("Expected final readings:");
    Serial.println("0  0  16384  0  0  0");

    Serial.println();
}

// write the calibration data to the EEPROM
void saveCalibration()
{
    config.accelX = ax_offset;
    config.accelY = ay_offset;
    config.accelZ = az_offset;

    config.gyroX = gx_offset;
    config.gyroY = gy_offset;
    config.gyroZ = gz_offset;

    saveConfig();
}



// --------------------------------------------------
// Public calibration call - initiate the process
// --------------------------------------------------

bool calibrate()
{
    if (calibrationState == CALIBRATION_RUNNING)
    {
        return false;
    }

    Serial.println();
    Serial.println("==============================");
    Serial.println("WEMOS MPU6050 CALIBRATION");
    Serial.println("==============================");

    calibrationState = CALIBRATION_RUNNING;
    calibrationStage = STAGE_INITIALISE;

    return true;
}

// Calibration loop


void calibrationLoop()
{
    if (calibrationStage != previousStage)
    {
        unsigned long now = millis();

        if (previousStage != STAGE_IDLE)
        {
            stageTimes[previousStage] += now - stageStartTime;
        }

        stageStartTime = now;
        previousStage = calibrationStage;
    }

    if (calibrationState != CALIBRATION_RUNNING)
    {
        return;
    }

    switch (calibrationStage)
    {
        case STAGE_INITIALISE:

            // Wemos D1 mini:
            // D2 = SDA
            // D1 = SCL

            Wire.begin(D2, D1);
            Serial.println("I2C started");
            Serial.println("Initialising MPU6050...");
            calibrationmpu.initialize();

            deviceId = calibrationmpu.getDeviceID();

            for (int i = 0; i < 10; i++) {
                deviceId = calibrationmpu.getDeviceID();
                Serial.printf("Read %d: device ID = 0x%02X\n", i + 1, deviceId);
                delay(100);
            }
            Serial.println();
            Serial.println("MPU6050 initialisation returned");

            if (!calibrationmpu.testConnection())
            {
                Serial.println("ERROR: MPU6050 connection failed!");

                calibrationState = CALIBRATION_FAILED;
                calibrationStage = STAGE_IDLE;
                return;
            }
            Serial.println("MPU6050 connection successful.");
            calibrationmpu.setXAccelOffset(0);
            calibrationmpu.setYAccelOffset(0);
            calibrationmpu.setZAccelOffset(0);

            calibrationmpu.setXGyroOffset(0);
            calibrationmpu.setYGyroOffset(0);
            calibrationmpu.setZGyroOffset(0);

            Serial.println();
            Serial.println("Place the bell stationary.");
            Serial.println("Keep the MPU6050 horizontal");
            Serial.println("with the package facing upwards.");
            Serial.println("Do not move the bell.");

            calibrationStartTime = millis();

            calibrationStage = STAGE_WAIT;

            break;


        case STAGE_WAIT:

            if (millis() - calibrationStartTime >= 3000)
            {
                Serial.println();
                Serial.println("Starting calibration...");

                calibrationStage = STAGE_INITIAL_MEASURE;
            }

            break;


        case STAGE_INITIAL_MEASURE:

            if (meanSensors())
            {
                calibrationStage = STAGE_INITIAL_OFFSETS;
            }

            break;


        case STAGE_INITIAL_OFFSETS:

            // Calculate the initial offsets exactly as before

            ax_offset = -mean_ax / 8;
            ay_offset = -mean_ay / 8;
            az_offset = (16384 - mean_az) / 8;

            gx_offset = -mean_gx / 4;
            gy_offset = -mean_gy / 4;
            gz_offset = -mean_gz / 4;

            calibrationIteration = 0;

            calibrationStage = STAGE_APPLY_OFFSETS;

            break;

        case STAGE_APPLY_OFFSETS:

            offsetStart = millis();

            calibrationmpu.setXAccelOffset(ax_offset);
            calibrationmpu.setYAccelOffset(ay_offset);
            calibrationmpu.setZAccelOffset(az_offset);

            calibrationmpu.setXGyroOffset(gx_offset);
            calibrationmpu.setYGyroOffset(gy_offset);
            calibrationmpu.setZGyroOffset(gz_offset);

            offsetTime = millis() - offsetStart;

            Serial.print("Assigning offsets took ");
            Serial.print(offsetTime);
            Serial.println(" ms");


            calibrationIteration++;

            Serial.print("Calibration iteration ");
            Serial.println(calibrationIteration);

            calibrationStage = STAGE_CALIBRATION_MEASURE;

            break;


        case STAGE_CALIBRATION_MEASURE:

            // meanSensors() takes many calls to complete.
            // Once complete, move to the check stage.

            if (meanSensors())
            {
                calibrationStage = STAGE_CALIBRATION_CHECK;
            }

            break;


        case STAGE_CALIBRATION_CHECK:

            calibrationReady = 0;

            if (abs(mean_ax) <= ACCEL_DEADZONE)
                calibrationReady++;

            if (abs(mean_ay) <= ACCEL_DEADZONE)
                calibrationReady++;

            if (abs(16384 - mean_az) <= ACCEL_DEADZONE)
                calibrationReady++;

            if (abs(mean_gx) <= GYRO_DEADZONE)
                calibrationReady++;

            if (abs(mean_gy) <= GYRO_DEADZONE)
                calibrationReady++;

            if (abs(mean_gz) <= GYRO_DEADZONE)
                calibrationReady++;

            Serial.print("Ready: ");
            Serial.print(calibrationReady);
            Serial.println(" / 6");

            if (calibrationReady == 6)
            {
                printCalibration();
                saveCalibration();
                printCalibrationTiming();

                calibrationState = CALIBRATION_COMPLETE;
                calibrationStage = STAGE_COMPLETE;


                break;
            }

            // Not ready, so adjust the offsets exactly as before

            if (abs(mean_ax) > ACCEL_DEADZONE)
                ax_offset -= mean_ax / 8;

            if (abs(mean_ay) > ACCEL_DEADZONE)
                ay_offset -= mean_ay / 8;

            if (abs(16384 - mean_az) > ACCEL_DEADZONE)
                az_offset += (16384 - mean_az) / 8;

            if (abs(mean_gx) > GYRO_DEADZONE)
                gx_offset -= mean_gx / 4;

            if (abs(mean_gy) > GYRO_DEADZONE)
                gy_offset -= mean_gy / 4;

            if (abs(mean_gz) > GYRO_DEADZONE)
                gz_offset -= mean_gz / 4;

            // Start another measurement cycle

            calibrationStage = STAGE_APPLY_OFFSETS;

            break;


        case STAGE_COMPLETE:
        case STAGE_IDLE:
            break;

    }
}

void printCalibrationTiming()
{
    stageTimes[calibrationStage] += millis() - stageStartTime;

    Serial.println();
    Serial.println("==============================");
    Serial.println("CALIBRATION TIMING");
    Serial.println("==============================");

    Serial.print("INITIALISE:           ");
    Serial.print(stageTimes[STAGE_INITIALISE]);
    Serial.println(" ms");

    Serial.print("WAIT:                 ");
    Serial.print(stageTimes[STAGE_WAIT]);
    Serial.println(" ms");

    Serial.print("INITIAL MEASURE:     ");
    Serial.print(stageTimes[STAGE_INITIAL_MEASURE]);
    Serial.println(" ms");

    Serial.print("INITIAL OFFSETS:     ");
    Serial.print(stageTimes[STAGE_INITIAL_OFFSETS]);
    Serial.println(" ms");

    Serial.print("APPLY OFFSETS:       ");
    Serial.print(stageTimes[STAGE_APPLY_OFFSETS]);
    Serial.println(" ms");

    Serial.print("CALIBRATION MEASURE: ");
    Serial.print(stageTimes[STAGE_CALIBRATION_MEASURE]);
    Serial.println(" ms");

    Serial.print("CALIBRATION CHECK:   ");
    Serial.print(stageTimes[STAGE_CALIBRATION_CHECK]);
    Serial.println(" ms");
}




