#pragma once

namespace Sensors
{
    void begin();
    bool isWarmedUp();
    void TaskSensorsCalibrateRun(void *parameter);

    String getValuesJSON();
    extern uint16_t hum, temp, pressure, co2, eco2, etvoc, co2_accuracy, etvoc_accuracy;

    // Specific features
    void mhzCalibrate();

    // Utilities
    void debugI2C();
    void ema_filter(uint16_t current_value, uint16_t *exponential_average);
} // namespace Sensors