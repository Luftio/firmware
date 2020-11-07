#include "FastLED.h"

#include "leds.hpp"
#include "pinout.hpp"

namespace Leds
{
    TaskHandle_t TaskLedAnimate;

    CRGB leds[LED_COUNT];

    Animation animation = OFF;
    unsigned char brightness = 255;
    unsigned char status = 0;
    unsigned char lastStatus = 0;

    void begin()
    {
        FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_COUNT).setCorrection(UncorrectedColor);
        FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);

        FastLED.setBrightness(255);

        // Set up animation task
        xTaskCreate(
            TaskLedAnimateRun,
            "TaskLedAnimate",
            10000,
            NULL,
            0,
            &TaskLedAnimate);
    }

    void update()
    {
        if (animation == STANDARD)
        {
            for (int i = 1; i <= abs(status - lastStatus); i++)
            {
                if (status < lastStatus)
                {
                    FastLED.showColor(CRGB(lastStatus - i, 255 - lastStatus + i, 0), brightness);
                }
                else
                {
                    FastLED.showColor(CRGB(lastStatus + i, 255 - lastStatus - i, 0), brightness);
                }
                vTaskDelay(min(1000 / abs(status - lastStatus), 50) / portTICK_PERIOD_MS);
            }
            FastLED.showColor(CRGB(status, 255 - status, 0), brightness);
            lastStatus = status;
        }
        else if (animation == STARTUP || animation == SETUP)
        {
            FastLED.clear();
            unsigned long time = millis();

            for (unsigned char i = 0; i < LED_COUNT; i++)
            {
                if (time / 2 > i * 256 / LED_COUNT)
                {
                    unsigned char c = sin8(time / 2 - i * 256 / LED_COUNT);
                    leds[i] = CRGB((animation == SETUP) ? 0 : c, (animation == SETUP) ? 0 : c, c);
                }
            }
            FastLED.show();
        }
        else if (animation == LAMP)
        {
            FastLED.showColor(CRGB::SandyBrown, brightness);
        }
        else if (animation == OFF)
        {
            FastLED.clear(true);
            delay(3);
            FastLED.clear(true);
        }

        if (animation == STARTUP || animation == SETUP)
        {
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        else
        {
            vTaskSuspend(NULL);
        }
    }

    void setAnimation(Animation newAnimation)
    {
        animation = newAnimation;
        vTaskResume(TaskLedAnimate);
    }

    void setBrightness(unsigned char newBrightness)
    {
        brightness = newBrightness;
        vTaskResume(TaskLedAnimate);
    }

    void setStatus(unsigned char newStatus)
    {
        status = newStatus;

        if (animation == STANDARD)
        {
            vTaskResume(TaskLedAnimate);
        }
    }

    void TaskLedAnimateRun(void *parameter)
    {
        for (;;)
        {
            update();
        }
    }
} // namespace Leds