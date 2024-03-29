#include "WiFi.h"
#include "DNSServer.h"
#include "HTTPClient.h"
#include "PubSubClient.h"
#include "ArduinoJson.h"
#include "rom/rtc.h"
#include "Preferences.h"
#include "esp_wifi.h"

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
            unsigned long now = millis();
            if (WiFi.softAPgetStationNum() > 0)
            {
                idleTimer = now;
            }
            else if (now > 120000 && idleTimer < now - 120000)
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
    long reconnectAttempts = 0;

    void loop()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi not connected");
            connect();
            return;
        }
        unsigned long now = millis();
        if (!mqttClient.connected())
        {
            if (reconnectAttempts > 20) {
                Serial.println("No access to server");
                // No access to server
                provision();
            } 
            else if (now - lastReconnectAttempt > 5000)
            {
                Serial.println("MQTT attempting reconnect");
                lastReconnectAttempt = now;
                reconnectAttempts++;
                if (mqttReconnect())
                {
                    reconnectAttempts = 0;
                    lastReconnectAttempt = 0;
                }
            }
        }
        else
        {
            reconnectAttempts = 0;
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
            log("MQTT reconnected, uptime " + String((long)esp_timer_get_time() / 1000000, 10) + "s, reason " + rtc_get_reset_reason(0) + "/"+ esp_reset_reason() +" v" + FW_VERSION);
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
            else if (doc["light"] == "lamp")
            {
                Leds::setAnimation(Leds::LAMP);
            }
            else 
            {
                Leds::setAnimation(Leds::OFF);
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
            else if (doc["method"] == "co2_calibrate")
            {
                mqttClient.publish((String("v1/devices/me/rpc/response/") + id).c_str(), "{\"status\":\"OK\"}");
                Sensors::calibrateCO2();
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
        Serial.print(ESP.getFreeHeap());
        Serial.print(", PSRAM: ");
        Serial.println(ESP.getFreePsram());
        if (!Sensors::isWarmedUp())
            return;

        String body = Sensors::getValuesJSON();
        Serial.println(body);
        mqttClient.publish("v1/devices/me/telemetry", body.c_str());
    }

    void log(String data)
    {
        Serial.println(data);
        String body = "{\"log\":\"" + data + "\"}";
        mqttClient.publish("v1/devices/me/telemetry", body.c_str());
    }
} // namespace Wireless