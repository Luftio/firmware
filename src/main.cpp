#include <Arduino.h>

#include "sensors.hpp"
#include "leds.hpp"
#include "wireless.hpp"

void setup()
{
    delay(1000);

    Serial.begin(115200);
    Serial.println("Yay! Welcome to Luftio debuging console.");

    // Init
    Leds::animation = Leds::STARTUP;
    Leds::begin();
    Sensors::begin();
    Wireless::begin();
}

void loop()
{
    Wireless::loop();
}