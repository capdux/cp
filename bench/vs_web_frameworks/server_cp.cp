// Real TCP Multi-Core SO_REUSEPORT + epoll Benchmark Server for `cap:web`
// Implements ElysiaJS official 3-route benchmark:
//   1. GET  /                -> "Hi"
//   2. GET  /id/:id?name=... -> "<id> <name>" + header `x-powered-by: benchmark`
//   3. POST /json            -> JSON body echo
// Env vars: PORT (default 3100), WORKERS (default 1)

import { App, AppMethod, Context, Response } from "../../lib/web/app.cp";
import { WebServer, ServerConfig } from "../../lib/web/server.cp";

extern fn getenv(name: [*:0]const u8): [*:0]const u8?;
extern fn atoi(s: [*:0]const u8): i32;

fn envInt(name: [*:0]const u8, default_val: i32): i32 {
    if (v := getenv(name)) {
        const n = atoi(v);
        if (n > 0) return n;
    }
    return default_val;
}

fn hPing(ctx: *Context): Response {
    * = ctx;
    return Response.text(200, "Hi");
}

fn hIdQuery(ctx: *Context): Response {
    * = ctx.setHeader("x-powered-by", "benchmark");
    const id = ctx.param("id") ?? "";
    const name = ctx.query("name") ?? "";
    if (id.len > 0 && name.len > 0) {
        return Response.textConcat3(200, id, " ", name);
    }
    return Response.text(400, "missing");
}

fn hJson(ctx: *Context): Response {
    return Response.json(200, ctx.body);
}

fn main() {
    const port: u16 = envInt("PORT", 3100) as u16;
    const workers: usize = envInt("WORKERS", 1) as usize;

    let app = App.init();
    * = app
        .get("/", hPing)
        .get("/id/:id", hIdQuery)
        .post("/json", hJson);

    let srv = WebServer.init(ServerConfig{
        port: port,
        workers: workers,
        backlog: 4096,
    });
    if (!srv.start(*app)) {
        @panic("server_cp failed to bind port");
    }
    @print("CP_SERVER_READY port={} workers={}", port, workers);
    srv.join();
}
