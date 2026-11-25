// Unit & integration tests for @web/circuit-breaker
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    CircuitState,
    CircuitBreakerConfig,
    cbState,
    cbFailureCount,
    cbSetTestClock,
    cbReset,
    registerCircuitBreaker,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn testStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

let g_upstream_fail: bool = false;
let g_handler_calls: u32 = 0;

fn upstreamHandler(ctx: *Context): Response {
    * = ctx;
    g_handler_calls += 1;
    if (g_upstream_fail) {
        return Response.json(502, "{\"error\":\"bad_gateway\"}");
    }
    return Response.json(200, "{\"ok\":true}");
}

pub fn main() {
    cbReset();
    cbSetTestClock(1000);
    g_upstream_fail = false;
    g_handler_calls = 0;

    let app = App.init();
    * = registerCircuitBreaker(*app, CircuitBreakerConfig{
        failure_threshold: 3,
        reset_timeout_secs: 30,
        retry_after_secs: "30",
    });
    * = app.get("/api", upstreamHandler);

    // 1. Healthy Closed state
    const r_ok = app.dispatch(Request.init(AppMethod.GET, "/api"));
    assertTrue(r_ok.status == 200, "healthy request -> 200");
    assertTrue(cbState() == CircuitState.Closed, "state is Closed");
    assertTrue(testStrEql(r_ok.header("X-Circuit-State") orelse "", "closed"), "header X-Circuit-State=closed");

    // 2. Accumulate failures up to threshold (3)
    g_upstream_fail = true;
    const r_f1 = app.dispatch(Request.init(AppMethod.GET, "/api"));
    assertTrue(r_f1.status == 502 && cbState() == CircuitState.Closed && cbFailureCount() == 1, "fail 1 -> still Closed");

    const r_f2 = app.dispatch(Request.init(AppMethod.GET, "/api"));
    assertTrue(r_f2.status == 502 && cbState() == CircuitState.Closed && cbFailureCount() == 2, "fail 2 -> still Closed");

    const r_f3 = app.dispatch(Request.init(AppMethod.GET, "/api"));
    assertTrue(r_f3.status == 502 && cbState() == CircuitState.Open, "fail 3 -> trips Open");
    assertTrue(testStrEql(r_f3.header("X-Circuit-State") orelse "", "open"), "tripping response has X-Circuit-State=open");

    // 3. Open state short-circuits without calling handler
    const calls_before = g_handler_calls;
    cbSetTestClock(1010);
    const r_short = app.dispatch(Request.init(AppMethod.GET, "/api"));
    assertTrue(r_short.status == 503, "open circuit -> 503");
    assertTrue(g_handler_calls == calls_before, "short-circuit did not invoke handler");
    assertTrue(testStrEql(r_short.header("X-Circuit-State") orelse "", "open"), "503 has X-Circuit-State=open");
    assertTrue(testStrEql(r_short.header("Retry-After") orelse "", "30"), "503 has Retry-After=30");

    // 4. Cooldown expires -> HalfOpen probe fails -> re-opens immediately
    cbSetTestClock(1030);
    const r_probe_fail = app.dispatch(Request.init(AppMethod.GET, "/api"));
    assertTrue(r_probe_fail.status == 502, "half-open probe invoked handler");
    assertTrue(g_handler_calls == calls_before + 1, "probe incremented handler calls");
    assertTrue(cbState() == CircuitState.Open, "failed half-open probe re-opens circuit");

    // Still open at 1050 (opened at 1030, timeout 30 -> 1060)
    cbSetTestClock(1050);
    const r_short2 = app.dispatch(Request.init(AppMethod.GET, "/api"));
    assertTrue(r_short2.status == 503, "still open before new cooldown expires");

    // 5. Cooldown expires at 1060 -> HalfOpen probe succeeds -> resets to Closed
    g_upstream_fail = false;
    cbSetTestClock(1060);
    const r_recover = app.dispatch(Request.init(AppMethod.GET, "/api"));
    assertTrue(r_recover.status == 200, "half-open probe succeeds -> 200");
    assertTrue(cbState() == CircuitState.Closed, "circuit recovered to Closed");
    assertTrue(cbFailureCount() == 0, "failure count reset to 0");
    assertTrue(testStrEql(r_recover.header("X-Circuit-State") orelse "", "closed"), "recovered header=closed");

    cbSetTestClock(0);
    cbReset();
    std.debug.print("PASS: @web/circuit-breaker all tests passed\n", .{});
}
