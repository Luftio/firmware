#ifdef ENABLE_CCS
#pragma once

#include <ccs811.h>
#include "../sensors.hpp"

namespace Sensors
{
    static CCS811 ccs;

    void initCCS()
    {
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
    }

    void readCCS()
    {
        uint16_t new_eco2,
            new_etvoc, errstat, raw;
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
}
#endif