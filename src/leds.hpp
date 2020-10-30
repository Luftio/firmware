#pragma once

namespace Leds
{
    // 0 = perfect
    // 255 = worst air ever
    extern unsigned char status;

    enum Animation : unsigned char
    {
        OFF, STARTUP, SETUP, STANDARD, LAMP
    };

    extern Animation animation;

    void begin();
    
    void setAnimation(Animation newAnimation);
    void setStatus(unsigned char status);
    void setBrightness(unsigned char brightness);

    void TaskLedAnimateRun(void *parameter);
    void update();
}