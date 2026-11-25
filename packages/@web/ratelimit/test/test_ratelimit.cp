// Unit tests for @web/ratelimit
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    RateLimitConfig,
    RateLimiter,
    registerRateLimit,
    createRateLimitPlugin,
    setTestClock,
    resetGlobal,
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

fn pingHandler(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"pong\":true}");
}

pub fn main() {
    // 1. Standalone RateLimiter unit tests
    let rl = RateLimiter.init(RateLimitConfig{
        max_requests: 2,
        window_secs: 60,
        key_header: "X-Forwarded-For",
    });

    const d1 = rl.check("10.0.0.1", 1000);
    assertTrue(d1.allowed && d1.remaining == 1 && d1.retry_after == 0, "check 1 allowed");

    const d2 = rl.check("10.0.0.1", 1010);
    assertTrue(d2.allowed && d2.remaining == 0 && d2.retry_after == 0, "check 2 allowed");

    const d3 = rl.check("10.0.0.1", 1020);
    assertTrue(!d3.allowed && d3.remaining == 0 && d3.retry_after == 40, "check 3 blocked with retry_after=40");

    // Different key is independent
    const d_other = rl.check("10.0.0.2", 1020);
    assertTrue(d_other.allowed && d_other.remaining == 1, "independent key allowed");

    // Window expiry resets counter
    const d4 = rl.check("10.0.0.1", 1060);
    assertTrue(d4.allowed && d4.remaining == 1, "window reset allows request");

    // 2. App integration via registerRateLimit + createPlugin
    resetGlobal();
    setTestClock(2000);

    let app = App.init();
    * = registerRateLimit(*app, RateLimitConfig{
        max_requests: 2,
        window_secs: 30,
        key_header: "X-Forwarded-For",
    });
    * = app.get("/ping", pingHandler);

    let req1 = Request.init(AppMethod.GET, "/ping");
    * = req1.withHeader("X-Forwarded-For", "203.0.113.5, 10.0.0.1");
    const r1 = app.dispatch(req1);
    assertTrue(r1.status == 200, "req 1 -> 200");
    assertTrue(strEql(r1.header("RateLimit-Limit") orelse "", "2"), "req 1 limit header");
    assertTrue(strEql(r1.header("RateLimit-Remaining") orelse "", "1"), "req 1 remaining=1");

    let req2 = Request.init(AppMethod.GET, "/ping");
    * = req2.withHeader("X-Forwarded-For", "203.0.113.5");
    const r2 = app.dispatch(req2);
    assertTrue(r2.status == 200, "req 2 -> 200");
    assertTrue(strEql(r2.header("RateLimit-Remaining") orelse "", "0"), "req 2 remaining=0");

    let req3 = Request.init(AppMethod.GET, "/ping");
    * = req3.withHeader("X-Forwarded-For", "203.0.113.5");
    const r3 = app.dispatch(req3);
    assertTrue(r3.status == 429, "req 3 -> 429");
    assertTrue(strEql(r3.header("Retry-After") orelse "", "30"), "req 3 Retry-After=30");
    assertTrue(strEql(r3.header("RateLimit-Remaining") orelse "", "0"), "req 3 remaining=0");

    // X-Real-IP fallback
    let req_real = Request.init(AppMethod.GET, "/ping");
    * = req_real.withHeader("X-Real-IP", "198.51.100.9");
    const r_real = app.dispatch(req_real);
    assertTrue(r_real.status == 200, "X-Real-IP fallback allowed");

    // Advance clock past window
    setTestClock(2030);
    let req4 = Request.init(AppMethod.GET, "/ping");
    * = req4.withHeader("X-Forwarded-For", "203.0.113.5");
    const r4 = app.dispatch(req4);
    assertTrue(r4.status == 200, "after window reset -> 200");

    // Verify createRateLimitPlugin directly
    let app2 = App.init();
    const plug = createRateLimitPlugin(RateLimitConfig{ max_requests: 1, window_secs: 10, key_header: "X-Forwarded-For" });
    * = app2.usePlugin(&plug);
    * = app2.get("/ping", pingHandler);
    assertTrue(app2.dispatch(Request.init(AppMethod.GET, "/ping")).status == 200, "global key 1st ok");
    assertTrue(app2.dispatch(Request.init(AppMethod.GET, "/ping")).status == 429, "global key 2nd 429");

    setTestClock(0);
    std.debug.print("PASS: @web/ratelimit all tests passed\n", .{});
}
