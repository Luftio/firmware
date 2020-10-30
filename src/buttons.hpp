#pragma once

namespace Buttons
{
    class Button
    {
    private:
        unsigned char pin;
        bool state, prevState;
    
    public:
        Button(unsigned char pin);
        void update();
        bool isJustPressed();
        bool isJustReleased();
        bool isPressed();
    };
    
    extern Button one;
}