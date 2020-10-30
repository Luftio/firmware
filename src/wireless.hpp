#pragma once

#include "Preferences.h"

namespace Wireless
{
    void begin();
    void provision();
    void saveNetwork(String name, String pass);
    void connect();
    
    void TaskUploadDataRun(void *parameter);
    void uploadData();
    void checkFWUpdates();
}