const ngrok = require("ngrok");
const fs = require("fs");
const http = require("http");
const axios = require("axios");

const DEVICE_ID = "05625980-c610-11eb-9b93-c7c640bc4881"; // "47fb2e10-0f40-11ec-b0b4-4109bdb75ac7";
const TOKEN =
  "Bearer eyJhbGciOiJIUzUxMiJ9.eyJzdWIiOiJ0b21hc0BsdWZ0aW8uY3oiLCJzY29wZXMiOlsiVEVOQU5UX0FETUlOIl0sInVzZXJJZCI6IjVlNzIyMDEwLTMyNjctMTFlYi1iY2ZlLTI3MGVlOTQxNGYxYSIsImZpcnN0TmFtZSI6IlRvbcOhxaEiLCJsYXN0TmFtZSI6Ik1hcnR5a8OhbiIsImVuYWJsZWQiOnRydWUsImlzUHVibGljIjpmYWxzZSwidGVuYW50SWQiOiI1MWI3NGU0MC0zMjY3LTExZWItYmNmZS0yNzBlZTk0MTRmMWEiLCJjdXN0b21lcklkIjoiMTM4MTQwMDAtMWRkMi0xMWIyLTgwODAtODA4MDgwODA4MDgwIiwiaXNzIjoidGhpbmdzYm9hcmQuaW8iLCJpYXQiOjE2Mjk3MzcxNTgsImV4cCI6MTYzNDU3NTU1OH0.E2LKw1o7jZStxgG9-kAj_TnhlmhIdB5RllALvezpPc7KBIc62Hf-mLvssc1YfkgnY3GX1xr0xVvUhJRnODtOHg";

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
    authtoken: "***REMOVED***",
  });
  console.log("Serving at " + url);

  axios
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
});
