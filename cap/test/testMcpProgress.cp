// Origin: Pure-CP Stdlib Test Suite (lib/mcp/progress.cp) | Created: 2026-11-22
// Production Deep Verification Suite for MCP 2025-03-26 Progress Token & Cancellation Notification FSM:
//   1. Integer & string ProgressToken registration, strictly monotonic updates & percent() calculation
//   2. min_interval_ms emission throttling (throttled_drops) & final total-completion bypass
//   3. Race-safe cancellation: active cancel, early cancel before register, late cancel after complete & idempotency
//   4. Timeout expiration reaping (reapExpired for slots & tombstones) & purgeTerminal slot reuse
//   5. Zero-alloc JSON-RPC 2.0 notifications/progress encode/parse round-trip (int & string tokens + escaping)
//   6. Zero-alloc JSON-RPC 2.0 notifications/cancelled encode/parse round-trip & malformed input rejection
//   7. Full boundary guards (dup token/req, NaN/negative, 64-slot full pool) & cap.mcp facade

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCP_PROGRESS_MAX_SLOTS,
    McpProgressTokenKind,
    McpProgressState,
    McpProgressReportStatus,
    McpCancelOutcome,
    McpProgressToken,
    encodeProgressNotification,
    parseProgressNotification,
    encodeCancelledNotification,
    parseCancelledNotification,
    McpProgressHub,
} from "cap:mcp";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

