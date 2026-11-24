// Real TCP Multi-Core SO_REUSEPORT + epoll + 32B SIMD WebSocket Benchmark Server
// Env vars: PORT (default 3200), WORKERS (default 1)
import { WsServer, WsServerConfig } from "../../lib/web/wsServer.cp";

extern fn getenv(name: [*:0]const u8): [*:0]const u8?;
extern fn atoi(s: [*:0]const u8): i32;

fn envInt(name: [*:0]const u8, default_val: i32): i32 {
    if (v := getenv(name)) {
        const n = atoi(v);
        if (n > 0) return n;
    }
    return default_val;
}

fn main() {
    const port: u16 = envInt("PORT", 3200) as u16;
    const workers: usize = envInt("WORKERS", 1) as usize;

    let srv = WsServer.init(WsServerConfig{
        port: port,
        workers: workers,
        backlog: 4096,
    });
    if (!srv.start()) {
        @panic("ws_server_cp failed to bind port");
    }
    @print("CP_WS_SERVER_READY port={} workers={}", port, workers);
    srv.join();
}
