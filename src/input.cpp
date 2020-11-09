#include "input.hpp"
#include "pinout.hpp"
#include "leds.hpp"
#include "wireless.hpp"

namespace Input
{
    AceButton button(BUTTON_PIN);
    void handleButton(AceButton *, uint8_t, uint8_t);
    ESP32Encoder encoder;

    unsigned long lastValueChange = 0;

    void begin()
    {
        pinMode(ONBOARDLED_PIN, OUTPUT);
        digitalWrite(ONBOARDLED_PIN, 0);
        pinMode(BUTTON_PIN, INPUT_PULLUP);
        button.setEventHandler(handleButton);

        ESP32Encoder::useInternalWeakPullResistors = UP;
        encoder.attachHalfQuad(ENCODER_PIN1, ENCODER_PIN2);
        encoder.setCount(0);
    }

    void update()
    {
        button.check();
        int32_t newValue = encoder.getCount();
        if (newValue != 0)
        {
            Leds::setBrightness(min(max(Leds::brightness + newValue * 3, 0), 255));
            lastValueChange = millis();
            encoder.setCount(0);
        }
        if (lastValueChange != 0 && lastValueChange < millis() - 10000)
        {
            Wireless::uploadLightAttributes(Leds::animation, Leds::brightness);
            lastValueChange = 0;
        }
    }

    void handleButton(AceButton *button, uint8_t eventType, uint8_t buttonState)
    {
        if (eventType == AceButton::kEventReleased)
        {
            if (Leds::animation == Leds::STANDARD)
            {
                Leds::setAnimation(Leds::OFF);
            }
            else if (Leds::animation == Leds::OFF)
            {
                Leds::setAnimation(Leds::LAMP);
            }
            else if (Leds::animation == Leds::LAMP)
            {
                Leds::setAnimation(Leds::STANDARD);
            }
            Wireless::uploadLightAttributes(Leds::animation, Leds::brightness);
        }
    }
} // namespace Input