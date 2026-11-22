// Origin: Pure-CP Stdlib Test Suite (lib/mcp/capability_negotiator.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc MCP Capability Negotiator & Resource Subscription Router:
//   1. Lossless bidirectional protocol version parsing, ordering & compatible version downgrade
//   2. Capability bitmask intersection, unknown-bit masking & missing required capability rejection
//   3. Strict 5-phase session state machine guards & ready-state can_invoke gating
//   4. Exact & trailing-* wildcard resource subscription matching with per-client deduplication
//   5. Idempotent subscribe dedup, unsubscribe / unsubscribe_all & 64-slot reclamation/reuse
//   6. Comprehensive boundary guards (unknown version, invalid range, oversized URI, mid-wildcard, full 64 slots)
//   7. Coexistence check with existing cap:mcp symbols (session.cp::McpProtocolVersion)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    McpProtocolVersion,
    MCP_CAP_TOOLS,
    MCP_CAP_TOOLS_LIST_CHANGED,
    MCP_CAP_RESOURCES,
    MCP_CAP_RESOURCES_SUBSCRIBE,
    MCP_CAP_PROMPTS,
    MCP_CAP_LOGGING,
    MCP_CAP_SAMPLING,
    MCP_CAP_ROOTS,
    MCP_CAP_ELICITATION,
    MCP_CAP_ALL_KNOWN,
    MCP_SUB_MAX_SLOTS,
    MCP_SUB_MAX_URI_LEN,
    McpNegotiatedVersion,
    McpSessionPhase,
    McpHandshakeRequest,
    McpHandshakeResult,
    McpSessionNegotiator,
    McpResourceSubscriptionRegistry,
} from "cap:mcp";

// ============================================================================
// Test 1: Bidirectional Protocol Version Parsing, Ordering & Downgrade
// ============================================================================
test("mcp_negotiator: lossless version parse/toString, ordering & compatible downgrade", () => {
    const v1 = McpNegotiatedVersion.parse("2024-11-05");
    const v2 = McpNegotiatedVersion.parse("2025-03-26");
    const v3 = McpNegotiatedVersion.parse("2025-06-18");
    const vu = McpNegotiatedVersion.parse("2099-01-01");

    expect_eq(v1 == McpNegotiatedVersion.v2024_11_05, true);
    expect_eq(v2 == McpNegotiatedVersion.v2025_03_26, true);
    expect_eq(v3 == McpNegotiatedVersion.v2025_06_18, true);
    expect_eq(vu == McpNegotiatedVersion.unknown, true);

    expect_eq(std.mem.eql(u8, v1.toString(), "2024-11-05"), true);
    expect_eq(std.mem.eql(u8, v2.toString(), "2025-03-26"), true);
    expect_eq(std.mem.eql(u8, v3.toString(), "2025-06-18"), true);
    expect_eq(std.mem.eql(u8, vu.toString(), "unknown"), true);

    // Ordering & comparisons
    expect_eq(v1.isKnown(), true);
    expect_eq(vu.isKnown(), false);
    expect_eq(v1.compare(v2), -1);
    expect_eq(v2.compare(v2), 0);
    expect_eq(v3.compare(v2), 1);
    expect_eq(v3.isAtLeast(v2), true);
    expect_eq(v1.isAtMost(v2), true);
    expect_eq(vu.isAtLeast(v1), false);

    // Compatible downgrade: server supports [2024-11-05, 2025-03-26], client proposes 2025-06-18
    let neg_down = McpSessionNegotiator.init(
        McpNegotiatedVersion.v2024_11_05,
        McpNegotiatedVersion.v2025_03_26,
        MCP_CAP_TOOLS | MCP_CAP_RESOURCES,
        0,
    );
    const r_down = neg_down.negotiate("2025-06-18", MCP_CAP_TOOLS);
    expect_eq(r_down.accepted, true);
    expect_eq(r_down.negotiated_version == McpNegotiatedVersion.v2025_03_26, true);

    // Client too old: server requires >= 2025-03-26, client proposes 2024-11-05 -> rejected
    let neg_old = McpSessionNegotiator.init(
        McpNegotiatedVersion.v2025_03_26,
        McpNegotiatedVersion.v2025_06_18,
        MCP_CAP_TOOLS,
        0,
    );
    const r_old = neg_old.negotiate("2024-11-05", MCP_CAP_TOOLS);
    expect_eq(r_old.accepted, false);
    expect_eq(r_old.negotiated_version == McpNegotiatedVersion.unknown, true);
    expect_eq(neg_old.phase == McpSessionPhase.uninitialized, true);
});

