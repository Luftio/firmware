#ifdef ENABLE_BME280
#pragma once

#include <Adafruit_BME280.h>
#include "../sensors.hpp"
#include "../wireless.hpp"

namespace Sensors
{
    static Adafruit_BME280 bme;

    void initBME280()
    {
        if (!bme.begin(0x76))
        {
            Wireless::log("BME280 sensor not found!");
            debugI2C();
            return;
        }

        // Default settings from datasheet
        bme.setSampling(Adafruit_BME280::MODE_FORCED, // Operating mode
                        Adafruit_BME280::SAMPLING_X1, // Temp. oversampling
                        Adafruit_BME280::SAMPLING_X1, // Pressure oversampling
                        Adafruit_BME280::SAMPLING_X1, // Humidity oversampling
                        Adafruit_BME280::FILTER_OFF); // Filtering
    }

    void readBME280()
    {
        if(!bme.takeForcedMeasurement()) {
            Wireless::log("BME280 take measurement failed!");
        }
        uint16_t new_temp = round(bme.readTemperature() * 10);
        Serial.print("BME raw temp: ");
        Serial.print(new_temp);
        uint16_t new_hum = round(bme.readHumidity() * 10);
        Serial.print(" raw hum: ");
        Serial.print(new_hum);
        uint16_t new_pressure = round(bme.readPressure() / 10);
        Serial.print(" raw pres: ");
        Serial.println(new_pressure);
        if (new_temp <= 0 || new_temp >= 500 || new_hum < 50 || new_hum > 950 || new_pressure < 5000 || new_pressure > 20000)
        {
            Serial.println("Discarding based on extreme");
            return;
        }
        if ((temp != 0 && abs(temp - new_temp) > 100) || (hum != 0 && abs(hum - new_hum) > 200) || (pressure != 0 && abs(pressure - new_pressure) > 500))
        {
            Serial.println("Discarding based on difference");
            return;
        }
        ema_filter(new_hum, &hum);
        ema_filter(new_temp, &temp);
        ema_filter(new_pressure, &pressure);
    }
}
#endif