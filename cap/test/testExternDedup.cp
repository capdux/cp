import { SERVER_MAX_WORKERS, ServerConfig } from "../../lib/web/server.cp";
import { WS_MAX_WORKERS, WsServerConfig, computeWebSocketAccept } from "../../lib/web/wsServer.cp";

// Duplicate private top-level consts & extern fns matching both imported modules
const AF_INET: i32 = 2;
const SOCK_STREAM: i32 = 1;

extern fn socket(domain: i32, sock_type: i32, protocol: i32): i32;
extern fn read(fd: i32, buf: *u8, count: usize): isize;
extern fn close(fd: i32): i32;

@c {
    fn write(fd: i32, buf: [*]const u8, count: usize): isize;
}

fn main() {
    let http_cfg = ServerConfig{ port: 8080 };
    let ws_cfg = WsServerConfig{ port: 8081 };
    let accept = computeWebSocketAccept("dGhlIHNhbXBsZSBub25jZQ==");

    let ok = SERVER_MAX_WORKERS == 64 && WS_MAX_WORKERS == 64 && AF_INET == 2 && SOCK_STREAM == 1 && http_cfg.port == 8080 && ws_cfg.port == 8081 && accept[0] == 115;
    if (!ok) {
        * = close(-1);
    }
}
