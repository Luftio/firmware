#include <Wire.h>
#include <Adafruit_BME280.h>
#include <ccs811.h>
#include <MHZ19.h>

#include "sensors.hpp"
#include "leds.hpp"
#include "pinout.hpp"
#include "config.hpp"

namespace Sensors
{
    void readMHZ();

    TaskHandle_t TaskSensorsCalibrate;

    HardwareSerial hwSerial1(1);

#ifdef ENABLE_CCS
    static CCS811 ccs;
#endif
    static Adafruit_BME280 bme;
    static MHZ19 mhz(&hwSerial1);

    bool warmedUp = false;

    void begin()
    {
        // Initialize I2C
        Wire.begin(SDA, SCL);
        delay(100);

#ifdef ENABLE_CCS
        // Initialize CCS811
        ccs.set_i2cdelay(50);
        if (!ccs.begin())
        {
            Serial.println("CCS811 sensor not found!");
            debugI2C();
            delay(5000);
            begin();
            return;
        }
        Serial.print("CCS hardware version: ");
        Serial.println(ccs.hardware_version(), HEX);
        Serial.print("CCS bootloader version: ");
        Serial.println(ccs.bootloader_version(), HEX);
        Serial.print("CCS application version: ");
        Serial.println(ccs.application_version(), HEX);
        ccs.start(CCS811_MODE_10SEC);
#endif
        hwSerial1.begin(9600, SERIAL_8N1, MHZ_RX, MHZ_TX);
        mhz.setRange(MHZ19_RANGE_5000);
        readMHZ();
        readMHZ();

        // Initialize BME280
        if (!bme.begin(0x76))
        {
            Serial.println("BME280 sensor not found!");
            debugI2C();
            delay(5000);
            begin();
            return;
        }

        // Default settings from datasheet
        bme.setSampling(Adafruit_BME280::MODE_FORCED, // Operating mode
                        Adafruit_BME280::SAMPLING_X1, // Temp. oversampling
                        Adafruit_BME280::SAMPLING_X1, // Pressure oversampling
                        Adafruit_BME280::SAMPLING_X1, // Humidity oversampling
                        Adafruit_BME280::FILTER_OFF); // Filtering

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

    void ema_filter(uint16_t current_value, uint16_t *exponential_average)
    {
        if (*exponential_average == 0)
        {
            *exponential_average = current_value;
            return;
        }
        *exponential_average = 0.7 * current_value + 0.3 * (*exponential_average);
    }

    uint16_t hum, temp, pressure, co2, eco2, etvoc;

    uint16_t readCO2()
    {
        return co2;
    }
    uint16_t readECO2()
    {
        return eco2;
    }

    uint16_t readTVOC()
    {
        return etvoc;
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

    uint32_t readPressure()
    {
        return pressure * 10;
    }

    float readHumidity()
    {
        return (float)hum / 10;
    }

#ifdef ENABLE_CCS
    uint16_t ccs_readBaseline()
    {
        uint16_t new_baseline;
        if (ccs.get_baseline(&new_baseline))
        {
            return new_baseline;
        }
        return 0;
    }

    bool ccs_writeBaseline(uint16_t baseline)
    {
        return ccs.set_baseline(baseline);
    }

    void readCCS()
    {
        uint16_t new_eco2, new_etvoc, errstat, raw;
        ccs.read(&new_eco2, &new_etvoc, &errstat, &raw);
        if (errstat == CCS811_ERRSTAT_OK)
        {
            ema_filter(new_eco2, &eco2);
            ema_filter(new_etvoc, &etvoc);
        }
        else if (errstat == CCS811_ERRSTAT_OK_NODATA)
        {
            // CCS waiting for new data
        }
        else if (errstat & CCS811_ERRSTAT_I2CFAIL)
        {
            Serial.println("CCS I2C error");
        }
        else
        {
            Serial.print("CCS error: ");
            Serial.println(ccs.errstat_str(errstat));
        }
    }
#endif

    void mhz_calibrate()
    {
        mhz.calibrateZero();
    }

    void readMHZ()
    {

        MHZ19_RESULT response = mhz.retrieveData();
        if (response == MHZ19_RESULT_OK)
        {
            Serial.print(F("MHZ CO2: "));
            co2 = mhz.getCO2();
            Serial.println(co2);
            Serial.print(F("MHZ Temperature: "));
            Serial.println(mhz.getTemperature());
            Serial.print(F("MHZ Accuracy: "));
            Serial.println(mhz.getAccuracy());
        }
        else
        {
            Serial.print(F("MHZ Error, code: "));
            Serial.println(response);
        }
    }

    void readBME()
    {
        bme.takeForcedMeasurement();
        uint16_t new_hum = round(bme.readHumidity() * 10);
        uint16_t new_temp = round(bme.readTemperature() * 10);
        Serial.print("BME raw temp: ");
        Serial.println(new_temp);
        uint16_t new_pressure = round(bme.readPressure() / 10);
        if (new_temp <= 0 || new_pressure < 5000 || new_temp >= 500)
        {
            return;
        }
        ema_filter(new_hum, &hum);
        ema_filter(new_temp, &temp);
        ema_filter(new_pressure, &pressure);

#ifdef ENABLE_CCS
        double avg_temp = temp;
        double fract = modf(avg_temp, &avg_temp);
        uint16_t tempHIGH = (((uint16_t)avg_temp + 25) << 9);
        uint16_t tempLOW = ((uint16_t)(fract / 0.001953125) & 0x1FF);
        uint16_t tempCONVERT = (tempHIGH | tempLOW);
        uint16_t humCONVERT = hum << 1 | 0x00;
        ccs.set_envdata(tempCONVERT, humCONVERT);
#endif
    }

    void TaskSensorsCalibrateRun(void *parameter)
    {
        for (;;)
        {
            readMHZ();
            readBME();
#ifdef ENABLE_CCS
            readCCS();
#endif

            // Print values
            Serial.print("Sensors ECO2 = ");
            Serial.print(readECO2());
            Serial.print(" TVOC = ");
            Serial.print(readTVOC());
            Serial.print(" hum = ");
            Serial.print(readHumidity());
            Serial.print(" pres = ");
            Serial.print(readPressure());
            Serial.print(" temp = ");
            Serial.println(readTemperature());

            // Show status
            Leds::setStatus(max(0, min((co2 - 500) / 16, 255)));
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }
} // namespace Sensors