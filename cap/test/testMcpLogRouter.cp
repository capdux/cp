// Origin: Pure-CP Stdlib Test Suite (lib/mcp/logRouter.cp) | Created: 2026-11-22
// Production Deep Verification Suite for MCP RFC 5424 Logging Level Router & Notification Ring:
//   1. All 8 RFC 5424 levels: strict rank ordering 0..7, case-insensitive parse, canonical format & invalid rejection
//   2. Global severity + longest-prefix logger overrides + delimiter boundary safety ("db" must not match "debugger")
//   3. Per-logger window rate limiting, logger isolation & window expiry reset
//   4. Monotonic clock rollback guard prevents rate-limit window bypass
//   5. 64-slot FIFO notification ring: newest-wins overwrite accounting & partial batch draining
//   6. Full boundary guards: invalid rate config, invalid level name, empty/overlong logger & message, 32-rule cap
//   7. Dynamic cap.mcp facade end-to-end verification

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCP_LOG_MAX_RULES,
    MCP_LOG_RING_CAPACITY,
    McpLogLevel,
    mcpLogLevelRank,
    parseMcpLogLevel,
    formatMcpLogLevel,
    McpLogEmitStatus,
    McpLogRateLimitConfig,
    McpLogNotification,
    McpLogRouter,
} from "cap:mcp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: RFC 5424 8-Level Ordering & Bidirectional Codec
// ============================================================================
test("mcpLogRouter: RFC 5424 8-level severity ordering & bidirectional parse/format codec", () => {
    const levels: [8]McpLogLevel = [
        McpLogLevel.Debug,
        McpLogLevel.Info,
        McpLogLevel.Notice,
        McpLogLevel.Warning,
        McpLogLevel.Error,
        McpLogLevel.Critical,
        McpLogLevel.Alert,
        McpLogLevel.Emergency,
    ];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        expect_eq(mcpLogLevelRank(levels[i]), i as u8);
        const wire = formatMcpLogLevel(levels[i]);
        if (parsed := parseMcpLogLevel(wire)) {
            expect_eq(parsed, levels[i]);
        } else {
            expect_true(false);
        }
    }

    // Case-insensitive parsing
    expect_eq(parseMcpLogLevel("DEBUG") ?? McpLogLevel.Emergency, McpLogLevel.Debug);
    expect_eq(parseMcpLogLevel("Warning") ?? McpLogLevel.Debug, McpLogLevel.Warning);
    expect_eq(parseMcpLogLevel("EMERGENCY") ?? McpLogLevel.Debug, McpLogLevel.Emergency);

    // Invalid level strings rejected
    expect_eq(parseMcpLogLevel("") == null, true);
    expect_eq(parseMcpLogLevel("trace") == null, true);
    expect_eq(parseMcpLogLevel("warn") == null, true);
    expect_eq(parseMcpLogLevel("fatal") == null, true);
});

// ============================================================================
// Test 2: Longest-Prefix Logger Routing & Delimiter Boundary Guard
// ============================================================================
test("mcpLogRouter: longest-prefix logger override & delimiter boundary protection", () => {
    let router = McpLogRouter.init(McpLogLevel.Warning, McpLogRateLimitConfig.unlimited()) ?? return;
    expect_true(router.setLoggerPrefixLevel("db", McpLogLevel.Info));
    expect_true(router.setLoggerPrefixLevel("db.pool", McpLogLevel.Debug));
    expect_true(router.setLoggerPrefixLevel("http", McpLogLevel.Error));

    // Longest prefix "db.pool" wins over "db"
    expect_eq(router.effectiveLevel("db.pool.conn"), McpLogLevel.Debug);
    expect_eq(router.effectiveLevel("db.query"), McpLogLevel.Info);
    expect_eq(router.effectiveLevel("http/v2"), McpLogLevel.Error);

    // "debugger" must NOT match prefix "db" (no delimiter boundary!) -> falls back to global warning
    expect_eq(router.effectiveLevel("debugger"), McpLogLevel.Warning);

    // Emit checks
    expect_eq(router.emitLog(McpLogLevel.Debug, "db.pool.conn", "acquired", 100), McpLogEmitStatus.enqueued);
    expect_eq(router.emitLog(McpLogLevel.Debug, "db.query", "select 1", 100), McpLogEmitStatus.filtered_level);
    expect_eq(router.emitLog(McpLogLevel.Info, "debugger", "step", 100), McpLogEmitStatus.filtered_level);
    expect_eq(router.emitLog(McpLogLevel.Warning, "http/v2", "slow", 100), McpLogEmitStatus.filtered_level);
    expect_eq(router.emitLog(McpLogLevel.Error, "http/v2", "502", 100), McpLogEmitStatus.enqueued);

    // Remove "db.pool" -> "db.pool.conn" falls back to "db" (info)
    expect_true(router.removeLoggerPrefixLevel("db.pool"));
    expect_eq(router.removeLoggerPrefixLevel("db.pool"), false);
    expect_eq(router.effectiveLevel("db.pool.conn"), McpLogLevel.Info);
    expect_eq(router.stats().filtered_drops, 3 as u64);
});

