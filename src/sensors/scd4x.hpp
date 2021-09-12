#ifdef ENABLE_SCD4X
#pragma once

#include <SensirionI2CScd4x.h>
#include "../sensors.hpp"
#include "../wireless.hpp"

namespace Sensors
{
    SensirionI2CScd4x scd4x;

    uint16_t error;
    char errorMessage[256];
    float aux_temp;
    float aux_hum;

    void readSCD4X()
    {
        uint16_t readyStatus;
        error = scd4x.getDataReadyStatus(readyStatus);
        if (error)
        {
            errorToString(error, errorMessage, 256);
            Wireless::log("SCD4X: Error trying to execute getDataReadyStatus():" + String(errorMessage));
        }

        uint16_t tmp_co2;
        float tmp_temp;
        float tmp_hum;
        error = scd4x.readMeasurement(tmp_co2, tmp_temp, tmp_hum);
        if (error)
        {
            errorToString(error, errorMessage, 256);
            Wireless::log("SCD4X: Error trying to execute readMeasurement():" + String(errorMessage));
        }
        else
        {
            Serial.println("SCD4X: " + String(tmp_co2) + "ppm, " + String(tmp_temp) + "C , " + String(tmp_hum) + "%");
            co2 = tmp_co2;
            aux_temp = tmp_temp;
            aux_hum = tmp_hum;
        }
    }

    void printUint16Hex(uint16_t value)
    {
        Serial.print(value < 4096 ? "0" : "");
        Serial.print(value < 256 ? "0" : "");
        Serial.print(value < 16 ? "0" : "");
        Serial.print(value, HEX);
    }

    void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2)
    {
        Serial.print("Serial: 0x");
        printUint16Hex(serial0);
        printUint16Hex(serial1);
        printUint16Hex(serial2);
        Serial.println();
    }

    void initSCD4X()
    {
        scd4x.begin(Wire);

        error = scd4x.stopPeriodicMeasurement();
        if (error)
        {
            Serial.print("SCD4X: Error trying to execute stopPeriodicMeasurement(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
        }

        uint16_t serial0;
        uint16_t serial1;
        uint16_t serial2;
        error = scd4x.getSerialNumber(serial0, serial1, serial2);
        if (error)
        {
            Serial.print("SCD4X: Error trying to execute getSerialNumber(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
        }
        else
        {
            printSerialNumber(serial0, serial1, serial2);
        }

        uint16_t sensorStatus;
        error = scd4x.performSelfTest(sensorStatus);
        if (error)
        {
            Serial.print("SCD4X: Error trying to execute performSelfTest(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
        }
        Wireless::log("SCD4X: sensorStatus 0x" + String((unsigned int)sensorStatus, 16));
        scd4x.setTemperatureOffset(TEMP_OFFSET);

        // Start Measurement
        error = scd4x.startLowPowerPeriodicMeasurement(); //startPeriodicMeasurement();
        if (error)
        {
            Serial.print("SCD4X: Error trying to execute startPeriodicMeasurement(): ");
            errorToString(error, errorMessage, 256);
            Serial.println(errorMessage);
        }
    }

    void calibrateCO2()
    {
        uint16_t frcCorrection = 0;
        scd4x.performForcedRecalibration(400, frcCorrection);
    }
}
#endif