// ============================================================================
// Test 1: Int & String ProgressToken Registration, Monotonic Updates & percent
// ============================================================================
test("mcpProgress: int & string tokens enforce monotonic updates and compute percent", () => {
    let hub = McpProgressHub.init(0, 10_000);
    const tok_int = McpProgressToken.fromInt(42);
    const tok_str = McpProgressToken.fromStr("scan-repo-1") ?? return;

    expect_eq(hub.register(tok_int, 101, 200.0, null, 0, 1_000) ?? McpProgressState.timed_out, McpProgressState.active);
    expect_eq(hub.register(tok_str, 102, null, null, 0, 1_000) ?? McpProgressState.timed_out, McpProgressState.active);

    expect_eq(hub.reportProgress(tok_int, 50.0, null, "quarter done", 1_100), McpProgressReportStatus.emitted);
    // Equal or decreasing progress rejected as non_monotonic
    expect_eq(hub.reportProgress(tok_int, 50.0, null, "same", 1_200), McpProgressReportStatus.non_monotonic);
    expect_eq(hub.reportProgress(tok_int, 40.0, null, "backwards", 1_200), McpProgressReportStatus.non_monotonic);
    expect_eq(hub.reportProgress(tok_int, 100.0, null, "halfway", 1_300), McpProgressReportStatus.emitted);

    if (slot := hub.getByToken(tok_int)) {
        expect_true(absF64((slot.percent() ?? 0.0) - 50.0) < 1.0e-6);
        expect_true(bytesEq(slot.messageSlice(), "halfway"));
    } else {
        expect_true(false);
    }

    // String token with dynamically supplied total
    expect_eq(hub.reportProgress(tok_str, 3.0, 10.0, "3/10 files", 1_400), McpProgressReportStatus.emitted);
    if (s2 := hub.getByRequestId(102)) {
        expect_true(absF64((s2.percent() ?? 0.0) - 30.0) < 1.0e-6);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 2: min_interval_ms Emission Throttling & Final Total Bypass
// ============================================================================
test("mcpProgress: min_interval_ms throttles rapid updates while letting final 100% bypass", () => {
    let hub = McpProgressHub.init(200, 10_000);
    const tok = McpProgressToken.fromInt(7);
    expect_true(hub.register(tok, 200, 100.0, null, 0, 1_000) != null);

    // First emit at t=1000 succeeds
    expect_eq(hub.reportProgress(tok, 10.0, null, "step 1", 1_000), McpProgressReportStatus.emitted);
    // At t=1050 (elapsed=50 < 200) -> throttled
    expect_eq(hub.reportProgress(tok, 20.0, null, "step 2", 1_050), McpProgressReportStatus.throttled);
    expect_eq(hub.reportProgress(tok, 30.0, null, "step 3", 1_150), McpProgressReportStatus.throttled);
    // At t=1200 (elapsed=200 >= 200) -> emitted
    expect_eq(hub.reportProgress(tok, 40.0, null, "step 4", 1_200), McpProgressReportStatus.emitted);
    // At t=1210 (elapsed=10 < 200), reaching total=100.0 bypasses throttle!
    expect_eq(hub.reportProgress(tok, 100.0, null, "done", 1_210), McpProgressReportStatus.emitted);

    const st = hub.stats();
    expect_eq(st.throttled_drops, 2);
    expect_eq(st.total_emitted, 3);
});

// ============================================================================
// Test 3: Race-Safe Cancellation (Active, Early Before Register, Late After Complete)
// ============================================================================
test("mcpProgress: handles active cancel, early cancel before register & late cancel after complete", () => {
    let hub = McpProgressHub.init(0, 10_000);

    // 3A. Cancel active request
    const tok1 = McpProgressToken.fromInt(1);
    expect_eq(hub.register(tok1, 301, 10.0, null, 0, 1_000) ?? McpProgressState.timed_out, McpProgressState.active);
    expect_eq(hub.cancelByRequestId(301, "user aborted", 1_100), McpCancelOutcome.cancelled_active);
    expect_eq(hub.cancelByRequestId(301, "duplicate", 1_200), McpCancelOutcome.already_cancelled);
    expect_true(hub.isCancelled(301));
    expect_eq(hub.reportProgress(tok1, 5.0, null, "late", 1_250), McpProgressReportStatus.already_terminal);
    if (s1 := hub.getByRequestId(301)) {
        expect_true(bytesEq(s1.cancelReasonSlice(), "user aborted"));
    } else {
        expect_true(false);
    }

    // 3B. Early cancel arrives BEFORE register (race condition!)
    expect_eq(hub.cancelByRequestId(302, "client disconnected early", 2_000), McpCancelOutcome.recorded_early_cancel);
    expect_eq(hub.cancelByRequestId(302, "dup early", 2_050), McpCancelOutcome.already_cancelled);
    expect_true(hub.isCancelled(302));

    const tok2 = McpProgressToken.fromStr("late-reg") ?? return;
    const reg_state = hub.register(tok2, 302, 100.0, null, 0, 2_100) ?? McpProgressState.active;
    expect_eq(reg_state, McpProgressState.cancelled);
    expect_eq(hub.stats().early_cancel_hits, 1);
    if (s2 := hub.getByRequestId(302)) {
        expect_true(bytesEq(s2.cancelReasonSlice(), "client disconnected early"));
    } else {
        expect_true(false);
    }

    // 3C. Late cancel arrives AFTER completion -> idempotent already_completed
    const tok3 = McpProgressToken.fromInt(3);
    expect_true(hub.register(tok3, 303, 10.0, null, 0, 3_000) != null);
    expect_true(hub.complete(303));
    expect_eq(hub.cancelByRequestId(303, "too late", 3_100), McpCancelOutcome.already_completed);
    expect_true(!hub.isCancelled(303));
});

// ============================================================================
// Test 4: Timeout Reaping & Terminal Slot Purging
// ============================================================================
test("mcpProgress: reapExpired marks timed-out slots and purgeTerminal reclaims capacity", () => {
    let hub = McpProgressHub.init(0, 500);
    const t1 = McpProgressToken.fromInt(11);
    const t2 = McpProgressToken.fromInt(12);
    expect_true(hub.register(t1, 401, 10.0, null, 1_000, 1_000) != null); // deadline 2000
    expect_true(hub.register(t2, 402, 10.0, null, 5_000, 1_000) != null); // deadline 6000

    // Record an early cancel tombstone with TTL 500 (expires at 1500)
    expect_eq(hub.cancelByRequestId(499, "transient", 1_000), McpCancelOutcome.recorded_early_cancel);

    // At t=2500: t1 times out (1), tombstone 499 expires, t2 stays active
    const reaped = hub.reapExpired(2_500);
    expect_eq(reaped, 1);
    expect_true(!hub.isCancelled(499));
    expect_eq(hub.reportProgress(t1, 1.0, null, "late", 2_600), McpProgressReportStatus.already_terminal);
    expect_eq(hub.reportProgress(t2, 1.0, null, "alive", 2_600), McpProgressReportStatus.emitted);

    const st1 = hub.stats();
    expect_eq(st1.active_count, 1);
    expect_eq(st1.timed_out_count, 1);

    // Purge terminal reclaims t1 only
    const purged = hub.purgeTerminal();
    expect_eq(purged, 1);
    expect_true(hub.getByToken(t1) == null);
    expect_true(hub.getByToken(t2) != null);
});

// ============================================================================
// Test 5: Zero-Alloc JSON-RPC 2.0 notifications/progress Round-Trip
// ============================================================================
test("mcpProgress: encodeProgressNotification and parseProgressNotification round-trip cleanly", () => {
    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];

    // String token with escaped message and fractional progress
    const tok_str = McpProgressToken.fromStr("tok/abc") ?? return;
    const n1 = encodeProgressNotification(tok_str, 12.5, 100.0, "step \"A\"\nnext", *buf_s) ?? 0;
    expect_true(n1 > 0);

    if (p1 := parseProgressNotification(buf[0..n1])) {
        expect_eq(p1.token.kind, McpProgressTokenKind.str_token);
        expect_true(bytesEq(p1.token.strSlice(), "tok/abc"));
        expect_true(absF64(p1.progress - 12.5) < 1.0e-6);
        expect_true(absF64((p1.total ?? 0.0) - 100.0) < 1.0e-6);
        expect_true(bytesEq(p1.messageSlice(), "step \"A\"\nnext"));
    } else {
        expect_true(false);
    }

    // Integer token without total or message
    const tok_int = McpProgressToken.fromInt(9001);
    const n2 = encodeProgressNotification(tok_int, 7.0, null, "", *buf_s) ?? 0;
    expect_true(n2 > 0);
    if (p2 := parseProgressNotification(buf[0..n2])) {
        expect_eq(p2.token.kind, McpProgressTokenKind.int_token);
        expect_eq(p2.token.int_val, 9001);
        expect_true(absF64(p2.progress - 7.0) < 1.0e-6);
        expect_true(p2.total == null);
        expect_eq(p2.messageSlice().len, 0);
    } else {
        expect_true(false);
    }

    // Undersized buffer returns null safely
    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(encodeProgressNotification(tok_int, 1.0, null, "", *tiny_s) == null);
});

// ============================================================================
// Test 6: Zero-Alloc JSON-RPC 2.0 notifications/cancelled Round-Trip
// ============================================================================
test("mcpProgress: encodeCancelledNotification and parseCancelledNotification round-trip", () => {
    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];

    const n1 = encodeCancelledNotification(777, "timeout \"exceeded\"", *buf_s) ?? 0;
    expect_true(n1 > 0);
    if (c1 := parseCancelledNotification(buf[0..n1])) {
        expect_eq(c1.request_id, 777);
        expect_true(bytesEq(c1.reasonSlice(), "timeout \"exceeded\""));
    } else {
        expect_true(false);
    }

    // Empty reason omitted & parsed as empty
    const n2 = encodeCancelledNotification(888, "", *buf_s) ?? 0;
    expect_true(n2 > 0);
    if (c2 := parseCancelledNotification(buf[0..n2])) {
        expect_eq(c2.request_id, 888);
        expect_eq(c2.reasonSlice().len, 0);
    } else {
        expect_true(false);
    }

    // Invalid payloads rejected
    expect_true(encodeCancelledNotification(0, "bad", *buf_s) == null);
    expect_true(parseCancelledNotification("{\"jsonrpc\":\"2.0\",\"method\":\"other\",\"params\":{\"requestId\":1}}") == null);
    expect_true(parseCancelledNotification("{\"jsonrpc\":\"2.0\",\"method\":\"notifications/cancelled\",\"params\":{\"requestId\":0}}") == null);
});

// ============================================================================
// Test 7: Boundary Guards, 64-Slot Full Pool & cap.mcp Facade
// ============================================================================
test("mcpProgress: boundary guards, 64-slot capacity limit & cap.mcp facade verification", () => {
    let hub = cap.mcp.McpProgressHub.init(0, 5_000);

    // Invalid token & invalid registration params
    expect_true(McpProgressToken.fromStr("") == null);
    const t0 = McpProgressToken.fromInt(1);
    expect_true(hub.register(t0, 0, 10.0, null, 0, 100) == null);
    expect_true(hub.register(t0, 1, 0.0, null, 0, 100) == null);
    expect_true(hub.register(t0, 1, std.math.nan(f64), null, 0, 100) == null);

    // Fill all 64 slots
    let i: usize = 0;
    while (i < MCP_PROGRESS_MAX_SLOTS) : (i += 1) {
        const tok = McpProgressToken.fromInt((i + 1) as i64);
        expect_true(hub.register(tok, (i + 1) as u64, 10.0, null, 0, 100) != null);
    }
    // Duplicate token/req and 65th slot rejected
    expect_true(hub.register(McpProgressToken.fromInt(1), 999, 10.0, null, 0, 100) == null);
    expect_true(hub.register(McpProgressToken.fromInt(999), 999, 10.0, null, 0, 100) == null);

    // Invalid NaN/negative progress rejected
    expect_eq(hub.reportProgress(McpProgressToken.fromInt(1), -1.0, null, "", 200), McpProgressReportStatus.invalid_value);
    expect_eq(hub.reportProgress(McpProgressToken.fromInt(1), std.math.nan(f64), null, "", 200), McpProgressReportStatus.invalid_value);
    expect_eq(hub.reportProgress(McpProgressToken.fromInt(999), 1.0, null, "", 200), McpProgressReportStatus.not_found);

    // Complete slot 1, purge terminal -> 65th slot now succeeds
    expect_true(hub.complete(1));
    expect_eq(hub.purgeTerminal(), 1);
    expect_true(hub.register(McpProgressToken.fromInt(999), 999, 10.0, null, 0, 300) != null);
    expect_eq(hub.stats().active_count, MCP_PROGRESS_MAX_SLOTS);
});
