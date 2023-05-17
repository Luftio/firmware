// Util
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define WIFI_ID "LUFTIO_"

#define HW_VERSION 3
#define FW_VERSION "hw_" STR(HW_VERSION) "-v113"

#define MQTT_SERVER "app.luftio.com"
#define MQTT_PORT 1883
#define MQTT_VERSION_3_1

// Access token key
// config-secret includes the definition for API_SECRET
#include "config-secret.hpp"
const uint8_t apiEncrypt[6] = API_SECRET;

// HW configs
#if HW_VERSION == 1
#define ENABLE_BME280 1
#define ENABLE_MHZ 1
#define TEMP_OFFSET 0.0
#elif HW_VERSION == 2
#define ENABLE_MHZ 1
#define ENABLE_BME680 1
#define BME680_ADDR 0x77
#define TEMP_OFFSET 0.0
#elif HW_VERSION == 3
#define ENABLE_SCD4X 1
#define ENABLE_BME680 1
#define BME680_ADDR 0x77
#define TEMP_OFFSET 7.0
#endif