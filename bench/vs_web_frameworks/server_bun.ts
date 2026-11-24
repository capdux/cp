// Bun.serve HTTP + WebSocket benchmark server (1-core & N-core reusePort: true)
// Elysia official 3 HTTP scenarios + WebSocket /ws broadcast
// Usage: bun server_bun.ts --port 3000 --workers 1

function parseArgs() {
  let port = 3000;
  let workers = 1;
  let isWorker = false;
  const argv = process.argv.slice(2);
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--port" && argv[i + 1]) port = parseInt(argv[++i], 10);
    else if (argv[i] === "--workers" && argv[i + 1]) workers = parseInt(argv[++i], 10);
    else if (argv[i] === "--worker") isWorker = true;
  }
  return { port, workers, isWorker };
}

const { port, workers, isWorker } = parseArgs();

if (workers > 1 && !isWorker) {
  const procs: ReturnType<typeof Bun.spawn>[] = [];
  for (let i = 0; i < workers; i++) {
    procs.push(
      Bun.spawn([process.execPath, import.meta.path, "--port", String(port), "--workers", "1", "--worker"], {
        stdout: "inherit",
        stderr: "inherit",
      })
    );
  }
  const cleanup = () => {
    for (const p of procs) {
      try { p.kill(); } catch {}
    }
    process.exit(0);
  };
  process.on("SIGINT", cleanup);
  process.on("SIGTERM", cleanup);
  // Keep parent alive
  await Promise.all(procs.map((p) => p.exited));
} else {
  const server = Bun.serve({
    port,
    reusePort: true,
    async fetch(req, srv) {
      const url = req.url;
      // Fast path: extract pathname & query without constructing full URL object when simple
      const pathStart = url.indexOf("/", 8); // after http://host:port
      const qIdx = url.indexOf("?", pathStart);
      const pathname = qIdx === -1 ? url.slice(pathStart) : url.slice(pathStart, qIdx);

      // Scenario 1: GET / -> "Hi"
      if (req.method === "GET" && pathname === "/") {
        return new Response("Hi");
      }

      // WebSocket upgrade: /ws
      if (pathname === "/ws") {
        if (srv.upgrade(req)) return undefined;
        return new Response("WebSocket upgrade failed", { status: 400 });
      }

      // Scenario 2: GET /id/:id?name=bun -> "${id} ${name}" + header x-powered-by: benchmark
      if (req.method === "GET" && pathname.startsWith("/id/")) {
        const id = pathname.slice(4);
        let name = "";
        if (qIdx !== -1) {
          const qs = url.slice(qIdx + 1);
          const idx = qs.indexOf("name=");
          if (idx !== -1) {
            const amp = qs.indexOf("&", idx + 5);
            name = amp === -1 ? qs.slice(idx + 5) : qs.slice(idx + 5, amp);
          }
        }
        return new Response(id + " " + name, {
          headers: { "x-powered-by": "benchmark" },
        });
      }

      // Scenario 3: POST /json -> parse JSON & echo JSON
      if (req.method === "POST" && pathname === "/json") {
        const data = await req.json();
        return Response.json(data);
      }

      return new Response("Not Found", { status: 404 });
    },
    websocket: {
      open(ws) {
        ws.subscribe("room");
      },
      message(ws, message) {
        ws.publish("room", message);
        ws.send(message);
      },
      close(ws) {
        ws.unsubscribe("room");
      },
    },
  });

  if (!isWorker) {
    console.log(`[bun] listening on http://127.0.0.1:${server.port} (workers=${workers})`);
  }
}
