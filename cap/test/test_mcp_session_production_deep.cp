// Origin: Pure-CP Stdlib Test Suite (MCP Session Lifecycle, Capability Negotiation & Request Tracker) | Created: 2026-11-22
// Deep Production Verification Suite for lib/mcp/session.cp
// Covers:
//   1. Protocol version parse/format/negotiation & capability bitmask intersection
//   2. Strict lifecycle state machine transitions & method/capability gating
//   3. Inflight request register/complete/cancel/purge & late-response cancellation semantics
//   4. Batch timeout collection, zero-timeout exemption, backward clock skew & output buffer truncation safety
//   5. Duplicate request ID, zero ID, full-pool overflow, slot reuse & zero-capacity boundary guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    McpProtocolVersion,
    parseMcpVersion,
    formatMcpVersion,
    negotiateMcpVersion,
    McpCapabilityFlags,
    negotiateCapabilities,
    McpSessionState,
    McpSessionOpError,
    McpInflightRequest,
    McpSessionStats,
    hashMethodName,
    McpSession,
} from "../../lib/mcp/index.cp";

fn testVersionAndCapabilityNegotiation()! {
    // Version parsing & formatting
    expect_eq(parseMcpVersion("2024-11-05") == McpProtocolVersion.V2024_11_05, true);
    expect_eq(parseMcpVersion("2025-03-26") == McpProtocolVersion.V2025_03_26, true);
    expect_eq(parseMcpVersion("2023-01-01") == McpProtocolVersion.Unknown, true);
    expect_eq(parseMcpVersion("") == McpProtocolVersion.Unknown, true);

    expect_eq(std.mem.eql(u8, formatMcpVersion(McpProtocolVersion.V2024_11_05), "2024-11-05"), true);
    expect_eq(std.mem.eql(u8, formatMcpVersion(McpProtocolVersion.V2025_03_26), "2025-03-26"), true);
    expect_eq(std.mem.eql(u8, formatMcpVersion(McpProtocolVersion.Unknown), "unknown"), true);

    // Version negotiation matrix
    const v_same = negotiateMcpVersion(McpProtocolVersion.V2025_03_26, McpProtocolVersion.V2025_03_26);
    expect_eq(v_same != null, true);
    expect_eq((v_same ?? McpProtocolVersion.Unknown) == McpProtocolVersion.V2025_03_26, true);

    const v_down_srv = negotiateMcpVersion(McpProtocolVersion.V2025_03_26, McpProtocolVersion.V2024_11_05);
    expect_eq((v_down_srv ?? McpProtocolVersion.Unknown) == McpProtocolVersion.V2024_11_05, true);

    const v_down_cli = negotiateMcpVersion(McpProtocolVersion.V2024_11_05, McpProtocolVersion.V2025_03_26);
    expect_eq((v_down_cli ?? McpProtocolVersion.Unknown) == McpProtocolVersion.V2024_11_05, true);

    expect_eq(negotiateMcpVersion(McpProtocolVersion.Unknown, McpProtocolVersion.V2025_03_26) == null, true);
    expect_eq(negotiateMcpVersion(McpProtocolVersion.V2025_03_26, McpProtocolVersion.Unknown) == null, true);

    // Capability bitmask operations & high-bit sanitization
    const raw_client = McpCapabilityFlags.fromBits(
        McpCapabilityFlags.TOOLS |
            McpCapabilityFlags.RESOURCES |
            McpCapabilityFlags.SAMPLING |
            McpCapabilityFlags.ROOTS |
            0xFFFF0000,
    );
    expect_eq(raw_client.bits, McpCapabilityFlags.TOOLS | McpCapabilityFlags.RESOURCES | McpCapabilityFlags.SAMPLING | McpCapabilityFlags.ROOTS);
    expect_eq(raw_client.has(McpCapabilityFlags.TOOLS | McpCapabilityFlags.RESOURCES), true);
    expect_eq(raw_client.has(McpCapabilityFlags.PROMPTS), false);
    expect_eq(raw_client.hasAny(McpCapabilityFlags.PROMPTS | McpCapabilityFlags.ROOTS), true);
    expect_eq(raw_client.isEmpty(), false);

    const updated = raw_client.with(McpCapabilityFlags.LOGGING).without(McpCapabilityFlags.SAMPLING);
    expect_eq(updated.has(McpCapabilityFlags.LOGGING), true);
    expect_eq(updated.has(McpCapabilityFlags.SAMPLING), false);

    const server_caps = McpCapabilityFlags.fromBits(
        McpCapabilityFlags.TOOLS |
            McpCapabilityFlags.RESOURCES |
            McpCapabilityFlags.PROMPTS |
            McpCapabilityFlags.LIST_CHANGED,
    );
    const negotiated = negotiateCapabilities(updated, server_caps);
    expect_eq(negotiated.bits, McpCapabilityFlags.TOOLS | McpCapabilityFlags.RESOURCES);
    expect_eq(negotiated.has(McpCapabilityFlags.TOOLS), true);
    expect_eq(negotiated.has(McpCapabilityFlags.RESOURCES), true);
    expect_eq(negotiated.has(McpCapabilityFlags.PROMPTS), false);
    expect_eq(negotiated.has(McpCapabilityFlags.LOGGING), false);
    expect_eq(McpCapabilityFlags{}.isEmpty(), true);
}

