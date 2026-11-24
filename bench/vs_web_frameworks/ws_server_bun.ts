// Bun.serve WebSocket Echo benchmark server (1-core & N-core reusePort: true)
// Usage: bun ws_server_bun.ts --port 18203 --workers 1

function parseArgs() {
  let port = 18203;
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
      Bun.spawn(
        [process.execPath, import.meta.path, "--port", String(port), "--workers", "1", "--worker"],
        { stdout: "inherit", stderr: "inherit" }
      )
    );
  }
  const cleanup = () => {
    for (const p of procs) {
      try {
        p.kill();
      } catch {}
    }
    process.exit(0);
  };
  process.on("SIGINT", cleanup);
  process.on("SIGTERM", cleanup);
  await Promise.all(procs.map((p) => p.exited));
} else {
  const server = Bun.serve({
    port,
    reusePort: true,
    fetch(req, srv) {
      if (srv.upgrade(req)) return undefined;
      return new Response("WebSocket upgrade required", { status: 400 });
    },
    websocket: {
      perMessageDeflate: false,
      message(ws, message) {
        ws.send(message);
      },
    },
  });

  if (!isWorker) {
    console.log(`[bun-ws] listening on ws://127.0.0.1:${server.port} (workers=${workers})`);
  }
}