// ============================================================================
// Test 2: Capability Bitmask Intersection & Missing Required Caps Rejection
// ============================================================================
test("mcp_negotiator: capability intersection, unknown-bit masking & required cap rejection", () => {
    const server_caps: u32 =
        MCP_CAP_TOOLS |
        MCP_CAP_TOOLS_LIST_CHANGED |
        MCP_CAP_RESOURCES |
        MCP_CAP_RESOURCES_SUBSCRIBE |
        MCP_CAP_PROMPTS |
        MCP_CAP_ELICITATION;
    const required_caps: u32 = MCP_CAP_TOOLS | MCP_CAP_SAMPLING;

    let neg = McpSessionNegotiator.init(
        McpNegotiatedVersion.v2024_11_05,
        McpNegotiatedVersion.v2025_06_18,
        server_caps | 0xFFFF0000, // unknown high bits must be stripped
        required_caps,
    );

    // Client lacks MCP_CAP_SAMPLING -> rejected with exact missing_required_caps
    const bad_res = neg.negotiate("2025-06-18", MCP_CAP_TOOLS | MCP_CAP_RESOURCES);
    expect_eq(bad_res.accepted, false);
    expect_eq(bad_res.missing_required_caps, MCP_CAP_SAMPLING);
    expect_eq(neg.phase == McpSessionPhase.uninitialized, true);

    // Retry with required caps present (+ unknown high bits that must be masked off)
    const req = McpHandshakeRequest{
        client_version: "2025-06-18",
        client_caps: MCP_CAP_TOOLS | MCP_CAP_SAMPLING | MCP_CAP_ELICITATION | MCP_CAP_ROOTS | 0x80000000,
    };
    const ok_res = neg.negotiate_request(req);
    expect_eq(ok_res.accepted, true);
    expect_eq(ok_res.negotiated_version == McpNegotiatedVersion.v2025_06_18, true);
    expect_eq(ok_res.missing_required_caps, 0);
    // Intersection: server_caps & client_caps == TOOLS | ELICITATION (SAMPLING & ROOTS not in server_caps)
    expect_eq(ok_res.effective_caps, MCP_CAP_TOOLS | MCP_CAP_ELICITATION);
    expect_eq((ok_res.effective_caps & (~MCP_CAP_ALL_KNOWN)), 0);
    expect_eq(neg.phase == McpSessionPhase.initializing, true);
});

// ============================================================================
// Test 3: Strict Session State Machine Transitions & can_invoke Gating
// ============================================================================
test("mcp_negotiator: session phase transitions and can_invoke gatekeeper", () => {
    let neg = McpSessionNegotiator.init(
        McpNegotiatedVersion.v2024_11_05,
        McpNegotiatedVersion.v2025_06_18,
        MCP_CAP_TOOLS | MCP_CAP_LOGGING,
        MCP_CAP_TOOLS,
    );

    // Phase: uninitialized
    expect_eq(neg.phase == McpSessionPhase.uninitialized, true);
    expect_eq(neg.mark_initialized(), false);
    expect_eq(neg.finish_close(), false);
    expect_eq(neg.can_invoke(MCP_CAP_TOOLS), false);

    // Transition: uninitialized -> initializing
    const r = neg.negotiate("2025-03-26", MCP_CAP_TOOLS | MCP_CAP_LOGGING | MCP_CAP_PROMPTS);
    expect_eq(r.accepted, true);
    expect_eq(neg.phase == McpSessionPhase.initializing, true);
    // Double negotiate in initializing phase is rejected
    expect_eq(neg.negotiate("2025-03-26", MCP_CAP_TOOLS).accepted, false);
    // Not ready yet -> can_invoke must still return false
    expect_eq(neg.can_invoke(MCP_CAP_TOOLS), false);

    // Transition: initializing -> ready
    expect_eq(neg.mark_initialized(), true);
    expect_eq(neg.phase == McpSessionPhase.ready, true);
    // Double mark_initialized rejected
    expect_eq(neg.mark_initialized(), false);

    // Ready gating
    expect_eq(neg.can_invoke(MCP_CAP_TOOLS), true);
    expect_eq(neg.can_invoke(MCP_CAP_LOGGING), true);
    expect_eq(neg.can_invoke(MCP_CAP_TOOLS | MCP_CAP_LOGGING), true);
    expect_eq(neg.can_invoke(MCP_CAP_PROMPTS), false);
    expect_eq(neg.can_invoke(MCP_CAP_RESOURCES), false);

    // Shutdown: ready -> closing -> closed
    expect_eq(neg.begin_close(), true);
    expect_eq(neg.phase == McpSessionPhase.closing, true);
    expect_eq(neg.can_invoke(MCP_CAP_TOOLS), false);
    expect_eq(neg.begin_close(), false);

    expect_eq(neg.finish_close(), true);
    expect_eq(neg.phase == McpSessionPhase.closed, true);
    expect_eq(neg.effective_caps, 0);
    expect_eq(neg.finish_close(), false);
    expect_eq(neg.can_invoke(MCP_CAP_TOOLS), false);
});

