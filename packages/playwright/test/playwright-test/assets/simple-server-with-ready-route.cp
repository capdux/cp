// Migrated from TypeScript (playwright-test/assets/simple-server-with-ready-route.js) via cap from -ts

const http = require("http");

const port = process.argv[2] || 3000;

let ready = false;
setTimeout(() => ready = true, 750);

const requestListener = (req, res) => {
  if (req.url == "/ready") {
    if (ready) {
      res.writeHead(200);
      res.end("hello");
    } else {
      res.writeHead(404);
      res.end("not-ready");
    }
  } else {
    res.writeHead(404);
    res.end();
  }
};

const server = http.createServer(requestListener);
server.listen(port, () => {
  @print("listening on port", port);
});
