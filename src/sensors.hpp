#pragma once

namespace Sensors
{
    void begin();
    void debugI2C();

    bool isWarmedUp();
    uint16_t readCO2();
    uint16_t readTVOC();
    float readTemperature();
    uint32_t readPressure();
    float readHumidity();
    uint16_t readBaseline();
    bool writeBaseline(uint16_t baseline);

    void TaskSensorsCalibrateRun(void *parameter);
} // namespace Sensors