fn testSessionLifecycleStateMachineAndGating()! {
    let slots_buf: [4]McpInflightRequest = [McpInflightRequest{}; 4];
    let slots: []McpInflightRequest = slots_buf[0..];
    let sess = McpSession.init(slots);

    // Initial Disconnected state
    expect_eq(sess.getState() == McpSessionState.Disconnected, true);
    expect_eq(sess.isReady(), false);
    expect_eq(sess.canDispatchMethod("initialize"), true);
    expect_eq(sess.canDispatchMethod("tools/call"), false);
    expect_eq(sess.canDispatchMethod("ping"), false);

    // Illegal transitions before initialize
    const err_ready_early = sess.markReady();
    expect_eq((err_ready_early ?? McpSessionOpError.RequestNotFound) == McpSessionOpError.InvalidState, true);
    const err_close_early = sess.beginClose();
    expect_eq((err_close_early ?? McpSessionOpError.RequestNotFound) == McpSessionOpError.InvalidState, true);
    const err_req_early = sess.beginMethodRequest(1, "tools/call", 100, 5000);
    expect_eq((err_req_early ?? McpSessionOpError.RequestNotFound) == McpSessionOpError.InvalidState, true);

    // Incompatible version keeps state Disconnected
    const cli_caps = McpCapabilityFlags.fromBits(McpCapabilityFlags.TOOLS | McpCapabilityFlags.RESOURCES);
    const srv_caps = McpCapabilityFlags.fromBits(McpCapabilityFlags.TOOLS | McpCapabilityFlags.RESOURCES | McpCapabilityFlags.PROMPTS);
    const err_ver = sess.beginInitialize(McpProtocolVersion.Unknown, McpProtocolVersion.V2025_03_26, cli_caps, srv_caps);
    expect_eq((err_ver ?? McpSessionOpError.InvalidState) == McpSessionOpError.IncompatibleVersion, true);
    expect_eq(sess.getState() == McpSessionState.Disconnected, true);

    // Valid Disconnected -> Initializing
    expect_eq(sess.beginInitialize(McpProtocolVersion.V2025_03_26, McpProtocolVersion.V2025_03_26, cli_caps, srv_caps) == null, true);
    expect_eq(sess.getState() == McpSessionState.Initializing, true);
    expect_eq(sess.getNegotiatedVersion() == McpProtocolVersion.V2025_03_26, true);
    expect_eq(sess.getNegotiatedCapabilities().bits, McpCapabilityFlags.TOOLS | McpCapabilityFlags.RESOURCES);

    // Initializing gates out tools/call and duplicate initialize, allows notifications/initialized & ping
    expect_eq(sess.canDispatchMethod("notifications/initialized"), true);
    expect_eq(sess.canDispatchMethod("ping"), true);
    expect_eq(sess.canDispatchMethod("tools/call"), false);
    const err_dup_init = sess.beginInitialize(McpProtocolVersion.V2025_03_26, McpProtocolVersion.V2025_03_26, cli_caps, srv_caps);
    expect_eq((err_dup_init ?? McpSessionOpError.IncompatibleVersion) == McpSessionOpError.InvalidState, true);
    const err_call_init = sess.beginMethodRequest(1, "tools/call", 200, 5000);
    expect_eq((err_call_init ?? McpSessionOpError.RequestNotFound) == McpSessionOpError.InvalidState, true);

    // Initializing -> Ready
    expect_eq(sess.markReady() == null, true);
    expect_eq(sess.isReady(), true);
    expect_eq(sess.canDispatchMethod("tools/call"), true);
    expect_eq(sess.canDispatchMethod("resources/read"), true);
    expect_eq(sess.canDispatchMethod("ping"), true);

    // Unnegotiated capabilities must be rejected with CapabilityUnsupported
    const err_sub = sess.checkMethodAllowed("resources/subscribe");
    expect_eq((err_sub ?? McpSessionOpError.InvalidState) == McpSessionOpError.CapabilityUnsupported, true);
    const err_prompt = sess.beginMethodRequest(2, "prompts/get", 300, 5000);
    expect_eq((err_prompt ?? McpSessionOpError.InvalidState) == McpSessionOpError.CapabilityUnsupported, true);
    const err_sampling = sess.checkMethodAllowed("sampling/createMessage");
    expect_eq((err_sampling ?? McpSessionOpError.InvalidState) == McpSessionOpError.CapabilityUnsupported, true);

    // Cannot reset while Ready
    const err_reset_active = sess.reset();
    expect_eq((err_reset_active ?? McpSessionOpError.RequestNotFound) == McpSessionOpError.InvalidState, true);

    // Register one inflight request, then beginClose -> Closing
    expect_eq(sess.beginMethodRequest(10, "tools/call", 400, 5000) == null, true);
    expect_eq(sess.beginClose() == null, true);
    expect_eq(sess.getState() == McpSessionState.Closing, true);
    expect_eq(sess.canDispatchMethod("tools/call"), false);

    // Closing rejects new requests, allows finishing close which cancels remaining inflight
    const err_req_closing = sess.beginMethodRequest(11, "tools/call", 450, 5000);
    expect_eq((err_req_closing ?? McpSessionOpError.RequestNotFound) == McpSessionOpError.InvalidState, true);
    expect_eq(sess.finishClose() == null, true);
    expect_eq(sess.getState() == McpSessionState.Closed, true);
    expect_eq(sess.inflightCount(), 0);
    expect_eq(sess.getStats().total_cancelled, 1);

    // Reset from Closed -> clean Disconnected
    expect_eq(sess.reset() == null, true);
    expect_eq(sess.getState() == McpSessionState.Disconnected, true);
    expect_eq(sess.getNegotiatedVersion() == McpProtocolVersion.Unknown, true);
    expect_eq(sess.getNegotiatedCapabilities().isEmpty(), true);
    expect_eq(sess.getStats().total_started, 0);
}

