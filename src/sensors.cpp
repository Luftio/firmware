#include <Wire.h>
#include "config.hpp"

#include "sensors.hpp"
#include "leds.hpp"
#include "pinout.hpp"

#include "sensors/mhz19.hpp"
#include "sensors/ccs.hpp"
#include "sensors/bme280.hpp"
#include "sensors/bme680.hpp"

namespace Sensors
{
    TaskHandle_t TaskSensorsCalibrate;

    uint16_t hum, temp, pressure, co2, eco2, etvoc, co2_accuracy, etvoc_accuracy;
    bool warmedUp = false;

    void begin()
    {
        // Initialize I2C
        Wire.begin(SDA, SCL);
        delay(100);

        initMHZ();
#ifdef ENABLE_CCS
        initCCS();
#endif
#ifdef ENABLE_BME280
        initBME280();
#endif
#ifdef ENABLE_BME680
        initBME680();
#endif

        Serial.println("All sensors found succesfully.");
        // Set up calibration task
        xTaskCreate(
            TaskSensorsCalibrateRun,
            "TaskSensorsCalibrate",
            10000,
            NULL,
            0,
            &TaskSensorsCalibrate);
    }

    bool isWarmedUp()
    {
        if (warmedUp)
        {
            return true;
        }
        if (millis() > 300000)
        {
            warmedUp = true;
            return true;
        }
        return false;
    }

    float readTemperature()
    {
        /*Serial.print("Raw temperature: ");
        Serial.print(temp / 10);
        Serial.print(", adjusting by index ");
        float readjIndex = 0.78;
        if (Leds::animation != Leds::OFF)
        {
            readjIndex -= ((float)Leds::brightness / (float)255) * 0.02;
        }
        Serial.println(readjIndex);*/
        return ((float)temp / 10) * 1;
    }

    String getValuesJSON()
    {
        String body = "{";
        body += "\"co2\":";
        body += co2;
        body += ",\"co2_accuracy\":";
        body += co2_accuracy;
        body += ",\"hum\":";
        body += ((float)hum / 10);
        body += ",\"temp\":";
        body += Sensors::readTemperature();
        body += ",\"pres\":";
        body += (pressure * 10);
#ifdef ENABLE_BME680
        body += ",\"eco2\":";
        body += eco2;
        body += ",\"tvoc\":";
        body += ((float)etvoc / 100);
        body += ",\"tvoc_accuracy\":";
        body += etvoc_accuracy;
#endif
#ifdef ENABLE_CCS
        body += ",\"eco2\":";
        body += eco2;
        body += ",\"tvoc\":";
        body += etvoc;
#endif
        body += "}";
        return body;
    }

    void TaskSensorsCalibrateRun(void *parameter)
    {
        for (;;)
        {
            readMHZ();
#ifdef ENABLE_BME280
            readBME280();
#endif
#ifdef ENABLE_BME680
            readBME680();
#endif
#ifdef ENABLE_CCS
            readCCS();
#endif

            // Show status
            Leds::setStatus(max(0, min((co2 - 500) / 11, 255)));
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }

    void debugI2C()
    {
        Serial.println(" Scanning I2C Addresses");
        uint8_t cnt = 0;
        for (uint8_t i = 0; i < 128; i++)
        {
            Wire.beginTransmission(i);
            uint8_t ec = Wire.endTransmission(true);
            if (ec == 0)
            {
                if (i < 16)
                    Serial.print('0');
                Serial.print(i, HEX);
                cnt++;
            }
            else
                Serial.print("..");
            Serial.print(' ');
            if ((i & 0x0f) == 0x0f)
                Serial.println();
        }
        Serial.print("Scan Completed, ");
        Serial.print(cnt);
        Serial.println(" I2C Devices found.");
    }

    void ema_filter(uint16_t current_value, uint16_t *exponential_average)
    {
        if (*exponential_average == 0)
        {
            *exponential_average = current_value;
            return;
        }
        *exponential_average = 0.6 * current_value + 0.4 * (*exponential_average);
    }
} // namespace Sensors