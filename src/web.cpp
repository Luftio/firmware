#include "WebServer.h"

#include "wireless.hpp"
#include "web.hpp"

namespace Web
{
    PROGMEM const char *htmlConnect = R"(
<html>
<head>
<title>AirGuard</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {
    font-family: sans-serif;
}
#container {
    max-width: 500px;
    text-align: center;
    margin: 2rem auto;
}
h2 {
    color: #666;
}
button.connect {
    display: block;
    width: 100%;
    padding: 1rem;
}
</style>
</head>
<body>
<div id="container">
<h1>Air-Guard</h1>
<h2>Select a network</h2>

<div id="network-list">
</div>
</div>

<script>
    function connect(el) {
        var password = "";
        if(el.dataset.secured) {
            password = window.prompt("Enter password for " + el.dataset.ssid, "");
        }
        fetch("/api/connect?ssid=" + el.dataset.ssid + "&pass=" + password).then(data => {
            document.querySelector("h2").innerHTML = "Connecting...";
            document.querySelector("#network-list").innerHTML = "";
        }).catch(error => window.alert("A network error occurred"))
    }
    fetch("/api/scan").then(res => res.json()).then(data => {
        document.querySelector("#network-list").innerHTML = data.map(it => `<button class="connect" data-ssid="${it.ssid}" data-secured="${it.secured}">${it.ssid}</button>`).join("");
        document.querySelectorAll(".connect").forEach(el => (el.onclick = () => connect(el)));
    }).catch(error => window.alert("A network error occurred"))
</script>
</body>
</html>
    )";
    std::unique_ptr<WebServer> server;

    void begin()
    {
        server.reset(new WebServer(80));
        server->on("/", routeRoot);
        server->on("/api/scan", routeScan);
        server->on("/api/connect", routeConnect);
        server->onNotFound(routeNotFound);
        server->begin();
    }

    void handleClient()
    {
        server->handleClient();
    }

    void close()
    {
        server->stop();
        server.reset();
        server.release();
    }

    void routeRoot()
    {
        Serial.println("Route root");
        server->send(200, "text/html", htmlConnect);
    }

    void routeScan()
    {
        Serial.println("Route scan");
        String body = "[";
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; i++)
        {
            body += "{\"ssid\":\"";
            body += WiFi.SSID(i);
            body += "\",\"secured\":\"";
            body += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "false" : "true";
            body += "\"}";
        }
        body += "]";
        server->send(200, "application/json", body);
    }

    void routeConnect()
    {
        if (!server->hasArg("ssid"))
        {
            server->send(200, "text/html", "Invalid args");
            return;
        }
        server->send(200, "text/html", "Connecting");

        Serial.print("Connecting to ");
        Serial.print(server->arg("ssid"));
        Serial.print(" ");
        Serial.println(server->arg("pass"));
        Wireless::saveNetwork(server->arg("ssid"), server->arg("pass"));
    }

    void routeNotFound()
    {
        Serial.println("Route not found");
        server->sendHeader("Location", "http://192.168.4.1/");
        server->send(302, "text/html", " ");
    }
} // namespace Web