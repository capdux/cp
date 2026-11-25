// CP WebSocket Chat Room Broadcast Server (Topic `publish` zero-copy Cork mode)
// Env vars: PORT (default 18301), WORKERS (default 1)
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
    const port: u16 = envInt("PORT", 18301) as u16;
    const workers: usize = envInt("WORKERS", 1) as usize;

    let srv = WsServer.init(WsServerConfig{
        port: port,
        workers: workers,
        backlog: 4096,
        pin_cpu: true,
        broadcast_mode: true,
    });
    if (!srv.start()) {
        @panic("ws_chat_cp failed to bind port");
    }
    @print("CP_WS_CHAT_READY port={} workers={}", port, workers);
    srv.join();
}