// ============================================================================
// Test 3: Per-Logger Rate Limiting & Logger Isolation
// ============================================================================
test("mcpLogRouter: per-logger rate limiting stops log storms while isolating other loggers", () => {
    const rl = McpLogRateLimitConfig{ max_per_window: 2, window_ms: 1_000 };
    let router = McpLogRouter.init(McpLogLevel.Debug, rl) ?? return;

    expect_eq(router.emitLog(McpLogLevel.Info, "worker.a", "m1", 1_000), McpLogEmitStatus.enqueued);
    expect_eq(router.emitLog(McpLogLevel.Info, "worker.a", "m2", 1_200), McpLogEmitStatus.enqueued);
    expect_eq(router.emitLog(McpLogLevel.Info, "worker.a", "m3", 1_500), McpLogEmitStatus.rate_limited);

    // Independent logger "worker.b" still admitted in same window
    expect_eq(router.emitLog(McpLogLevel.Info, "worker.b", "b1", 1_500), McpLogEmitStatus.enqueued);

    // After window expires (t=2000 >= 1000 + 1000), "worker.a" resets and succeeds
    expect_eq(router.emitLog(McpLogLevel.Info, "worker.a", "m4", 2_000), McpLogEmitStatus.enqueued);
    expect_eq(router.stats().rate_limited_drops, 1 as u64);
});

// ============================================================================
// Test 4: Monotonic Clock Rollback Guard
// ============================================================================
test("mcpLogRouter: monotonic clock rollback cannot bypass rate-limit window", () => {
    const rl = McpLogRateLimitConfig{ max_per_window: 1, window_ms: 5_000 };
    let router = McpLogRouter.init(McpLogLevel.Info, rl) ?? return;

    expect_eq(router.emitLog(McpLogLevel.Info, "auth", "ok1", 10_000), McpLogEmitStatus.enqueued);
    // Clock rolls backward to 2_000 -> clamped to 10_000, still inside window -> rate_limited!
    expect_eq(router.emitLog(McpLogLevel.Info, "auth", "ok2", 2_000), McpLogEmitStatus.rate_limited);
    // Advance to 15_000 -> window expires -> admitted!
    expect_eq(router.emitLog(McpLogLevel.Info, "auth", "ok3", 15_000), McpLogEmitStatus.enqueued);
});

// ============================================================================
// Test 5: 64-Slot FIFO Notification Ring Overwrite & Partial Drain
// ============================================================================
test("mcpLogRouter: 64-slot ring overwrites oldest on overflow and drains in exact FIFO order", () => {
    let router = McpLogRouter.init(McpLogLevel.Info, McpLogRateLimitConfig.unlimited()) ?? return;

    // Emit 70 logs into 64-slot ring -> oldest 6 overwritten (seq 1..6), seq 7..70 retained
    let i: u64 = 0;
    while (i < 70) : (i += 1) {
        expect_eq(router.emitLog(McpLogLevel.Info, "sys", "heartbeat", 1_000 + i), McpLogEmitStatus.enqueued);
    }

    const st = router.stats();
    expect_eq(st.queued_count, MCP_LOG_RING_CAPACITY);
    expect_eq(st.total_emitted, 70 as u64);
    expect_eq(st.ring_overwrites, 6 as u64);

    // Partial drain 10 notifications -> must be seq 7..16
    let batch10: [10]McpLogNotification = [McpLogNotification{}; 10];
    let batch10_s: []McpLogNotification = batch10[0..];
    const d1 = router.drainNotifications(*batch10_s);
    expect_eq(d1, 10 as usize);
    expect_eq(batch10[0].seq, 7 as u64);
    expect_eq(batch10[9].seq, 16 as u64);
    expect_eq(router.pendingCount(), 54 as usize);

    // Drain remaining 54 notifications -> seq 17..70
    let batch64: [64]McpLogNotification = [McpLogNotification{}; 64];
    let batch64_s: []McpLogNotification = batch64[0..];
    const d2 = router.drainNotifications(*batch64_s);
    expect_eq(d2, 54 as usize);
    expect_eq(batch64[0].seq, 17 as u64);
    expect_eq(batch64[53].seq, 70 as u64);
    expect_eq(router.pendingCount(), 0 as usize);
});

