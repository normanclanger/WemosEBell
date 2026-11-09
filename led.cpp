#include <Arduino.h>
#include "led.h"

LEDState requestedState=LED_IDLE;
unsigned long LEDTime;
uint8_t LEDState;
unsigned int flashTime; // period between changes
signed int seqCount, flashNo, flashCount;  // counter for sequences, no. of flashes per sequence, count of how many flashes have happened
unsigned int strikeTime=200; // time light is on when bell strikes

// functions that cause LEDLoop to do things

void LEDFlashOn()
{
  requestedState=LED_FLASH;
  LEDState=HIGH;  // off
  LEDTime=millis();
  flashTime=500;

}

void LEDFlashOff()
{
  requestedState=LED_OFF;

}

void LEDStrike()
{
  Serial.println("In LEDStrike");

  requestedState=LED_STRIKE;
  LEDTime=millis();
  digitalWrite(LED_BUILTIN,LOW);  // switch the LED on 
}

void LEDRegister(uint8_t bellNo)
{

  requestedState=LED_REGISTER;
  LEDState=HIGH; // off
  LEDTime=millis();
  flashTime=1000;
  seqCount=3;  // 3 sequences
  flashNo=bellNo*2;  // no of changes in sequence
  flashCount=bellNo*2;  // no of changes count down

}

void LEDLoop()
{
  if (requestedState!=LED_IDLE)
  {
    if (requestedState==LED_FLASH)
    {
      if (millis()-LEDTime>flashTime)  // time to change
      {
        LEDTime=millis();
        if (LEDState==HIGH)
          LEDState=LOW;
        else
          LEDState=HIGH;
        
        digitalWrite(LED_BUILTIN,LEDState);

      }
    }
    if (requestedState==LED_OFF)
    {
      digitalWrite(LED_BUILTIN,HIGH);
      LEDState=HIGH;  // switch it off
      requestedState=LED_IDLE;
    }

    if (requestedState==LED_REGISTER)
    {
      if (millis()-LEDTime>flashTime)  // time to change
      {
        LEDTime=millis();
        if (LEDState==HIGH)
          LEDState=LOW;
        else
          LEDState=HIGH;
        
        flashCount--;
        if(flashCount<=0)  // reached zero
          LEDState=HIGH;   // count down 4 gaps below the required number - but don't light LED, so a gap is introduced

        if(flashCount<=-4)  // reached 4 under - reset counters
        {
          seqCount--;
          flashCount=flashNo; // start again
        }

        if (seqCount==0)
          requestedState=LED_IDLE;

        digitalWrite(LED_BUILTIN,LEDState);  

        

      }
    }

    if (requestedState==LED_STRIKE)
    {
      if (millis()-LEDTime>strikeTime)  // time to switch off
      {
        digitalWrite(LED_BUILTIN,HIGH);
        requestedState=LED_IDLE; 
      }
    }

  }

}


