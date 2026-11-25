// Native Node.js HTTP Upgrade RFC 6455 WebSocket Chat Broadcast Server
// Usage: node ws_chat_node.mjs --port 18304

import http from "node:http";
import crypto from "node:crypto";

const WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

function parsePort(defaultPort) {
  let port = Number(process.env.PORT || defaultPort);
  const argv = process.argv.slice(2);
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === "--port" && argv[i + 1]) {
      port = parseInt(argv[++i], 10);
    }
  }
  return port;
}

function buildFrame(opcode, payload) {
  const len = payload.length;
  let hdrLen = 2;
  if (len >= 126 && len <= 65535) hdrLen = 4;
  else if (len > 65535) hdrLen = 10;

  const out = Buffer.allocUnsafe(hdrLen + len);
  out[0] = 0x80 | (opcode & 0x0f);
  if (len < 126) {
    out[1] = len;
  } else if (len <= 65535) {
    out[1] = 126;
    out.writeUInt16BE(len, 2);
  } else {
    out[1] = 127;
    out.writeBigUInt64BE(BigInt(len), 2);
  }
  payload.copy(out, hdrLen);
  return out;
}

const port = parsePort(18304);
const clients = new Set();

const server = http.createServer((_req, res) => {
  res.writeHead(400);
  res.end("WebSocket upgrade required");
});

server.on("upgrade", (req, socket) => {
  const key = req.headers["sec-websocket-key"];
  if (!key) {
    socket.destroy();
    return;
  }
  const accept = crypto
    .createHash("sha1")
    .update(key + WS_GUID)
    .digest("base64");

  socket.setNoDelay(true);
  socket.write(
    "HTTP/1.1 101 Switching Protocols\r\n" +
      "Upgrade: websocket\r\n" +
      "Connection: Upgrade\r\n" +
      `Sec-WebSocket-Accept: ${accept}\r\n\r\n`
  );

  clients.add(socket);
  let buf = Buffer.alloc(0);

  socket.on("data", (chunk) => {
    buf = buf.length === 0 ? chunk : Buffer.concat([buf, chunk]);
    let offset = 0;

    while (buf.length - offset >= 2) {
      const b0 = buf[offset];
      const b1 = buf[offset + 1];
      const opcode = b0 & 0x0f;
      const masked = (b1 & 0x80) !== 0;
      let payloadLen = b1 & 0x7f;
      let hdrLen = 2;

      if (payloadLen === 126) {
        if (buf.length - offset < 4) break;
        payloadLen = buf.readUInt16BE(offset + 2);
        hdrLen = 4;
      } else if (payloadLen === 127) {
        if (buf.length - offset < 10) break;
        payloadLen = Number(buf.readBigUInt64BE(offset + 2));
        hdrLen = 10;
      }

      if (masked) hdrLen += 4;
      const frameLen = hdrLen + payloadLen;
      if (buf.length - offset < frameLen) break;

      const payload = Buffer.allocUnsafe(payloadLen);
      if (masked) {
        const m0 = buf[offset + hdrLen - 4];
        const m1 = buf[offset + hdrLen - 3];
        const m2 = buf[offset + hdrLen - 2];
        const m3 = buf[offset + hdrLen - 1];
        const mask = [m0, m1, m2, m3];
        const pStart = offset + hdrLen;
        for (let i = 0; i < payloadLen; i++) {
          payload[i] = buf[pStart + i] ^ mask[i & 3];
        }
      } else {
        buf.copy(payload, 0, offset + hdrLen, offset + frameLen);
      }

      offset += frameLen;

      if (opcode === 0x1 || opcode === 0x2) {
        const frame = buildFrame(opcode, payload);
        for (const c of clients) {
          if (!c.destroyed && c.writable) {
            c.write(frame);
          }
        }
      } else if (opcode === 0x9) {
        if (!socket.destroyed && socket.writable) {
          socket.write(buildFrame(0x0a, payload));
        }
      } else if (opcode === 0x8) {
        clients.delete(socket);
        socket.end(buildFrame(0x08, payload));
        return;
      }
    }

    if (offset > 0) {
      buf = offset === buf.length ? Buffer.alloc(0) : buf.subarray(offset);
    }
  });

  const cleanup = () => {
    clients.delete(socket);
  };
  socket.on("close", cleanup);
  socket.on("error", () => {
    cleanup();
    socket.destroy();
  });
});

server.listen(port, "127.0.0.1", () => {
  console.log(`[node-ws-chat] listening on ws://127.0.0.1:${port} (pid=${process.pid})`);
});