fn testInflightRequestLifecycleAndCancellation()! {
    let slots_buf: [6]McpInflightRequest = [McpInflightRequest{}; 6];
    let slots: []McpInflightRequest = slots_buf[0..];
    let sess = McpSession.init(slots);

    const all_caps = McpCapabilityFlags.fromBits(McpCapabilityFlags.ALL_KNOWN);
    expect_eq(sess.beginInitialize(McpProtocolVersion.V2025_03_26, McpProtocolVersion.V2025_03_26, all_caps, all_caps) == null, true);
    expect_eq(sess.markReady() == null, true);

    // Deterministic FNV-1a method hash
    const h_tools = hashMethodName("tools/call");
    expect_eq(h_tools, hashMethodName("tools/call"));
    expect_eq(h_tools != hashMethodName("tools/list"), true);

    // Start 4 requests
    expect_eq(sess.beginMethodRequest(101, "tools/call", 1000, 3000) == null, true);
    expect_eq(sess.beginMethodRequest(102, "resources/read", 1100, 3000) == null, true);
    expect_eq(sess.beginMethodRequest(103, "prompts/get", 1200, 3000) == null, true);
    expect_eq(sess.beginMethodRequest(104, "logging/setLevel", 1300, 3000) == null, true);
    expect_eq(sess.inflightCount(), 4);

    const rec101 = sess.getInflight(101) ?? McpInflightRequest{};
    expect_eq(rec101.active, true);
    expect_eq(rec101.request_id, 101);
    expect_eq(rec101.method_hash, h_tools);
    expect_eq(rec101.cancelled, false);

    // Normal completion of 101
    const done101 = sess.completeRequest(101);
    expect_eq(done101 != null, true);
    expect_eq((done101 ?? McpInflightRequest{}).cancelled, false);
    expect_eq(sess.completeRequest(101) == null, true);
    expect_eq(sess.inflightCount(), 3);

    // Cancel 102 (idempotent second cancel returns false)
    expect_eq(sess.cancelRequest(102), true);
    expect_eq(sess.cancelRequest(102), false);
    expect_eq(sess.cancelRequest(999), false);
    const rec102 = sess.getInflight(102) ?? McpInflightRequest{};
    expect_eq(rec102.cancelled, true);

    // Late response arrives for cancelled 102: completeRequest returns cancelled=true and does NOT increment total_completed
    const late102 = sess.completeRequest(102) ?? McpInflightRequest{};
    expect_eq(late102.request_id, 102);
    expect_eq(late102.cancelled, true);
    expect_eq(sess.inflightCount(), 2);

    // Cancel 103 and drain with purgeCancelled
    expect_eq(sess.cancelRequest(103), true);
    let purged_buf: [4]u64 = [0; 4];
    let purged: []u64 = purged_buf[0..];
    const purged_n = sess.purgeCancelled(purged);
    expect_eq(purged_n, 1);
    expect_eq(purged[0], 103);
    expect_eq(sess.inflightCount(), 1);

    // forceClose cancels remaining active request 104
    const forced = sess.forceClose();
    expect_eq(forced, 1);
    expect_eq(sess.getState() == McpSessionState.Closed, true);
    expect_eq(sess.inflightCount(), 0);

    const st = sess.getStats();
    expect_eq(st.total_started, 4);
    expect_eq(st.total_completed, 1);
    expect_eq(st.total_cancelled, 3);
    expect_eq(st.peak_inflight, 4);
}

