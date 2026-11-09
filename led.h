#ifndef LED_H
#define LED_H

enum LEDState
{
    LED_IDLE,
    LED_ON,
    LED_OFF,
    LED_FLASH,
    LED_REGISTER,
    LED_STRIKE
};

void LEDFlashOn();
void LEDFlashOff();
void LEDRegister(uint8_t);
void LEDStrike();

void LEDLoop();

#endif