#ifndef CALIBRATION_H
#define CALIBRATION_H

enum CalibrationState
{
    CALIBRATION_IDLE,
    CALIBRATION_RUNNING,
    CALIBRATION_COMPLETE,
    CALIBRATION_FAILED
};

bool calibrate();
void calibrationLoop();
CalibrationState getCalibrationState();
void printCalibrationTiming();

#endif