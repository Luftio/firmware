# Luftio firmware

Firmware for the Luftio smart quality sensor. PlatformIO project for the ESP32 platform.

## Features

- Reading values from supported sensors
- MQTT communication
- WiFi with captive portal setup
- OTA updates
- Animated LED indication

## Hardware

Latest stack (Luftio v3):

- ESP32 WROOM
- SCD41 sensor for CO2
- BME680 sensor for VOC, temperature, humidity
- WS2812B LED ring for indiciation

Older supported sensors:

- BME280
- CCS811
- MHZ19
