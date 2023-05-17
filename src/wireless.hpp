#pragma once

namespace Wireless
{
    void begin();
    void provision();
    void saveNetwork(String name, String pass);
    void connect();

    void loop();
    bool mqttReconnect();
    void mqttCallback(char *topic, byte *payload, unsigned int length);
    void uploadData();
    void log(String data);
} // namespace Wireless