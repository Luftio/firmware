#include "WiFi.h"
#include "HTTPClient.h"
#include "HTTPUpdate.h"

#include "otaupdate.hpp"
#include "config.hpp"
#include "wireless.hpp"

namespace OTAUpdate
{
    void checkFWUpdates(const String &url)
    {
        Wireless::log("Installing update " + url);

        WiFiClient client;
        t_httpUpdate_return ret = httpUpdate.update(client, url, FW_VERSION);
        if (ret == HTTP_UPDATE_FAILED)
        {
            int errorCode = httpUpdate.getLastError();
            Wireless::log("Update HTTP connection failed " + httpUpdate.getLastErrorString());
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", errorCode, httpUpdate.getLastErrorString().c_str());
            // Check for redirect
            HTTPClient httpClient;
            if (!httpClient.begin(url))
            {
                Wireless::log("Update HTTP connection failed");
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
                    Wireless::log("Update HTTP response code " + newResponseCode);
                }
                else
                {
                    String newUrl = httpClient.header("Location");
                    Wireless::log("New URL is " + newUrl);
                    if (newUrl != "")
                    {
                        checkFWUpdates(newUrl);
                    }
                }
            }
            catch (...)
            {
                Wireless::log("Error occured while checking update URL");
            }
        }
        else if (ret == HTTP_UPDATE_NO_UPDATES)
        {
            Wireless::log("HTTP no updates");
        }
        else if (ret == HTTP_UPDATE_OK)
        {
            Wireless::log("HTTP update OK");
        }
    }
} // namespace OTAUpdate