#include "sensors.hpp"
#include "leds.hpp"
#include "pinout.hpp"

#include <Wire.h>
#include <Adafruit_CCS811.h>
#include <Adafruit_BME280.h>

namespace Sensors
{
    TaskHandle_t TaskSensorsCalibrate;

    static Adafruit_CCS811 ccs;
    static Adafruit_BME280 bme;

    bool warmedUp = false;

    void begin()
    {
        // Initialize I2C
        Wire.begin(SDA, SCL);
        delay(100);

        // Initialize BME280
        if (!bme.begin(0x76))
        {
            Serial.println("BME280 sensor not found!");
            debugI2C();
            delay(5000);
            begin();
            return;
        }

        // Initialize CCS811
        if (!ccs.begin())
        {
            Serial.println("CCS811 sensor not found!");
            debugI2C();
            delay(5000);
            begin();
            return;
        }

        // Default settings from datasheet
        bme.setSampling(Adafruit_BME280::MODE_NORMAL,     // Operating mode
                        Adafruit_BME280::SAMPLING_X2,     // Temp. oversampling
                        Adafruit_BME280::SAMPLING_X16,    // Pressure oversampling
                        Adafruit_BME280::SAMPLING_X16,    // Humidity oversampling
                        Adafruit_BME280::FILTER_X16,      // Filtering
                        Adafruit_BME280::STANDBY_MS_500); // Standby time
        ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);

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

    unsigned short readCO2()
    {
        if (ccs.available())
            ccs.readData();

        return ccs.geteCO2();
    }

    unsigned short readTVOC()
    {
        if (ccs.available())
            ccs.readData();

        return ccs.getTVOC();
    }

    float readTemperature()
    {
        return bme.readTemperature();
    }

    float readPressure()
    {
        return bme.readPressure();
    }

    float readHumidity()
    {
        return bme.readHumidity();
    }

    void TaskSensorsCalibrateRun(void *parameter)
    {
        for (int i = 0; true; i++)
        {
            float hum = readHumidity();
            float temp = readTemperature();
            ccs.setEnvironmentalData(readHumidity(), readTemperature());
            unsigned short CO2 = readCO2();
            Serial.print("Sensors CO2 = ");
            Serial.print(CO2);
            Serial.print(" TVOC = ");
            Serial.print(readTVOC());
            Serial.print(" hum = ");
            Serial.print(hum);
            Serial.print(" pres = ");
            Serial.print(readPressure());
            Serial.print(" temp = ");
            Serial.println(temp);
            Leds::setStatus(min((CO2 - 400) / 16, 255));
            vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
    }
} // namespace Sensors