#pragma once

namespace Sensors
{
    void begin();
    void debugI2C();

    bool isWarmedUp();
    unsigned short readCO2();
    float readTemperature();
    float readPressure();
    float readTemperatureAlt();
    float readHumidity();

    void TaskSensorsCalibrateRun(void *parameter);
} // namespace Sensors