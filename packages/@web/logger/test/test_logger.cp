// Unit tests for @web/logger
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    LoggerConfig,
    formatAccessLogJson,
    formatTraceparent,
    registerLogger,
    resetState,
    getMetrics,
    lastAccessLog,
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

fn contains(haystack: string, needle: string): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) : (i += 1) {
        let j: usize = 0;
        let ok: bool = true;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

fn okHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"status\":\"ok\"}");
}

fn errHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(500, "{\"error\":\"boom\"}");
}

pub fn main() {
    // 1. formatAccessLogJson unit test
    let log_buf: [256]u8 = [0; 256];
    const n = formatAccessLogJson("GET", "/items?q=\"a\"", 200, "req-42", log_buf[0..]) orelse 0;
    assertTrue(n > 0, "formatAccessLogJson returns length");
    assertTrue(
        strEql(log_buf[0..n], "{\"method\":\"GET\",\"path\":\"/items?q=\\\"a\\\"\",\"status\":200,\"request_id\":\"req-42\"}"),
        "formatAccessLogJson output matches",
    );

    // 2. formatTraceparent unit test (W3C Trace Context: 55 chars)
    let tp_buf: [64]u8 = [0; 64];
    const tp_len = formatTraceparent(7, tp_buf[0..]) orelse 0;
    assertTrue(tp_len == 55, "traceparent length is 55");
    assertTrue(strEql(tp_buf[0..3], "00-"), "traceparent version 00-");
    assertTrue(strEql(tp_buf[52..55], "-01"), "traceparent sampled -01");

    // 3. App integration
    resetState();
    let app = App.init();
    * = registerLogger(*app, LoggerConfig{
        request_id_header: "X-Request-Id",
        metrics_path: "/metrics",
        enable_trace: true,
    });
    * = app.get("/ok", okHandler);
    * = app.get("/boom", errHandler);

    // 3a. Auto-generated X-Request-Id and traceparent
    const r1 = app.dispatch(Request.init(AppMethod.GET, "/ok"));
    assertTrue(r1.status == 200, "/ok returns 200");
    assertTrue(strEql(r1.header("X-Request-Id") orelse "", "req-1"), "auto X-Request-Id=req-1");
    const tp1 = r1.header("traceparent") orelse "";
    assertTrue(tp1.len == 55, "auto traceparent attached");
    assertTrue(contains(lastAccessLog(), "\"request_id\":\"req-1\""), "lastAccessLog records req-1");

    // 3b. Pass-through X-Request-Id and traceparent
    let req2 = Request.init(AppMethod.GET, "/ok");
    * = req2.withHeader("X-Request-Id", "ext-trace-999");
    * = req2.withHeader("traceparent", "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
    const r2 = app.dispatch(req2);
    assertTrue(strEql(r2.header("X-Request-Id") orelse "", "ext-trace-999"), "pass-through X-Request-Id");
    assertTrue(
        strEql(r2.header("traceparent") orelse "", "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"),
        "pass-through traceparent",
    );

    // 3c. Trigger 404 and 500 to verify metrics buckets
    const r404 = app.dispatch(Request.init(AppMethod.GET, "/missing"));
    assertTrue(r404.status == 404, "/missing returns 404");

    const r500 = app.dispatch(Request.init(AppMethod.GET, "/boom"));
    assertTrue(r500.status == 500, "/boom returns 500");

    const snap = getMetrics();
    assertTrue(snap.requests_total == 4, "requests_total == 4");
    assertTrue(snap.responses_2xx_total == 2, "responses_2xx_total == 2");
    assertTrue(snap.responses_4xx_total == 1, "responses_4xx_total == 1");
    assertTrue(snap.responses_5xx_total == 1, "responses_5xx_total == 1");

    // 3d. GET /metrics returns OpenMetrics exposition text
    const rm = app.dispatch(Request.init(AppMethod.GET, "/metrics"));
    assertTrue(rm.status == 200, "GET /metrics -> 200");
    assertTrue(contains(rm.body, "http_requests_total 4"), "metrics contains http_requests_total 4");
    assertTrue(contains(rm.body, "http_responses_2xx_total 2"), "metrics contains 2xx 2");
    assertTrue(contains(rm.body, "http_responses_4xx_total 1"), "metrics contains 4xx 1");
    assertTrue(contains(rm.body, "http_responses_5xx_total 1"), "metrics contains 5xx 1");

    std.debug.print("PASS: @web/logger all tests passed\n", .{});
}
