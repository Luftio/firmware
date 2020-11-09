#pragma once
#include <ESP32Encoder.h>
#include <AceButton.h>
using namespace ace_button;

namespace Input
{
    void begin();
    void update();
    void handleButton(AceButton *button, uint8_t eventType, uint8_t buttonState);
}