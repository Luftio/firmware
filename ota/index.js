const ngrok = require("ngrok");
const fs = require("fs");
const http = require("http");
const axios = require("axios");
require("dotenv");

if (!process.argv[2]) {
  console.log("You must add the device label as an argument");
  return;
}

const DEVICE_LABELS = [process.argv[2]];
console.log(DEVICE_LABELS);
const TOKEN = process.env.token;
const server = http.createServer((req, res) => {
  console.log("Received request");
  console.log(req.headers);
  fs.readFile(
    __dirname + "/../.pio/build/esp32dev/firmware.bin",
    function (err, data) {
      if (err) {
        res.writeHead(404);
        res.end(JSON.stringify(err));
        return;
      }
      res.writeHead(200, "OK", {
        "Content-Length": data.length,
      });
      res.end(data);
    }
  );
});

server.listen(8080, async () => {
  await ngrok.disconnect();
  const url = await ngrok.connect({
    bind_tls: false,
    proto: "http",
    addr: 8080,
    authtoken: process.env.NGROK_TOKEN,
  });
  console.log("Serving at " + url);

  const response = await axios
    .get("https://app.luftio.com/api/tenant/devices?page=0&pageSize=100", {
      headers: {
        "X-Authorization": TOKEN,
      },
    })
    .catch((error) => {
      console.log(error.response.data);
    });
  const deviceIds = response.data.data
    .filter((it) => DEVICE_LABELS.includes(it.label))
    .map((it) => it.id.id);
  console.log(deviceIds);

  for (const DEVICE_ID of deviceIds) {
    await axios
      .post(
        "https://app.luftio.com/api/plugins/rpc/twoway/" + DEVICE_ID,
        {
          method: "update",
          params: {
            url: url + "/dl",
          },
          timeout: 500,
        },
        {
          headers: {
            "X-Authorization": TOKEN,
          },
        }
      )
      .then((response) => {
        console.log(response.data);
      })
      .catch((error) => {
        console.log(error.response.data);
      });
  }
});
