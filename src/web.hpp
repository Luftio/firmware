#pragma once

namespace Web
{
    void begin();
    void handleClient();
    void close();

    void routeRoot();
    void routeScan();
    void routeConnect();
    void routeNotFound();
}