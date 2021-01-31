#include "WiFi.h"
#include "DNSServer.h"
#include "HTTPClient.h"
#include "PubSubClient.h"
#include "ArduinoJson.h"

#include "wireless.hpp"
#include "sensors.hpp"
#include "config.hpp"
#include "web.hpp"
#include "leds.hpp"
#include "otaupdate.hpp"

namespace Wireless
{
    Preferences preferences;
    bool connecting = false;
    std::unique_ptr<DNSServer> dnsServer;

    WiFiClient wifiClient;
    PubSubClient mqttClient(wifiClient);

    char macStr[20];
    char authKey[20];
    char apSSID[20];

    void begin()
    {
        preferences.begin("luftio-wifi", false);
        uint8_t mac[6];

        esp_efuse_mac_get_default(mac);
        snprintf(apSSID, 20, "%s%02X%02X", WIFI_ID, mac[4], mac[5]);
        snprintf(macStr, 20, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        snprintf(authKey, 20, "%02X%02X%02X%02X%02X%02X", mac[0] ^ apiEncrypt[0], mac[1] ^ apiEncrypt[1], mac[2] ^ apiEncrypt[2], mac[3] ^ apiEncrypt[3], mac[4] ^ apiEncrypt[4], mac[5] ^ apiEncrypt[5]);
        Serial.println(macStr);
        Serial.println(apSSID);
        Serial.println(authKey);

        connect();

        randomSeed(micros());
        Leds::setAnimation(Leds::OFF);

        mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
        mqttClient.setCallback(mqttCallback);
    }

    void provision()
    {
        unsigned long idleTimer = millis();
        Leds::setAnimation(Leds::SETUP);
        Serial.println("Begining provisioning");
        WiFi.disconnect();
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(apSSID, NULL, 1, 0, 4);
        WiFi.scanNetworks(true, false, false, 1000);

        dnsServer.reset(new DNSServer());
        dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer->start(53, "*", WiFi.softAPIP());
        Web::begin();

        for (;;)
        {
            if (WiFi.softAPgetStationNum() != 0)
            {
                idleTimer = millis();
            }
            else if (millis() > 120000 && idleTimer < millis() - 120000)
            {
                String savedWifi = preferences.getString("wifi_name", "");
                if (savedWifi.length() != 0)
                {
                    Serial.println("Timed out, trying to connect again");
                    connecting = true;
                }
            }
            dnsServer->processNextRequest();
            Web::handleClient();

            if (connecting)
            {
                connecting = false;
                dnsServer->stop();
                dnsServer.reset();
                Web::close();
                WiFi.softAPdisconnect();
                ESP.restart();
                break;
            }
        }
    }

    void saveNetwork(String name, String pass)
    {
        preferences.putString("wifi_name", name);
        preferences.putString("wifi_pass", pass);
        connecting = true;
    }

    void connect()
    {
        Leds::setAnimation(Leds::STARTUP);
        if (preferences.getBytesLength("wifi_name") < 0)
        {
            // First setup
            preferences.putString("wifi_name", "");
            preferences.putString("wifi_pass", "");
        }
        String savedWifi = preferences.getString("wifi_name", "");
        String savedWifiPass = preferences.getString("wifi_pass", "");
        if (savedWifi.length() == 0)
        {
            provision();
            return;
        }
        Serial.print("Connecting to saved WiFi ");
        Serial.println(savedWifi);
        WiFi.begin(savedWifi.c_str(), savedWifiPass.c_str());

        int maxAttempts = 60;
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; i++)
        {
            if (WiFi.SSID(i) == savedWifi)
            {
                Serial.println("Network found on scan");
                maxAttempts = 100;
                break;
            }
        }
        int connectionAttempts = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            connectionAttempts++;
            if (connectionAttempts >= maxAttempts)
            {
                provision();
                return;
            }
            else if (connectionAttempts % 20 == 0)
            {
                WiFi.begin(savedWifi.c_str(), savedWifiPass.c_str());
            }
        }

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }

    bool lightSet = false;
    long lastUpload = 0;
    long lastReconnectAttempt = 0;

    void loop()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            connect();
            return;
        }
        unsigned long now = millis();
        if (!mqttClient.connected())
        {
            if (now - lastReconnectAttempt > 5000)
            {
                Serial.println("MQTT attempting reconnect");
                lastReconnectAttempt = now;
                if (mqttReconnect())
                {
                    lastReconnectAttempt = 0;
                }
            }
        }
        else
        {
            mqttClient.loop();

            if (now - lastUpload > 60000)
            {
                lastUpload = now;
                uploadData();
            }
            else
            {
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
    }

    bool mqttReconnect()
    {
        if (mqttClient.connect(authKey, authKey, ""))
        {
            mqttClient.subscribe("v1/devices/me/attributes");
            mqttClient.subscribe("v1/devices/me/attributes/response/+");
            mqttClient.subscribe("v1/devices/me/rpc/request/+");

            String body = "{\"sharedKeys\":\"light,brightness\"}";
            mqttClient.publish("v1/devices/me/attributes/request/1", body.c_str());

            body = "{\"fw_version\":\"";
            body += FW_VERSION;
            body += "\"}";
            mqttClient.publish("v1/devices/me/attributes", body.c_str());
        }
        else
        {
            Serial.print("MQTT failed, rc=");
            Serial.println(mqttClient.state());
        }
        return mqttClient.connected();
    }

    void processAttrs(JsonObject doc)
    {
        if (doc.containsKey("light"))
        {
            lightSet = true;
            if (doc["light"] == "standard")
            {
                Leds::setAnimation(Leds::STANDARD);
            }
            else if (doc["light"] == "off")
            {
                Leds::setAnimation(Leds::OFF);
            }
            else if (doc["light"] == "lamp")
            {
                Leds::setAnimation(Leds::LAMP);
            }
        }
        if (doc.containsKey("brightness"))
        {
            Leds::setBrightness(min(255, max(0, doc["brightness"].as<int>())));
        }
    }

    void mqttCallback(char *topic, byte *payload, unsigned int length)
    {
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.println("] ");
        char str[length + 1];
        memcpy(str, payload, length);
        str[length] = 0;
        Serial.println(str);
        if (strcmp(topic, "v1/devices/me/attributes") == 0)
        {
            DynamicJsonDocument doc(JSON_OBJECT_SIZE(10));
            DeserializationError err = deserializeJson(doc, str);
            if (err != DeserializationError::Ok)
            {
                Serial.print("Deserialization error ");
                Serial.println(err.c_str());
                return;
            }
            processAttrs(doc.as<JsonObject>());
            doc.clear();
        }
        else if (strcmp(topic, "v1/devices/me/attributes/response/1") == 0)
        {
            DynamicJsonDocument doc(JSON_OBJECT_SIZE(10));
            DeserializationError err = deserializeJson(doc, str);
            if (err != DeserializationError::Ok)
            {
                Serial.print("Deserialization error ");
                Serial.println(err.c_str());
                return;
            }
            processAttrs(doc["shared"].as<JsonObject>());
            doc.clear();
        }
        else if (strstr(topic, "v1/devices/me/rpc/request/") != NULL)
        {
            int id = 0;
            sscanf(topic, "v1/devices/me/rpc/request/%d", &id);
            Serial.println(id);
            DynamicJsonDocument doc(JSON_OBJECT_SIZE(10));
            DeserializationError err = deserializeJson(doc, str);
            if (err != DeserializationError::Ok)
            {
                Serial.print("Deserialization error ");
                Serial.println(err.c_str());
                return;
            }
            if (doc["method"] == "reboot")
            {
                mqttClient.publish((String("v1/devices/me/rpc/response/") + id).c_str(), "{\"status\":\"OK\"}");
                ESP.restart();
            }
            else if (doc["method"] == "update")
            {
                mqttClient.publish((String("v1/devices/me/rpc/response/") + id).c_str(), "{\"status\":\"OK\"}");
                OTAUpdate::checkFWUpdates(doc["params"]["url"]);
            }
            else if (doc["method"] == "forgetwifi")
            {
                mqttClient.publish((String("v1/devices/me/rpc/response/") + id).c_str(), "{\"status\":\"OK\"}");
                preferences.putString("wifi_name", "");
            }
            else if (doc["method"] == "ccs_baseline")
            {
                mqttClient.publish((String("v1/devices/me/rpc/response/") + id).c_str(), "{\"status\":\"OK\"}");
#ifdef ENABLE_CCS
                Sensors::ccs_writeBaseline(doc["params"]["baseline"].as<uint16_t>());
#endif
            }
            else if (doc["method"] == "mhz_calibrate")
            {
                mqttClient.publish((String("v1/devices/me/rpc/response/") + id).c_str(), "{\"status\":\"OK\"}");
                Sensors::mhz_calibrate();
            }
            else
            {
                mqttClient.publish((String("v1/devices/me/rpc/response/") + id).c_str(), "{\"status\":\"Unknown command\"}");
            }
            doc.clear();
        }
    }

    void uploadData()
    {
        Serial.print("Free heap: ");
        Serial.println(ESP.getFreeHeap());
        Serial.print("Free PSRAM: ");
        Serial.println(ESP.getFreePsram());
        if (!Sensors::isWarmedUp())
        {
            return;
        }
        String body = "{";
        body += "\"co2\":";
        body += Sensors::readCO2();
#ifdef ENABLE_CCS
        body += ",\"eco2\":";
        body += Sensors::readECO2();
        body += ",\"tvoc\":";
        body += Sensors::readTVOC();
        body += ",\"baseline\":";
        body += Sensors::ccs_readBaseline();
#endif
        body += ",\"hum\":";
        body += Sensors::readHumidity();
        body += ",\"temp\":";
        body += Sensors::readTemperature();
        body += ",\"pres\":";
        body += Sensors::readPressure();
        body += "}";
        Serial.println(body);

        mqttClient.publish("v1/devices/me/telemetry", body.c_str());
    }
} // namespace Wireless