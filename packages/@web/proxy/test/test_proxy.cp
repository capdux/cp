// Unit tests for @web/proxy
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    ProxyConfig,
    ProxyRequestPlan,
    ProxyTransportFn,
    buildProxyTargetPath,
    buildProxyRequestPlan,
    formatUpstreamHttp1Request,
    registerProxy,
    registerProxyWithTransport,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn strContains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (strEql(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

let g_test_last_req_buf: [1024]u8 = [0; 1024];
let g_test_last_req_len: usize = 0;

fn mockUpstreamTransport(cfg: *const ProxyConfig, ctx: *Context, target_path: string): Response {
    const client_ip = ctx.header("X-Real-IP") orelse "10.0.0.5";
    const req_len = formatUpstreamHttp1Request(
        ctx.method,
        target_path,
        ctx.body,
        cfg.*,
        client_ip,
        g_test_last_req_buf[0..],
    ) orelse 0;
    g_test_last_req_len = req_len;
    let resp = Response.text(200, target_path);
    * = resp.setHeader("X-Proxied-By", "cp-proxy");
    return resp;
}

fn localHealthHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"local\":true}");
}

pub fn main() {
    const cfg = ProxyConfig{
        prefix: "/api",
        upstream_host: "backend.internal",
        upstream_port: 9090,
        strip_prefix: true,
        add_forwarded_headers: true,
    };

    // 1. Safe prefix stripping + query preservation
    let out_path: [256]u8 = [0; 256];
    const len1 = buildProxyTargetPath("/api/v1/users", "page=2&limit=10", cfg, out_path[0..]) orelse 0;
    assertTrue(strEql(out_path[0..len1], "/v1/users?page=2&limit=10"), "strip prefix & preserve query");

    const len2 = buildProxyTargetPath("/api", "", cfg, out_path[0..]) orelse 0;
    assertTrue(strEql(out_path[0..len2], "/"), "exact prefix maps to root /");

    // 2. Keep prefix when strip_prefix = false
    const cfg_keep = ProxyConfig{
        prefix: "/api",
        upstream_host: "backend.internal",
        upstream_port: 9090,
        strip_prefix: false,
        add_forwarded_headers: false,
    };
    const len3 = buildProxyTargetPath("/api/orders", "id=7", cfg_keep, out_path[0..]) orelse 0;
    assertTrue(strEql(out_path[0..len3], "/api/orders?id=7"), "keep prefix when strip_prefix=false");

    // 3. Path traversal rejection
    assertTrue(buildProxyTargetPath("/api/../etc/passwd", "", cfg, out_path[0..]) == null, "reject .. traversal");
    assertTrue(buildProxyTargetPath("/api/%2e%2e/secret", "", cfg, out_path[0..]) == null, "reject %2e%2e traversal");
    assertTrue(buildProxyTargetPath("/api/foo\\bar", "", cfg, out_path[0..]) == null, "reject backslash");
    assertTrue(buildProxyTargetPath("/other/path", "", cfg, out_path[0..]) == null, "reject non-matching prefix");

    // 4. buildProxyRequestPlan
    const plan = buildProxyRequestPlan("/api/items", "sort=asc", cfg, "192.168.1.9") orelse {
        assertTrue(false, "buildProxyRequestPlan should succeed");
        return;
    };
    assertTrue(strEql(plan.targetPathSlice(), "/items?sort=asc"), "plan target_path");
    assertTrue(strEql(plan.hostHeaderSlice(), "backend.internal:9090"), "plan host_header");
    assertTrue(strEql(plan.forwarded_for, "192.168.1.9"), "plan forwarded_for");

    // 5. formatUpstreamHttp1Request
    let http_buf: [1024]u8 = [0; 1024];
    const hlen = formatUpstreamHttp1Request(
        AppMethod.POST,
        "/v1/items?ok=1",
        "{\"name\":\"cp\"}",
        cfg,
        "203.0.113.7",
        http_buf[0..],
    ) orelse 0;
    assertTrue(hlen > 0, "formatUpstreamHttp1Request succeeds");
    const wire = http_buf[0..hlen];
    assertTrue(strContains(wire, "POST /v1/items?ok=1 HTTP/1.1\r\n"), "request line");
    assertTrue(strContains(wire, "Host: backend.internal:9090\r\n"), "Host header");
    assertTrue(strContains(wire, "X-Forwarded-For: 203.0.113.7\r\n"), "X-Forwarded-For header");
    assertTrue(strContains(wire, "X-Forwarded-Proto: http\r\n"), "X-Forwarded-Proto header");
    assertTrue(strContains(wire, "Content-Length: 13\r\n"), "Content-Length header");
    assertTrue(strContains(wire, "\r\n\r\n{\"name\":\"cp\"}"), "HTTP body appended");

    // 6. App integration with pluggable transport
    let app = App.init();
    * = registerProxyWithTransport(*app, cfg, mockUpstreamTransport);
    * = app.get("/health", localHealthHandler);

    // 6a. Non-prefixed route passes through to local handler
    const r_local = app.dispatch(Request.init(AppMethod.GET, "/health"));
    assertTrue(r_local.status == 200, "local route passes through");
    assertTrue(strEql(r_local.body, "{\"local\":true}"), "local route body");

    // 6b. Prefixed route hits mockUpstreamTransport
    let req_px = Request.init(AppMethod.POST, "/api/v1/echo?x=1");
    * = req_px.withBody("{\"hi\":1}");
    * = req_px.withHeader("X-Real-IP", "10.20.30.40");
    const r_px = app.dispatch(req_px);
    assertTrue(r_px.status == 200, "proxied request returns 200");
    assertTrue(strEql(r_px.body, "/v1/echo?x=1"), "transport received stripped target_path");
    assertTrue(strEql(r_px.header("X-Proxied-By") orelse "", "cp-proxy"), "proxy response header preserved");
    assertTrue(
        strContains(g_test_last_req_buf[0..g_test_last_req_len], "X-Forwarded-For: 10.20.30.40\r\n"),
        "upstream HTTP/1.1 wire includes client IP",
    );

    // 6c. Traversal request under prefix -> 400
    const r_bad = app.dispatch(Request.init(AppMethod.GET, "/api/../secret"));
    assertTrue(r_bad.status == 400, "traversal request blocked with 400");

    // 7. Default registerProxy without transport -> 502
    let app_def = App.init();
    * = registerProxy(*app_def, cfg);
    const r_502 = app_def.dispatch(Request.init(AppMethod.GET, "/api/ping"));
    assertTrue(r_502.status == 502, "default proxy transport returns 502");

    std.debug.print("PASS: @web/proxy all tests passed!\n", .{});
}
