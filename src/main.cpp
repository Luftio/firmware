#include <Arduino.h>
#include "sensors.hpp"
#include "leds.hpp"
#include "buttons.hpp"
#include "wireless.hpp"

#define WARNING_TIME 60000
#define CRITICAL_TIME 4000

#define WARNING_VALUE 2000
#define CRITICAL_VALUE 5000

void setup()
{
    delay(1000);

    Serial.begin(115200);
    Serial.println("Yay! Welcome to AirGuard debuging console.");

    Leds::animation = Leds::STARTUP;
    Leds::begin();

    Sensors::begin();
    Wireless::begin();
}

void loop()
{
    /*Buttons::one.update();
    if (Buttons::one.isJustReleased())
    {
        if (Leds::animation == Leds::STANDARD)
        {
            Leds::setAnimation(Leds::OFF);
        }
        else if (Leds::animation == Leds::OFF)
        {
            Leds::setAnimation(Leds::LAMP);
            Leds::setBrightness(255);
        }
        else if (Leds::animation == Leds::LAMP)
            Leds::setAnimation(Leds::STANDARD);
    }*/
}