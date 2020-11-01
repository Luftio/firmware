#include "WiFi.h"
#include "HTTPClient.h"
#include "HTTPUpdate.h"

#include "otaupdate.hpp"
#include "config.hpp"

namespace OTAUpdate
{
    void checkFWUpdates(const String &url)
    {
        WiFiClient client;
        t_httpUpdate_return ret = httpUpdate.update(client, url, FW_VERSION);
        if (ret == HTTP_UPDATE_FAILED)
        {
            int errorCode = httpUpdate.getLastError();
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", errorCode, httpUpdate.getLastErrorString().c_str());
            // Check for redirect
            HTTPClient httpClient;
            if (!httpClient.begin(url))
            {
                Serial.println("HTTP connection failed");
                return;
            }
            try
            {
                const char *headers[] = {"Location"};
                size_t size = 1;
                httpClient.collectHeaders(headers, size);
                int newResponseCode = httpClient.GET();
                if (newResponseCode != 302 && newResponseCode != 301)
                {
                    Serial.println("HTTP response code " + newResponseCode);
                }
                else
                {
                    String newUrl = httpClient.header("Location");
                    Serial.println("New URL is " + newUrl);
                    if (newUrl != "")
                    {
                        checkFWUpdates(newUrl);
                    }
                }
            }
            catch (...)
            {
                Serial.println("Error occured while checking update URL");
            }
        }
        else if (ret == HTTP_UPDATE_NO_UPDATES)
        {
            Serial.println("HTTP_UPDATE_NO_UPDATES");
        }
        else if (ret == HTTP_UPDATE_OK)
        {
            Serial.println("HTTP_UPDATE_OK");
        }
    }
} // namespace OTAUpdate