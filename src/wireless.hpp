#pragma once

#include "Preferences.h"

namespace Wireless
{
    void begin();
    void provision();
    void saveNetwork(String name, String pass);
    void connect();

    void checkFWUpdates();
    void checkFWUpdates(const String& url);

    void loop();
    bool mqttReconnect();
    void mqttCallback(char* topic, byte* payload, unsigned int length);
    void uploadData();
}