// ============================================================================
// Test 6: Full Boundary Guards (Invalid Config, Empty/Overlong, 32-Rule Cap)
// ============================================================================
test("mcpLogRouter: rejects invalid rate config, bad level names, overlong strings & full rules", () => {
    // Invalid rate config (max_per_window > 0 with window_ms == 0)
    expect_eq(
        McpLogRouter.init(McpLogLevel.Info, McpLogRateLimitConfig{ max_per_window: 5, window_ms: 0 }) == null,
        true,
    );

    let router = McpLogRouter.init(McpLogLevel.Info, McpLogRateLimitConfig.unlimited()) ?? return;
    expect_eq(router.setGlobalLevelByName("unknown_level"), false);
    expect_true(router.setGlobalLevelByName("NOTICE"));
    expect_eq(router.stats().global_level, McpLogLevel.Notice);

    // Empty logger/message & overlong logger (65 bytes) / message (257 bytes) rejected
    const long_logger: [65]u8 = ['a'; 65];
    const long_msg: [257]u8 = ['x'; 257];
    expect_eq(router.emitLog(McpLogLevel.Error, "", "msg", 100), McpLogEmitStatus.invalid_input);
    expect_eq(router.emitLog(McpLogLevel.Error, "log", "", 100), McpLogEmitStatus.invalid_input);
    expect_eq(router.emitLog(McpLogLevel.Error, long_logger[0..], "msg", 100), McpLogEmitStatus.invalid_input);
    expect_eq(router.emitLog(McpLogLevel.Error, "log", long_msg[0..], 100), McpLogEmitStatus.invalid_input);
    expect_eq(router.stats().invalid_drops, 4 as u64);

    // Empty/overlong prefix rejected
    expect_eq(router.setLoggerPrefixLevel("", McpLogLevel.Debug), false);
    expect_eq(router.setLoggerPrefixLevel(long_logger[0..], McpLogLevel.Debug), false);

    // Fill all 32 rule slots
    let k: usize = 0;
    while (k < MCP_LOG_MAX_RULES) : (k += 1) {
        const d1: u8 = ('0' as u8) + ((k / 10) as u8);
        const d2: u8 = ('0' as u8) + ((k % 10) as u8);
        let name: [4]u8 = ['r', '_', d1, d2];
        expect_true(router.setLoggerPrefixLevel(name[0..], McpLogLevel.Debug));
    }
    expect_eq(router.stats().rule_count, MCP_LOG_MAX_RULES);

    // 33rd new rule rejected, updating existing rule succeeds
    expect_eq(router.setLoggerPrefixLevel("overflow", McpLogLevel.Debug), false);
    expect_true(router.setLoggerPrefixLevel("r_00", McpLogLevel.Critical));
    expect_eq(router.effectiveLevel("r_00"), McpLogLevel.Critical);
});

// ============================================================================
// Test 7: Dynamic cap.mcp Facade End-to-End Parity
// ============================================================================
test("mcpLogRouter: cap.mcp dynamic facade end-to-end emit and drain", () => {
    let router = cap.mcp.McpLogRouter.init(McpLogLevel.Info, McpLogRateLimitConfig.unlimited()) ?? return;
    router.setGlobalLevel(McpLogLevel.Notice);
    expect_eq(router.emitLog(McpLogLevel.Alert, "sec.audit", "key rotated", 9_000), McpLogEmitStatus.enqueued);

    let out: [2]McpLogNotification = [McpLogNotification{}; 2];
    let out_s: []McpLogNotification = out[0..];
    const n = router.drainNotifications(*out_s);
    expect_eq(n, 1 as usize);
    expect_eq(out[0].level, McpLogLevel.Alert);
    expect_eq(out[0].timestamp_ms, 9_000 as u64);
    expect_true(strEq(out[0].loggerSlice(), "sec.audit"));
    expect_true(strEq(out[0].messageSlice(), "key rotated"));
});
