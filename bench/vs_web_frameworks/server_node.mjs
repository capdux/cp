// Node.js http.createServer benchmark server (1-core & N-core cluster mode)
// Elysia official 3 HTTP scenarios
// Usage: node server_node.mjs --port 3001 --workers 1

import http from "node:http";
import cluster from "node:cluster";

function parseArgs() {
  let port = 3001;
  let workers = 1;
  const argv = process.argv.slice(2);
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--port" && argv[i + 1]) port = parseInt(argv[++i], 10);
    else if (argv[i] === "--workers" && argv[i + 1]) workers = parseInt(argv[++i], 10);
  }
  return { port, workers };
}

const { port, workers } = parseArgs();

if (workers > 1 && cluster.isPrimary) {
  for (let i = 0; i < workers; i++) {
    cluster.fork();
  }
  const cleanup = () => {
    for (const id in cluster.workers) {
      cluster.workers[id]?.kill();
    }
    process.exit(0);
  };
  process.on("SIGINT", cleanup);
  process.on("SIGTERM", cleanup);
  console.log(`[node] cluster primary listening on http://127.0.0.1:${port} (workers=${workers})`);
} else {
  const server = http.createServer((req, res) => {
    const url = req.url || "/";
    const method = req.method || "GET";

    // Scenario 1: GET / -> "Hi"
    if (method === "GET" && url === "/") {
      res.writeHead(200, { "content-type": "text/plain", "content-length": "2" });
      res.end("Hi");
      return;
    }

    // Scenario 2: GET /id/:id?name=bun -> "${id} ${name}" + x-powered-by: benchmark
    if (method === "GET" && url.startsWith("/id/")) {
      const qIdx = url.indexOf("?");
      const id = qIdx === -1 ? url.slice(4) : url.slice(4, qIdx);
      let name = "";
      if (qIdx !== -1) {
        const qs = url.slice(qIdx + 1);
        const idx = qs.indexOf("name=");
        if (idx !== -1) {
          const amp = qs.indexOf("&", idx + 5);
          name = amp === -1 ? qs.slice(idx + 5) : qs.slice(idx + 5, amp);
        }
      }
      const body = id + " " + name;
      res.writeHead(200, {
        "content-type": "text/plain",
        "x-powered-by": "benchmark",
        "content-length": Buffer.byteLength(body),
      });
      res.end(body);
      return;
    }

    // Scenario 3: POST /json -> parse JSON & echo JSON
    if (method === "POST" && url === "/json") {
      let raw = "";
      req.setEncoding("utf8");
      req.on("data", (chunk) => {
        raw += chunk;
      });
      req.on("end", () => {
        try {
          const parsed = JSON.parse(raw);
          const out = JSON.stringify(parsed);
          res.writeHead(200, {
            "content-type": "application/json",
            "content-length": Buffer.byteLength(out),
          });
          res.end(out);
        } catch {
          res.writeHead(400, { "content-type": "text/plain" });
          res.end("Invalid JSON");
        }
      });
      return;
    }

    res.writeHead(404, { "content-type": "text/plain" });
    res.end("Not Found");
  });

  server.keepAliveTimeout = 60000;
  server.listen(port, "127.0.0.1", () => {
    if (workers === 1) {
      console.log(`[node] listening on http://127.0.0.1:${port} (workers=1)`);
    }
  });
}
