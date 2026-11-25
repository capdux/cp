// Official @web/pressure Plugin for `cap:web`
// Overload protection (body size limit 413, concurrency shedding 503 + Retry-After)
// and Kubernetes liveness/readiness probes (/healthz, /readyz) — fusing @fastify/under-pressure + tower-http::limit.

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub struct PressureConfig {
    max_body_bytes: usize = 1048576; // 1 MiB default
    max_inflight: u32 = 10000;
    retry_after_secs: string = "5";
    health_path: string = "/healthz";
    ready_path: string = "/readyz";
}

let g_press_cfg: PressureConfig = PressureConfig{};
let g_press_inflight: u32 = 0;
let g_press_ready: bool = true;
let g_press_shed_count: u64 = 0;

pub fn pressureSetReady(ready: bool) {
    g_press_ready = ready;
}

pub fn pressureSetInflightForTest(n: u32) {
    g_press_inflight = n;
}

pub fn pressureShedCount(): u64 {
    return g_press_shed_count;
}

pub fn pressureResetStats() {
    g_press_inflight = 0;
    g_press_ready = true;
    g_press_shed_count = 0;
}

fn pressStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

pub fn pressureOnRequest(ctx: *Context): bool {
    const cfg = g_press_cfg;
    // Always allow K8s probes through even under load
    if (pressStrEql(ctx.path, cfg.health_path) || pressStrEql(ctx.path, cfg.ready_path)) {
        return true;
    }
    if (cfg.max_body_bytes > 0 && ctx.body.len > cfg.max_body_bytes) {
        ctx.abort(Response.json(413, "{\"error\":\"payload_too_large\"}"));
        return false;
    }
    if (cfg.max_inflight > 0 && g_press_inflight >= cfg.max_inflight) {
        g_press_shed_count += 1;
        let r = Response.json(503, "{\"error\":\"service_under_pressure\"}");
        if (cfg.retry_after_secs.len > 0) {
            * = r.setHeader("Retry-After", cfg.retry_after_secs);
        }
        ctx.abort(r);
        return false;
    }
    g_press_inflight += 1;
    return true;
}

pub fn pressureOnAfter(ctx: *Context, resp: *Response) {
    * = resp;
    const cfg = g_press_cfg;
    if (pressStrEql(ctx.path, cfg.health_path) || pressStrEql(ctx.path, cfg.ready_path)) {
        return;
    }
    if (g_press_inflight > 0) {
        g_press_inflight -= 1;
    }
}

fn handleHealthz(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"status\":\"alive\"}");
}

fn handleReadyz(ctx: *Context): Response {
    * = ctx;
    if (!g_press_ready) {
        let r = Response.json(503, "{\"status\":\"draining\"}");
        * = r.setHeader("Retry-After", g_press_cfg.retry_after_secs);
        return r;
    }
    return Response.json(200, "{\"status\":\"ready\"}");
}

pub fn createPressurePlugin(cfg: PressureConfig): Plugin {
    g_press_cfg = cfg;
    let p = Plugin.init("@web/pressure");
    * = p
        .withScope(PluginScope.Global)
        .onRequest(pressureOnRequest)
        .onAfterHandle(pressureOnAfter);
    if (cfg.health_path.len > 0) {
        * = p.route(AppMethod.GET, cfg.health_path, "Kubernetes Liveness Probe", handleHealthz);
    }
    if (cfg.ready_path.len > 0) {
        * = p.route(AppMethod.GET, cfg.ready_path, "Kubernetes Readiness Probe", handleReadyz);
    }
    return p;
}

pub fn registerPressure(app: *App, cfg: PressureConfig): *App {
    const p = createPressurePlugin(cfg);
    return app.usePlugin(&p);
}