fn testTimeoutCollectionAndClockSkewDefense()! {
    let slots_buf: [6]McpInflightRequest = [McpInflightRequest{}; 6];
    let slots: []McpInflightRequest = slots_buf[0..];
    let sess = McpSession.init(slots);

    const caps = McpCapabilityFlags.fromBits(McpCapabilityFlags.TOOLS);
    expect_eq(sess.beginInitialize(McpProtocolVersion.V2025_03_26, McpProtocolVersion.V2025_03_26, caps, caps) == null, true);
    expect_eq(sess.markReady() == null, true);

    // Req 1: sent=1000, timeout=500 -> expires at 1500
    // Req 2: sent=1100, timeout=1000 -> expires at 2100
    // Req 3: sent=1200, timeout=0 -> never expires
    // Req 4: sent=-50 (clamped to 0), timeout=200 -> expires at 200
    // Req 5: sent=1000, timeout=500, pre-cancelled -> expires at 1500 without incrementing total_timed_out
    // Req 6: sent=1000, timeout=-100 (clamped to 0) -> never expires
    expect_eq(sess.beginRequest(1, 10, 1000, 500) == null, true);
    expect_eq(sess.beginRequest(2, 20, 1100, 1000) == null, true);
    expect_eq(sess.beginRequest(3, 30, 1200, 0) == null, true);
    expect_eq(sess.beginRequest(4, 40, -50, 200) == null, true);
    expect_eq(sess.beginRequest(5, 50, 1000, 500) == null, true);
    expect_eq(sess.beginRequest(6, 60, 1000, -100) == null, true);
    expect_eq(sess.cancelRequest(5), true);

    let out_buf: [2]u64 = [0; 2];
    let out: []u64 = out_buf[0..];

    // Negative / backward clock skew triggers 0 timeouts
    expect_eq(sess.collectTimedOut(-100, out), 0);
    expect_eq(sess.collectTimedOut(199, out), 0);
    expect_eq(sess.inflightCount(), 6);

    // At t=1499: only Req 4 (deadline 200) has expired; Req 1 & 5 (deadline 1500) are still alive
    expect_eq(sess.collectTimedOut(1499, out), 1);
    expect_eq(out[0], 4);
    expect_eq(sess.inflightCount(), 5);
    expect_eq(sess.getStats().total_timed_out, 1);

    // At t=2500: Req 1, Req 2, Req 5 all expire (3 total), but out buffer only holds 2 -> truncation-safe!
    const reclaimed = sess.collectTimedOut(2500, out);
    expect_eq(reclaimed, 3);
    expect_eq(out[0], 1);
    expect_eq(out[1], 2);
    // Req 3 and Req 6 (no timeout) remain active
    expect_eq(sess.inflightCount(), 2);
    expect_eq(sess.getInflight(3) != null, true);
    expect_eq(sess.getInflight(6) != null, true);
    // Req 5 was already cancelled, so total_timed_out only increased by 2 (Req 1 + Req 2) => 1 + 2 = 3
    expect_eq(sess.getStats().total_timed_out, 3);
}

