#include "WiFi.h"
#include "DNSServer.h"
#include "HTTPClient.h"
#include "HTTPUpdate.h"

#include "wireless.hpp"
#include "sensors.hpp"
#include "config.hpp"
#include "web.hpp"
#include "leds.hpp"

namespace Wireless
{
    Preferences preferences;
    bool connecting = false;
    std::unique_ptr<DNSServer> dnsServer;
    TaskHandle_t TaskUploadData;

    void begin()
    {
        preferences.begin("airguard-wifi", false); 
        // preferences.putString("wifi_name", "ayy");
        connect();

        checkFWUpdates();
        Leds::setAnimation(Leds::STANDARD);

        // Set up upload task
        xTaskCreate(
            TaskUploadDataRun,
            "TaskUploadData",
            10000,
            NULL,
            0,
            &TaskUploadData);
    }

    void provision() 
    {
        Leds::setAnimation(Leds::SETUP);
        Serial.println("Begining provisioning");
        WiFi.disconnect();
        WiFi.softAP(WIFI_ID, NULL, 1, 0, 4);

        dnsServer.reset(new DNSServer());
        dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer->start(53 , "*", WiFi.softAPIP());
        Web::begin();

        for(;;) {
            dnsServer->processNextRequest();
            Web::handleClient();

            if(connecting) {
                connecting = false;
                dnsServer->stop();
                dnsServer.reset();
                Web::close();

                connect();

                break;
            }
        }
    }

    void saveNetwork(String name, String pass) {
        preferences.putString("wifi_name", name);
        preferences.putString("wifi_pass", pass);
        connecting = true;
    }

    void connect()
    {
        Leds::setAnimation(Leds::STARTUP);
        String savedWifi = preferences.getString("wifi_name", "");
        String savedWifiPass = preferences.getString("wifi_pass", "");
        if(savedWifi.length() == 0) {
            provision();
            return;
        }
        Serial.print("Connecting to saved WiFi ");
        Serial.println(savedWifi);
        WiFi.begin(savedWifi.c_str(), savedWifiPass.c_str());
        
        int connectionAttempts = 0;
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
            connectionAttempts++;
            if(connectionAttempts >= 60) {
                provision();
                return;
            } else if(connectionAttempts % 20 == 0) {
                WiFi.begin(savedWifi.c_str(), savedWifiPass.c_str());
            }
        }

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }

    void TaskUploadDataRun(void *parameter)
    {
        for (;;)
        {
            uploadData();
            vTaskDelay(60000 / portTICK_PERIOD_MS);
        }
    }

    void uploadData()
    {
        if (WiFi.status() != WL_CONNECTED) {
            connect();
            return;
        }

        if(!Sensors::isWarmedUp()) {
            Serial.println("Not warmed up yet");
            return;
        }

        Serial.println("Uploading results...");
        HTTPClient client;
        if (!client.begin(API_URL)) {
            Serial.println("connection failed");
            return;
        }

        try {
            client.addHeader("Content-Type", "application/json");
            String body = "{";
            body += "\"co2\":";
            body += Sensors::readCO2();
            body += ",\"hum\":";
            body += Sensors::readHumidity();
            body += ",\"temp\":";
            body += Sensors::readTemperature();
            body += ",\"pres\":";
            body += Sensors::readPressure();
            body += "}";
            Serial.println(body);
            int httpResponseCode = client.POST(body);
            Serial.println("HTTP response " + httpResponseCode);
        } catch(...) {
            Serial.println("Error occured while uploading");
        }
    }

    void checkFWUpdates() {
        WiFiClient client;
        t_httpUpdate_return ret = httpUpdate.update(client, FW_UPDATE_URL, FW_VERSION);
        switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            break;

        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;

        case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            break;
        }
    }
}