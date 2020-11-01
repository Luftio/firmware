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
        preferences.begin("airguard-wifi", false);
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
        Leds::setAnimation(Leds::SETUP);
        Serial.println("Begining provisioning");
        WiFi.disconnect();
        WiFi.softAP(apSSID, NULL, 1, 0, 4);

        dnsServer.reset(new DNSServer());
        dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer->start(53, "*", WiFi.softAPIP());
        Web::begin();

        for (;;)
        {
            dnsServer->processNextRequest();
            Web::handleClient();

            if (connecting)
            {
                connecting = false;
                dnsServer->stop();
                dnsServer.reset();
                Web::close();

                connect();

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

        int connectionAttempts = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
            connectionAttempts++;
            if (connectionAttempts >= 60)
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
            processAttrs(doc.as<JsonObject>());
        }
        else if (strcmp(topic, "v1/devices/me/attributes/response/1") == 0)
        {
            DynamicJsonDocument doc(JSON_OBJECT_SIZE(10));
            DeserializationError err = deserializeJson(doc, str);
            processAttrs(doc["shared"].as<JsonObject>());
        }
        else if (strstr(topic, "v1/devices/me/rpc/request/") != NULL)
        {
            int id = 0;
            sscanf(topic, "v1/devices/me/rpc/request/%d", &id);
            Serial.println(id);
            DynamicJsonDocument doc(JSON_OBJECT_SIZE(10));
            DeserializationError err = deserializeJson(doc, str);
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
            else if (doc["method"] == "setbaseline")
            {
                mqttClient.publish((String("v1/devices/me/rpc/response/") + id).c_str(), "{\"status\":\"OK\"}");
                Sensors::writeBaseline(doc["params"]["baseline"].as<uint16_t>());
            }
            else
            {
                mqttClient.publish((String("v1/devices/me/rpc/response/") + id).c_str(), "{\"status\":\"Unknown command\"}");
            }
        }
    }

    void uploadData()
    {
        if (!Sensors::isWarmedUp())
        {
            return;
        }
        String body = "{";
        body += "\"co2\":";
        body += Sensors::readCO2();
        body += ",\"tvoc\":";
        body += Sensors::readTVOC();
        body += ",\"hum\":";
        body += Sensors::readHumidity();
        body += ",\"temp\":";
        body += Sensors::readTemperature();
        body += ",\"pres\":";
        body += Sensors::readPressure();
        body += ",\"baseline\":";
        body += Sensors::readBaseline();
        body += "}";
        Serial.println(body);

        mqttClient.publish("v1/devices/me/telemetry", body.c_str());
    }
} // namespace Wireless