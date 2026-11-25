// Official @web/circuit-breaker Plugin for `cap:web`
// Envoy / Tower three-state circuit breaker (Closed -> Open -> HalfOpen -> Closed):
//   - Tracks consecutive 5xx upstream/handler failures
//   - Opens circuit at failure_threshold, short-circuiting with 503 + X-Circuit-State + Retry-After
//   - Transitions to HalfOpen after reset_timeout_secs to allow a probe request
//   - Zero heap allocations, zero external C dependencies

const std = @import("std");
import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";

pub enum CircuitState {
    Closed,
    Open,
    HalfOpen,

    pub fn asString(): string {
        match (_) {
            .Closed => return "closed",
            .Open => return "open",
            .HalfOpen => return "half_open",
        }
    }
}

pub struct CircuitBreakerConfig {
    failure_threshold: u32 = 5;
    reset_timeout_secs: u64 = 30;
    retry_after_secs: string = "30";
}

let g_cb_cfg: CircuitBreakerConfig = CircuitBreakerConfig{};
let g_cb_state: CircuitState = CircuitState.Closed;
let g_cb_failures: u32 = 0;
let g_cb_opened_at: u64 = 0;
let g_cb_test_clock: u64 = 0;

fn cbNowSec(): u64 {
    if (g_cb_test_clock > 0) return g_cb_test_clock;
    const cb_builtin = @import("builtin");
    if (comptime cb_builtin.os.tag == .linux) {
        let ts: std.os.linux.timespec = undefined;
        * = std.os.linux.clock_gettime(.MONOTONIC, &ts);
        return if (ts.sec > 0) @intCast(ts.sec) else 1;
    }
    let ts: std.c.timespec = undefined;
    * = std.c.clock_gettime(.MONOTONIC, &ts);
    return if (ts.sec > 0) @intCast(ts.sec) else 1;
}

pub fn cbState(): CircuitState {
    return g_cb_state;
}

pub fn cbFailureCount(): u32 {
    return g_cb_failures;
}

pub fn cbSetTestClock(now_sec: u64) {
    g_cb_test_clock = now_sec;
}

pub fn cbReset() {
    g_cb_state = CircuitState.Closed;
    g_cb_failures = 0;
    g_cb_opened_at = 0;
}

pub fn cbBeforeHandle(ctx: *Context): bool {
    if (g_cb_state == CircuitState.Open) {
        const now = cbNowSec();
        if (now >= g_cb_opened_at && (now - g_cb_opened_at) >= g_cb_cfg.reset_timeout_secs) {
            g_cb_state = CircuitState.HalfOpen;
            return true;
        }
        * = ctx.setDerived("cb_short_circuit", "1");
        let r = Response.json(503, "{\"error\":\"circuit_open\"}");
        * = r.setHeader("X-Circuit-State", "open");
        if (g_cb_cfg.retry_after_secs.len > 0) {
            * = r.setHeader("Retry-After", g_cb_cfg.retry_after_secs);
        }
        ctx.abort(r);
        return false;
    }
    return true;
}

pub fn cbAfterHandle(ctx: *Context, resp: *Response) {
    if (ctx.derived("cb_short_circuit") != null) {
        return;
    }
    if (resp.status >= 500) {
        g_cb_failures += 1;
        if (g_cb_state == CircuitState.HalfOpen || g_cb_failures >= g_cb_cfg.failure_threshold) {
            g_cb_state = CircuitState.Open;
            g_cb_opened_at = cbNowSec();
        }
    } else {
        g_cb_failures = 0;
        g_cb_state = CircuitState.Closed;
    }
    * = resp.setHeader("X-Circuit-State", g_cb_state.asString());
}

pub fn cbCreatePlugin(cfg: CircuitBreakerConfig): Plugin {
    g_cb_cfg = cfg;
    let p = Plugin.init("@web/circuit-breaker");
    * = p
        .withScope(PluginScope.Global)
        .onBeforeHandle(cbBeforeHandle)
        .onAfterHandle(cbAfterHandle);
    return p;
}

pub fn registerCircuitBreaker(app: *App, cfg: CircuitBreakerConfig): *App {
    const p = cbCreatePlugin(cfg);
    return app.usePlugin(&p);
}