fn testDuplicateIdFullPoolAndBoundaryGuards()! {
    let slots_buf: [2]McpInflightRequest = [McpInflightRequest{}; 2];
    let slots: []McpInflightRequest = slots_buf[0..];
    let sess = McpSession.init(slots);

    const caps = McpCapabilityFlags.fromBits(McpCapabilityFlags.TOOLS);
    expect_eq(sess.beginInitialize(McpProtocolVersion.V2024_11_05, McpProtocolVersion.V2025_03_26, caps, caps) == null, true);
    expect_eq(sess.markReady() == null, true);
    expect_eq(sess.capacity(), 2);

    // Zero request_id rejected
    const err_zero = sess.beginRequest(0, 1, 100, 1000);
    expect_eq((err_zero ?? McpSessionOpError.InvalidState) == McpSessionOpError.InvalidRequestId, true);

    // Fill 2 slots
    expect_eq(sess.beginRequest(10, 1, 100, 1000) == null, true);
    // Duplicate active ID rejected
    const err_dup = sess.beginRequest(10, 2, 110, 1000);
    expect_eq((err_dup ?? McpSessionOpError.InvalidState) == McpSessionOpError.DuplicateRequestId, true);

    // Cancelled-but-undrained ID still rejected as duplicate
    expect_eq(sess.cancelRequest(10), true);
    const err_dup_cancel = sess.beginRequest(10, 3, 120, 1000);
    expect_eq((err_dup_cancel ?? McpSessionOpError.InvalidState) == McpSessionOpError.DuplicateRequestId, true);

    expect_eq(sess.beginRequest(20, 2, 130, 1000) == null, true);

    // Pool is full (2/2)
    const err_full = sess.beginRequest(30, 3, 140, 1000);
    expect_eq((err_full ?? McpSessionOpError.InvalidState) == McpSessionOpError.InflightPoolFull, true);

    // Free slot 20 and reuse
    expect_eq(sess.completeRequest(20) != null, true);
    expect_eq(sess.beginRequest(30, 3, 150, 1000) == null, true);
    expect_eq(sess.inflightCount(), 2);
    expect_eq(sess.getStats().peak_inflight, 2);
    expect_eq(sess.getStats().total_rejected, 4);

    // Zero-capacity slot pool boundary test
    let empty_buf: [0]McpInflightRequest = undefined;
    let empty_slots: []McpInflightRequest = empty_buf[0..];
    let empty_sess = McpSession.init(empty_slots);
    expect_eq(empty_sess.capacity(), 0);
    expect_eq(empty_sess.beginInitialize(McpProtocolVersion.V2025_03_26, McpProtocolVersion.V2025_03_26, caps, caps) == null, true);
    expect_eq(empty_sess.markReady() == null, true);
    const err_empty = empty_sess.beginRequest(1, 1, 100, 1000);
    expect_eq((err_empty ?? McpSessionOpError.InvalidState) == McpSessionOpError.InflightPoolFull, true);
    expect_eq(empty_sess.inflightCount(), 0);
}

fn main() {
    test("1. mcp session: version & capability bitmask negotiation", () => {
        testVersionAndCapabilityNegotiation()?;
    });
    test("2. mcp session: strict lifecycle state machine & method gating", () => {
        testSessionLifecycleStateMachineAndGating()?;
    });
    test("3. mcp session: inflight request register/complete/cancel/purge", () => {
        testInflightRequestLifecycleAndCancellation()?;
    });
    test("4. mcp session: timeout collection, clock skew & truncation defense", () => {
        testTimeoutCollectionAndClockSkewDefense()?;
    });
    test("5. mcp session: duplicate ID, zero ID, full-pool & zero-capacity guards", () => {
        testDuplicateIdFullPoolAndBoundaryGuards()?;
    });
}
