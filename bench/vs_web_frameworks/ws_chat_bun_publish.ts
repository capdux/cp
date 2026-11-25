// Bun.serve WebSocket Chat Broadcast benchmark (native pub/sub: server.publish)
// Usage: bun ws_chat_bun_publish.ts --port 18302

function parsePort(defaultPort: number): number {
  let port = Number(process.env.PORT || defaultPort);
  const argv = process.argv.slice(2);
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--port" && argv[i + 1]) {
      port = parseInt(argv[++i], 10);
    }
  }
  return port;
}

const port = parsePort(18302);
const TOPIC = "chat";

const server = Bun.serve({
  hostname: "127.0.0.1",
  port,
  fetch(req, srv) {
    if (srv.upgrade(req)) return undefined;
    return new Response("WebSocket upgrade required", { status: 400 });
  },
  websocket: {
    perMessageDeflate: false,
    open(ws) {
      ws.subscribe(TOPIC);
    },
    message(_ws, message) {
      server.publish(TOPIC, message);
    },
    close(ws) {
      ws.unsubscribe(TOPIC);
    },
  },
});

console.log(`[bun-ws-publish] listening on ws://127.0.0.1:${server.port}`);
