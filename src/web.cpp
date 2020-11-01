#include "WebServer.h"

#include "wireless.hpp"
#include "web.hpp"

namespace Web
{
    PROGMEM const char *htmlConnect = R"A(
<html>
<head>
<title>Air-Guard</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {
    text-align: center;
    font-family: "San Francisco", Helvetica, "Segoe UI", sans-serif;
    color: #2d3748;
}
#container {
    max-width: 500px;
    margin: 2rem auto;
}
h2 {
    font-weight: normal;
    color: #4a5568;
}
button {
    display: block;
    width: 100%;
    background-color: #E6E7EA;
    border: none;
    outline: 0;
    color: #2d3748;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 16px;
    padding: 14px 40px;
    margin: 4px 2px;
    cursor: pointer;
    border-radius: 3px;
    font-weight: bold;
}
button.primary {
    background-color: #031846;
    color: white;
}
input {
    display: block;
    width: 100%;
    background-color: white;
    border: 2px solid #E6E7EA;
    outline: 0;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 16px;
    padding: 14px;
    margin: 4px 2px;
    border-radius: 3px;
    text-align: left;
    -webkit-appearance: none;
}
#password-prompt {
    position: fixed;
    top: 20px;
    left: 0;
    right: 0;
    margin: auto;
    width: calc(100% - 60px);
    max-width: 500px;
    height: auto;
    z-index: 10;
    background: white;
    padding: 20px;
    border-radius: 5px;
    display: none;
    opacity: 0;
    transition: opacity 300ms ease-in-out;
}
#password-prompt-background {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    width: 100%;
    height: 100%;
    z-index: 1;
    background: #00000066;
    display: none;
    opacity: 0;
    transition: opacity 300ms ease-in-out;
}
</style>
</head>
<body>
<div id="container">
<h1>Welcome to Air-Guard</h1>
<h2>Select a network</h2>
<div id="network-list">
</div>
</div>
<div id="password-prompt-background" onclick="closePrompt()"></div>
<div id="password-prompt">
    <h1>Enter password</h1>
    <h2 id="password-prompt-text"></h2>
    <input type="password" placeholder="Password" />
    <button class="primary" id="password-prompt-connect">Connect</button>
    <button id="password-prompt-connect" onclick="closePrompt()">Close</button>
</div>

<script>
    var selectedNetwork = "";
    function finishConnection(ssid, password) {
        closePrompt();
        fetch("/api/connect?ssid=" + ssid + "&pass=" + password).then(data => {
            document.querySelector("h2").innerHTML = "Connecting...<br><br><small>When the light stops flashing, it's finished.</small>";
            document.querySelector("#network-list").innerHTML = "";
        }).catch(error => window.alert("A network error occurred"));
    }
    function showPrompt() {
        document.querySelector("#password-prompt").style.display = "block";
        document.querySelector("#password-prompt-background").style.display = "block";
        setTimeout(() => {
            document.querySelector("#password-prompt").style.opacity = 1;
            document.querySelector("#password-prompt-background").style.opacity = 1;
        }, 100);
        document.querySelector("#password-prompt-text").innerHTML = "For network " + selectedNetwork;
        document.querySelector("#password-prompt-connect").onclick = () => {
            var password = document.querySelector("#password-prompt input").value;
            finishConnection(selectedNetwork, password);
        }
    }
    function closePrompt() {
        document.querySelector("#password-prompt").style.opacity = 0;
        document.querySelector("#password-prompt-background").style.opacity = 0;
        setTimeout(() => {
            document.querySelector("#password-prompt").style.display = "none";
            document.querySelector("#password-prompt-background").style.display = "none";
        }, 300)
    }
    function connect(el) {
        if(el.dataset.secured == "true") {
            selectedNetwork = el.dataset.ssid;
            showPrompt();
        } else {
            finishConnection(el.dataset.ssid, "");
        }
    }
    fetch("/api/scan").then(res => res.json()).then(data => {
        var data = [{ssid:"martykanovi2", secured: true}];
        document.querySelector("#network-list").innerHTML = data.map(it => `<button class="connect" data-ssid="${it.ssid}" data-secured="${it.secured}">${it.ssid}</button>`).join("");
        document.querySelectorAll(".connect").forEach(el => (el.onclick = () => connect(el)));
    }).catch(error => window.alert("A network error occurred"))
</script>
</body>
</html>
    )A";
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