// ============================================================================
// Test 4: Exact & Trailing-* Wildcard Resource Subscription & Client Dedup
// ============================================================================
test("mcp_negotiator: exact & wildcard URI matching with per-client deduplication", () => {
    let reg = McpResourceSubscriptionRegistry.init();

    // Client 10 subscribes to both prefix wildcard and exact file inside same prefix
    expect_eq(reg.subscribe(10, "file:///workspace/*"), true);
    expect_eq(reg.subscribe(10, "file:///workspace/src/main.cp"), true);
    // Client 20 subscribes only to exact file
    expect_eq(reg.subscribe(20, "file:///workspace/src/main.cp"), true);
    // Client 30 subscribes to a different prefix
    expect_eq(reg.subscribe(30, "file:///docs/*"), true);
    // Client 40 subscribes to global wildcard '*'
    expect_eq(reg.subscribe(40, "*"), true);
    expect_eq(reg.active_count(), 5);

    // Match file:///workspace/src/main.cp -> clients 10 (deduped!), 20, 40
    let out: [8]u64 = [0; 8];
    let out_s: []u64 = out[0..];
    const n1 = reg.match_subscribers("file:///workspace/src/main.cp", out_s);
    expect_eq(n1, 3);
    expect_eq(out[0], 10);
    expect_eq(out[1], 20);
    expect_eq(out[2], 40);

    // Match file:///workspace/README.md -> clients 10, 40
    const n2 = reg.match_subscribers("file:///workspace/README.md", out_s);
    expect_eq(n2, 2);
    expect_eq(out[0], 10);
    expect_eq(out[1], 40);

    // Match file:///docs/guide.md -> clients 30, 40
    const n3 = reg.match_subscribers("file:///docs/guide.md", out_s);
    expect_eq(n3, 2);
    expect_eq(out[0], 30);
    expect_eq(out[1], 40);

    // Small output buffer truncation safety
    let small: [1]u64 = [0; 1];
    let small_s: []u64 = small[0..];
    expect_eq(reg.match_subscribers("file:///workspace/src/main.cp", small_s), 1);
    expect_eq(small[0], 10);
});

// ============================================================================
// Test 5: Idempotent Dedup, Unsubscribe, Unsubscribe_All & Slot Reclamation
// ============================================================================
test("mcp_negotiator: idempotent subscribe, unsubscribe_all & slot reuse", () => {
    let reg = McpResourceSubscriptionRegistry.init();

    // Idempotent duplicate subscribe does not consume extra slot
    expect_eq(reg.subscribe(101, "mem://cache/*"), true);
    expect_eq(reg.subscribe(101, "mem://cache/*"), true);
    expect_eq(reg.active_count(), 1);

    expect_eq(reg.subscribe(101, "mem://metrics/cpu"), true);
    expect_eq(reg.subscribe(202, "mem://cache/*"), true);
    expect_eq(reg.active_count(), 3);

    // Unsubscribe specific pattern
    expect_eq(reg.unsubscribe(101, "mem://metrics/cpu"), true);
    expect_eq(reg.unsubscribe(101, "mem://metrics/cpu"), false);
    expect_eq(reg.active_count(), 2);

    // Add another rule for 101 then clear all for 101
    expect_eq(reg.subscribe(101, "mem://logs/*"), true);
    expect_eq(reg.active_count(), 3);
    expect_eq(reg.unsubscribe_all(101), 2);
    expect_eq(reg.unsubscribe_all(101), 0);
    expect_eq(reg.active_count(), 1);

    // Only client 202 remains subscribed to mem://cache/key1
    let out: [4]u64 = [0; 4];
    let out_s: []u64 = out[0..];
    const n = reg.match_subscribers("mem://cache/key1", out_s);
    expect_eq(n, 1);
    expect_eq(out[0], 202);
});

