// Bun.serve WebSocket Chat Broadcast benchmark (manual Set loop: c.send)
// Usage: bun ws_chat_bun_send.ts --port 18303

import type { ServerWebSocket } from "bun";

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

const port = parsePort(18303);
const clients = new Set<ServerWebSocket<unknown>>();

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
      clients.add(ws);
    },
    message(_ws, message) {
      for (const c of clients) {
        c.send(message);
      }
    },
    close(ws) {
      clients.delete(ws);
    },
  },
});

console.log(`[bun-ws-send] listening on ws://127.0.0.1:${server.port}`);
