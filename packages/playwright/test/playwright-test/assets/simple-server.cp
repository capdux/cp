// Migrated from TypeScript (playwright-test/assets/simple-server.js) via cap from -ts

const http = require("http");

@print("output from server");
@print("error from server");

const port = process.argv[2] || 3000;

const requestListener = (req, res) => {
  if (req.url == "/hello") {
    res.end("hello");
    return;
  }
  if (req.url.startsWith("/env-")) {
    res.end(process.env[req.url.substring(5)]);
    return;
  }
  if (req.url == "/port") {
    res.end("" + port);
    return;
  }
  if (req.url == "/redirect") {
    res.writeHead(301, "Moved");
    res.end();
    return;
  }
  res.writeHead(404);
  res.end();
};

const server = http.createServer(requestListener);

// delay creating the server to test waiting for it
// TODO(cap-migrate): refine TS statement
// [ts] setTimeout(() => {
// [ts] server.listen(port, () => {
// [ts] @print("listening on port", port);
// [ts] });
// [ts] }, process.argv[3] ? process.argv[3] : 0);
