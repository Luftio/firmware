#pragma once
#include <MHZ19.h>
#include "../sensors.hpp"

namespace Sensors
{
    HardwareSerial hwSerial1(1);

    static MHZ19 mhz(&hwSerial1);

    void readMHZ()
    {

        MHZ19_RESULT response = mhz.retrieveData();
        if (response == MHZ19_RESULT_OK)
        {
            Serial.print(F("MHZ CO2: "));
            co2 = mhz.getCO2();
            Serial.print(co2);
            Serial.print(F(", temperature: "));
            Serial.print(mhz.getTemperature());
            Serial.print(F(", accuracy: "));
            co2_accuracy = mhz.getAccuracy();
            Serial.println(co2_accuracy);
        }
        else
        {
            Serial.print(F("MHZ Error, code: "));
            Serial.println(response);
        }
    }

    void initMHZ()
    {
        hwSerial1.begin(9600, SERIAL_8N1, MHZ_RX, MHZ_TX);
        mhz.setRange(MHZ19_RANGE_5000);
        readMHZ();
        readMHZ();
    }

    void mhzCalibrate()
    {
        mhz.calibrateZero();
    }
}