// ============================================================================
// Test 6: Boundary Guards (Unknown Version, Oversized URI, Mid-Wildcard, Full 64 Slots)
// ============================================================================
test("mcp_negotiator: boundary guards for unknown version, bad URIs & full 64 slots", () => {
    // Unknown / empty client version rejected
    let neg = McpSessionNegotiator.init(
        McpNegotiatedVersion.v2024_11_05,
        McpNegotiatedVersion.v2025_06_18,
        MCP_CAP_TOOLS,
        0,
    );
    expect_eq(neg.negotiate("invalid-ver", MCP_CAP_TOOLS).accepted, false);
    expect_eq(neg.negotiate("", MCP_CAP_TOOLS).accepted, false);

    // Invalid server range (min > max) rejects all handshakes
    let bad_neg = McpSessionNegotiator.init(
        McpNegotiatedVersion.v2025_06_18,
        McpNegotiatedVersion.v2024_11_05,
        MCP_CAP_TOOLS,
        0,
    );
    expect_eq(bad_neg.negotiate("2025-03-26", MCP_CAP_TOOLS).accepted, false);

    // Subscription registry guards
    let reg = McpResourceSubscriptionRegistry.init();
    expect_eq(reg.subscribe(0, "file:///a"), false);        // client_id == 0
    expect_eq(reg.subscribe(1, ""), false);                 // empty URI
    expect_eq(reg.subscribe(1, "file://*/bad"), false);     // mid-string wildcard
    expect_eq(reg.subscribe(1, "file:///a**"), false);      // double wildcard

    // Oversized URI (> 128 bytes) rejected
    let too_long: [129]u8 = [97; 129]; // 129 'a's
    expect_eq(reg.subscribe(1, too_long[0..]), false);
    expect_eq(MCP_SUB_MAX_URI_LEN, 128);
    expect_eq(MCP_SUB_MAX_SLOTS, 64);

    // Fill all 64 slots cleanly
    let i: u64 = 1;
    while (i <= 64) : (i += 1) {
        expect_eq(reg.subscribe(i, "file:///slot/*"), true);
    }
    expect_eq(reg.active_count(), 64);
    // 65th unique subscription rejected cleanly
    expect_eq(reg.subscribe(65, "file:///slot/*"), false);

    // Reclaim 1 slot and verify 65th now succeeds
    expect_eq(reg.unsubscribe(1, "file:///slot/*"), true);
    expect_eq(reg.active_count(), 63);
    expect_eq(reg.subscribe(65, "file:///slot/*"), true);
    expect_eq(reg.active_count(), 64);

    // Empty changed_uri / empty output buffer return 0
    let out: [4]u64 = [0; 4];
    let out_s: []u64 = out[0..];
    expect_eq(reg.match_subscribers("", out_s), 0);
    expect_eq(reg.match_subscribers("file:///slot/x", out[0..0]), 0);
});

// ============================================================================
// Test 7: Coexistence with Existing cap:mcp Barrel Symbols
// ============================================================================
test("mcp_negotiator: zero symbol conflict with existing session.cp McpProtocolVersion", () => {
    const legacy_ver = McpProtocolVersion.V2025_03_26;
    const neg_ver = McpNegotiatedVersion.v2025_06_18;
    expect_eq(legacy_ver == McpProtocolVersion.V2025_03_26, true);
    expect_eq(neg_ver == McpNegotiatedVersion.v2025_06_18, true);
});
