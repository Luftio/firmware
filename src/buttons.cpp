#include "buttons.hpp"

#include "pinout.hpp"
#include <Arduino.h>

namespace Buttons
{
    Button::Button(uint8_t pin)
        : pin(pin), state(false), prevState(false)
    {
        pinMode(pin, INPUT_PULLUP);
    }

    void Button::update()
    {
        prevState = state;
        state = !digitalRead(pin);
    }

    bool Button::isJustPressed()
    {
        return state && !prevState;
    }

    bool Button::isJustReleased()
    {
        return !state && prevState;
    }

    bool Button::isPressed()
    {
        return state;
    }

    Button one(BUTTON1_PIN);
}