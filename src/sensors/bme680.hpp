#ifdef ENABLE_BME680
#pragma once

#include <bsec.h>
#include "../sensors.hpp"
#include "../wireless.hpp"

const uint8_t bsec_config_iaq[] = {
#include "config/generic_33v_3s_4d/bsec_iaq.txt"
};

namespace Sensors
{
    static Bsec bme680;

    bsec_virtual_sensor_t sensor_list_fast[5] = {
        BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY};

    bsec_virtual_sensor_t sensor_list[9] = {
        BSEC_OUTPUT_RAW_GAS,
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STATIC_IAQ,
        BSEC_OUTPUT_CO2_EQUIVALENT,
        BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
        BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
        BSEC_OUTPUT_COMPENSATED_GAS,
        BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS};

    void checkBME680(void)
    {
        if (bme680.status != BSEC_OK)
        {
            if (bme680.status < BSEC_OK)
            {
                Wireless::log("BSEC error code : " + String(bme680.status));
                debugI2C();
            }
            else
            {
                Wireless::log("BSEC warning code : " + String(bme680.status));
            }
        }
        if (bme680.bme680Status != BME680_OK)
        {
            if (bme680.bme680Status < BME680_OK)
            {
                Wireless::log("BME680 error code : " + String(bme680.bme680Status));
                debugI2C();
            }
            else
            {
                Wireless::log("BME680 warning code : " + String(bme680.bme680Status));
            }
        }
    }

    void initBME680()
    {
        bme680.begin(BME680_ADDR, Wire);
        Serial.println("BSEC library version " + String(bme680.version.major) + "." + String(bme680.version.minor) + "." + String(bme680.version.major_bugfix) + "." + String(bme680.version.minor_bugfix));
        bme680.setConfig(bsec_config_iaq);
        checkBME680();
        bme680.setTemperatureOffset(TEMP_OFFSET);
        bme680.updateSubscription(sensor_list, 9, BSEC_SAMPLE_RATE_ULP);
        bme680.updateSubscription(sensor_list_fast, 5, BSEC_SAMPLE_RATE_LP);
        checkBME680();
    }

    void readBME680()
    {
        if (bme680.run())
        {
            checkBME680();
            Serial.printf("BME680 Temperature %.2f, ", bme680.temperature);
            Serial.printf("Humidity %.2f, ", bme680.humidity);
            Serial.printf("Pressure %.2f kPa\n", bme680.pressure / 1000);
            Serial.printf("IAQ %.0f sIAQ %.0f\n", bme680.iaq, bme680.staticIaq);
            Serial.printf("ECO2 %.2f, BreathVOC %.2f accuracy %d\n", bme680.co2Equivalent, bme680.breathVocEquivalent, bme680.breathVocAccuracy);
            uint16_t new_temp = round(bme680.temperature * 10);
            uint16_t new_hum = round(bme680.humidity * 10);
            uint16_t new_pressure = round(bme680.pressure / 10);
            uint16_t new_tvoc = round(bme680.breathVocEquivalent * 100);
            uint16_t new_eco2 = round(bme680.co2Equivalent);
            uint16_t new_iaq = round(bme680.iaq);
            uint16_t new_siaq = round(bme680.staticIaq);
            ema_filter(new_hum, &hum);
            ema_filter(new_temp, &temp);
            // Filter erronous values
            if (new_pressure >= pressure - 1000)
            {
                ema_filter(new_pressure, &pressure);
            }
            ema_filter(new_tvoc, &etvoc);
            ema_filter(new_eco2, &eco2);
            ema_filter(new_iaq, &iaq);
            ema_filter(new_siaq, &siaq);
            siaq_accuracy = bme680.staticIaqAccuracy;
            etvoc_accuracy = bme680.breathVocAccuracy;
        }
        else
        {
            checkBME680();
        }
    }
}
#endif