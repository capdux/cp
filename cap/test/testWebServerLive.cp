// Live Real-Socket E2E Test for `cap:web` Multi-Core SO_REUSEPORT + epoll HTTP/1.1 Server
import { App, AppMethod, Context, Response } from "../../lib/web/app.cp";
import { WebServer, ServerConfig } from "../../lib/web/server.cp";

@c struct ClientSockAddrIn {
    sin_family: u16,
    sin_port: u16,
    sin_addr: u32,
    sin_zero: [8]u8,
}

extern fn connect(fd: i32, addr: *const ClientSockAddrIn, addrlen: u32): i32;

fn htons16(v: u16): u16 {
    return ((v & 0x00FF) << 8) | ((v >> 8) & 0x00FF);
}

fn containsStr(hay: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > hay.len) return false;
    let i: usize = 0;
    while (i + needle.len <= hay.len) : (i += 1) {
        let ok = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (hay[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

fn hPing(ctx: *Context): Response {
    * = ctx;
    return Response.text(200, "Hi");
}

fn hIdQuery(ctx: *Context): Response {
    const id = ctx.param("id") ?? "";
    const name = ctx.query("name") ?? "";
    * = ctx.setHeader("x-powered-by", "benchmark");
    if (id.len > 0 && name.len > 0) {
        return Response.text(200, "id_ok");
    }
    return Response.text(400, "bad");
}

fn hJsonEcho(ctx: *Context): Response {
    return Response.json(200, ctx.body);
}

fn main() {
    let app = App.init();
    * = app
        .get("/", hPing)
        .get("/id/:id", hIdQuery)
        .post("/json", hJsonEcho);

    const port: u16 = 18923;
    let srv = WebServer.init(ServerConfig{
        port: port,
        workers: 2,
        backlog: 128,
        ws_path: "/ws",
        shutdown_grace_ms: 50,
    });
    if (!srv.start(*app)) {
        @panic("Failed to start WebServer on port 18923");
    }

    // Connect with real TCP socket and send 3 Keep-Alive HTTP/1.1 requests on same connection
    const cfd = socket(2, 1, 0);
    if (cfd < 0) {
        srv.stop();
        @panic("client socket failed");
    }
    let addr = ClientSockAddrIn{
        sin_family: 2,
        sin_port: htons16(port),
        sin_addr: 0x0100007F, // 127.0.0.1 in network byte order (little endian host)
        sin_zero: [0; 8],
    };
    if (connect(cfd, *addr, 16) < 0) {
        * = close(cfd);
        srv.stop();
        @panic("client connect failed");
    }

    let rbuf: [2048]u8 = [0; 2048];

    // Request 1: GET /
    const req1 = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n";
    * = write(cfd, req1.ptr, req1.len);
    const n1 = read(cfd, &rbuf[0], 2048);
    if (n1 <= 0 || !containsStr(rbuf[0..(n1 as usize)], "200 OK") || !containsStr(rbuf[0..(n1 as usize)], "\r\n\r\nHi")) {
        * = close(cfd);
        srv.stop();
        @panic("Request 1 (GET /) failed");
    }

    // Request 2: GET /id/42?name=cp (same Keep-Alive connection!)
    const req2 = "GET /id/42?name=cp HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n";
    * = write(cfd, req2.ptr, req2.len);
    const n2 = read(cfd, &rbuf[0], 2048);
    if (n2 <= 0 || !containsStr(rbuf[0..(n2 as usize)], "x-powered-by: benchmark") || !containsStr(rbuf[0..(n2 as usize)], "\r\n\r\nid_ok")) {
        * = close(cfd);
        srv.stop();
        @panic("Request 2 (GET /id/42?name=cp) failed");
    }

    // Request 3: POST /json with body
    const req3 = "POST /json HTTP/1.1\r\nHost: localhost\r\nContent-Length: 17\r\nConnection: close\r\n\r\n{\"hello\":\"world\"}";
    * = write(cfd, req3.ptr, req3.len);
    const n3 = read(cfd, &rbuf[0], 2048);
    if (n3 <= 0 || !containsStr(rbuf[0..(n3 as usize)], "200 OK") || !containsStr(rbuf[0..(n3 as usize)], "{\"hello\":\"world\"}")) {
        * = close(cfd);
        srv.stop();
        @panic("Request 3 (POST /json) failed");
    }

    * = close(cfd);

    // Request 4: Same-port HTTP -> WebSocket Upgrade on /ws + Masked Echo
    const wsfd = socket(2, 1, 0);
    if (wsfd < 0 || connect(wsfd, *addr, 16) < 0) {
        srv.stop();
        @panic("Same-port WS connect failed");
    }
    const ws_req = "GET /ws HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
    * = write(wsfd, ws_req.ptr, ws_req.len);
    const n4 = read(wsfd, &rbuf[0], 2048);
    if (n4 <= 0 || !containsStr(rbuf[0..(n4 as usize)], "101 Switching Protocols") || !containsStr(rbuf[0..(n4 as usize)], "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=")) {
        * = close(wsfd);
        srv.stop();
        @panic("Request 4 (Same-port WS Upgrade 101) failed");
    }
    // Send masked "Hi" (0x81, 0x82, mask=0x11,0x22,0x33,0x44, 'H'^0x11=0x59, 'i'^0x22=0x4B)
    let ws_frame: [8]u8 = [0x81, 0x82, 0x11, 0x22, 0x33, 0x44, 0x59, 0x4B];
    * = write(wsfd, ws_frame[0..].ptr, 8);
    const n5 = read(wsfd, &rbuf[0], 2048);
    if (n5 != 4 || rbuf[0] != 0x81 || rbuf[1] != 2 || rbuf[2] != 'H' || rbuf[3] != 'i') {
        * = close(wsfd);
        srv.stop();
        @panic("Request 4 (Same-port WS Echo) failed");
    }
    * = close(wsfd);

    srv.stop();
    @print("PASS testWebServerLive (2-worker SO_REUSEPORT epoll + HTTP/1.1 Keep-Alive + Same-Port WS Upgrade 4/4)");
}
