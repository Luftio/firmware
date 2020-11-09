#pragma once

namespace Sensors
{
    void begin();
    void debugI2C();

    bool isWarmedUp();
    uint16_t readCO2();
    uint16_t readECO2();
    uint16_t readTVOC();
    float readTemperature();
    uint32_t readPressure();
    float readHumidity();

    uint16_t ccs_readBaseline();
    bool ccs_writeBaseline(uint16_t baseline);
    void mhz_calibrate();

    void TaskSensorsCalibrateRun(void *parameter);
} // namespace Sensors