#include "config.h"
#include "webInterface.h"
#include "strikeDetection.h"
#include "calibration.h"
#include "network.h"
#include "led.h"

// PIN for switch used to put in webinterface mode
const int buttonPin=D5;


// bell runs in one of two modes - config or running - determined by whether config can be loaded

enum OperatingMode
{
    MODE_CONFIG,
    MODE_RUN
};

OperatingMode mode;
bool twoSecHold=false;  // assume button won't be held down

void setup()
{

    pinMode(buttonPin,INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(9600);
    delay(1000);  // give Serial port a chance
    
    // check for button being held down
    unsigned long buttonTime=millis();   // hold time at which button was pressed

    if (digitalRead(buttonPin)==LOW)  // pressed - so let's loop for 2 seconds
    {
        twoSecHold=true;
        Serial.println("Button LOW");
        digitalWrite(LED_BUILTIN,LOW);  // turn the internal LED on
        while(millis()-buttonTime<2000)
        {
            if (digitalRead(buttonPin)==HIGH)
            {
                twoSecHold=false;  // button released, so carry on normally
                digitalWrite(LED_BUILTIN,HIGH);  // turn the internal LED off
            }

            yield();
        }
        

    }

    digitalWrite(LED_BUILTIN,HIGH);  // turn the internal LED off
    Serial.print("Result of testing for button: ");
    Serial.println(twoSecHold);

    beginConfig();  // initiatise EEPROM, ready for use



    Serial.println("Checking config...");

    if (loadConfig() && !twoSecHold)  // if config loaded & button not held down
    {
        mode = MODE_RUN;
        Serial.println("Config loaded and button not pressed");
        startStrikeDetection();
    }
     else
    {
        mode = MODE_CONFIG;
        Serial.println("Config not loaded or button pressed");
        startWebInterface();
    }

}


void loop()
{
    if (mode == MODE_CONFIG)
    {
        calibrationLoop();
        webInterfaceLoop();
                
    }
    else
    {
        strikeDetectionLoop();
        networkLoop();
        LEDLoop();
       
    }
}





