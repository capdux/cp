// Origin: Pure-CP Stdlib Test Suite (lib/a2a/handoffGate.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Multi-Agent Handoff Anti-Loop Gate & Rollback FSM:
//   1. Normal multi-hop handoff chain, owner tracking, capability matching & visit counts
//   2. Self-handoff, capability_mismatch (exact missing_caps), and direct ping-pong loop (A->B->A) guards
//   3. Per-agent revisit ceiling (`revisit_limit_exceeded`) across 3-node cycles (A->B->C->A->B->C->A)
//   4. Max active depth enforcement (`max_depth_exceeded`) & 16-hop audit table capacity (`history_full`)
//   5. Safe failure rollback (`rollbackLast`) restoring previous owner, preserving audit log & re-routing
//   6. Zero-alloc trace formatting (`formatTrace`), global stats & `cap.a2a` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    HANDOFF_GATE_MAX_HOPS,
    HANDOFF_GATE_NO_AGENT,
    HandoffReason,
    HandoffHopState,
    HandoffDecisionKind,
    HandoffGateConfig,
    HandoffGate,
    formatHandoffReason,
    formatHandoffDecision,
    formatHandoffHopState,
} from "cap:a2a";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

test("HandoffGate: normal multi-hop chain, ownership tracking & visit counts") {
    // Reject invalid initial agent ID 0
    expect_true(HandoffGate.init(HANDOFF_GATE_NO_AGENT) == null);

    let gate = HandoffGate.init(10) ?? return;
    expect_eq(gate.originAgent(), @as(u32, 10));
    expect_eq(gate.currentOwner(), @as(u32, 10));
    expect_eq(gate.activeDepth(), @as(usize, 0));
    expect_eq(gate.visitCount(10), @as(u8, 1));

    const CAP_SQL: u64 = 1 << 0;
    const CAP_CODE: u64 = 1 << 1;
    const CAP_SEC: u64 = 1 << 2;

    // 10 -> 20 (specialist_escalation)
    const d1 = gate.requestHandoff(20, CAP_SQL | CAP_CODE, CAP_SQL, 0x1111, HandoffReason.specialist_escalation, 1000);
    expect_true(d1.allowed);
    expect_eq(d1.kind, HandoffDecisionKind.allowed);
    expect_eq(d1.from_agent_id, @as(u32, 10));
    expect_eq(d1.to_agent_id, @as(u32, 20));
    expect_eq(d1.active_depth, @as(usize, 1));
    expect_eq(d1.target_visits, @as(u8, 1));
    expect_eq(gate.currentOwner(), @as(u32, 20));

    // 20 -> 30 (user_request)
    const d2 = gate.requestHandoff(30, CAP_SQL | CAP_CODE | CAP_SEC, CAP_SEC, 0x2222, HandoffReason.user_request, 2000);
    expect_true(d2.allowed);
    expect_eq(gate.currentOwner(), @as(u32, 30));
    expect_eq(gate.activeDepth(), @as(usize, 2));
    expect_eq(gate.historyLen(), @as(usize, 2));

    if (h0 := gate.getHop(0)) {
        expect_eq(h0.hop_seq, @as(u32, 1));
        expect_eq(h0.from_agent_id, @as(u32, 10));
        expect_eq(h0.to_agent_id, @as(u32, 20));
        expect_eq(h0.context_hash, @as(u64, 0x1111));
        expect_eq(h0.state, HandoffHopState.committed);
    } else {
        expect_true(false);
    }

    expect_true(bytesEq(formatHandoffReason(HandoffReason.specialist_escalation), "specialist_escalation"));
    expect_true(bytesEq(formatHandoffDecision(HandoffDecisionKind.ping_pong_loop), "ping_pong_loop"));
    expect_true(bytesEq(formatHandoffHopState(HandoffHopState.rolled_back), "rolled_back"));
}

test("HandoffGate: self_handoff, capability_mismatch & direct ping_pong_loop guards") {
    let gate = HandoffGate.init(1) ?? return;

    // 1. Invalid target ID 0
    const d_inv = gate.requestHandoff(0, 0xFF, 0, 1, HandoffReason.load_shed, 100);
    expect_true(!d_inv.allowed);
    expect_eq(d_inv.kind, HandoffDecisionKind.invalid_agent_id);

    // 2. Self-handoff 1 -> 1
    const d_self = gate.requestHandoff(1, 0xFF, 0, 1, HandoffReason.load_shed, 200);
    expect_true(!d_self.allowed);
    expect_eq(d_self.kind, HandoffDecisionKind.self_handoff);

    // 3. Capability mismatch: requires 0b1101, target only has 0b0101 -> missing 0b1000
    const d_cap = gate.requestHandoff(2, 0b0101, 0b1101, 2, HandoffReason.specialist_escalation, 300);
    expect_true(!d_cap.allowed);
    expect_eq(d_cap.kind, HandoffDecisionKind.capability_mismatch);
    expect_eq(d_cap.missing_caps, @as(u64, 0b1000));
    expect_eq(gate.currentOwner(), @as(u32, 1)); // unchanged

    // 4. Commit 1 -> 2, then attempt immediate ping-pong 2 -> 1
    expect_true(gate.requestHandoff(2, 0b1111, 0b1101, 3, HandoffReason.specialist_escalation, 400).allowed);
    const d_pp = gate.requestHandoff(1, 0b1111, 0b0001, 4, HandoffReason.completion_return, 500);
    expect_true(!d_pp.allowed);
    expect_eq(d_pp.kind, HandoffDecisionKind.ping_pong_loop);
    expect_eq(gate.currentOwner(), @as(u32, 2));
}

test("HandoffGate: per-agent revisit_limit_exceeded across indirect 3-node cycles") {
    let cfg = HandoffGateConfig.defaults();
    cfg.max_revisits_per_agent = 2; // each agent may appear at most 2 times in active chain
    cfg.max_depth = 10;
    let gate = HandoffGate.initWithConfig(1, cfg) ?? return;

    // 1 -> 2 -> 3 -> 1 (not direct ping-pong; agent 1 now has 2 visits)
    expect_true(gate.requestHandoff(2, 1, 1, 1, HandoffReason.load_shed, 100).allowed);
    expect_true(gate.requestHandoff(3, 1, 1, 2, HandoffReason.load_shed, 200).allowed);
    const d_back = gate.requestHandoff(1, 1, 1, 3, HandoffReason.completion_return, 300);
    expect_true(d_back.allowed);
    expect_eq(d_back.target_visits, @as(u8, 2));

    // Continue 1 -> 2 (agent 2 now has 2 visits) -> 3 (agent 3 now has 2 visits)
    expect_true(gate.requestHandoff(2, 1, 1, 4, HandoffReason.load_shed, 400).allowed);
    expect_true(gate.requestHandoff(3, 1, 1, 5, HandoffReason.load_shed, 500).allowed);

    // Attempting 3 -> 1 again: agent 1 already has 2 visits -> revisit_limit_exceeded!
    const d_rev = gate.requestHandoff(1, 1, 1, 6, HandoffReason.load_shed, 600);
    expect_true(!d_rev.allowed);
    expect_eq(d_rev.kind, HandoffDecisionKind.revisit_limit_exceeded);
    expect_eq(d_rev.target_visits, @as(u8, 2));
    expect_eq(gate.currentOwner(), @as(u32, 3));
}

test("HandoffGate: max_depth_exceeded & 16-hop audit capacity guard") {
    // 1. Custom max_depth = 3
    let cfg3 = HandoffGateConfig.defaults();
    cfg3.max_depth = 3;
    let g3 = HandoffGate.initWithConfig(1, cfg3) ?? return;
    expect_true(g3.requestHandoff(2, 1, 0, 1, HandoffReason.specialist_escalation, 10).allowed);
    expect_true(g3.requestHandoff(3, 1, 0, 2, HandoffReason.specialist_escalation, 20).allowed);
    expect_true(g3.requestHandoff(4, 1, 0, 3, HandoffReason.specialist_escalation, 30).allowed);
    const d_over = g3.requestHandoff(5, 1, 0, 4, HandoffReason.specialist_escalation, 40);
    expect_true(!d_over.allowed);
    expect_eq(d_over.kind, HandoffDecisionKind.max_depth_exceeded);

    // 2. Full 16-hop capacity with rollbacks consuming history slots -> history_full
    let cfg16 = HandoffGateConfig.defaults();
    cfg16.max_depth = HANDOFF_GATE_MAX_HOPS;
    let g16 = HandoffGate.initWithConfig(100, cfg16) ?? return;
    let i: u32 = 1;
    while (i <= 16) : (i += 1) {
        expect_true(g16.requestHandoff(100 + i, 1, 0, @as(u64, i), HandoffReason.load_shed, @as(u64, i * 10)).allowed);
    }
    // Roll back 1 hop so active_depth == 15 (< max_depth 16), but hop_count == 16 -> history_full
    expect_true(g16.rollbackLast(999).rolled_back);
    expect_eq(g16.activeDepth(), @as(usize, 15));
    expect_eq(g16.historyLen(), HANDOFF_GATE_MAX_HOPS);
    const d_full = g16.requestHandoff(999, 1, 0, 99, HandoffReason.fallback_recovery, 1000);
    expect_true(!d_full.allowed);
    expect_eq(d_full.kind, HandoffDecisionKind.history_full);
}

test("HandoffGate: safe failure rollback restores previous owner & preserves audit history") {
    let gate = HandoffGate.init(1) ?? return;

    // Rollback at origin is a safe no-op
    const rb0 = gate.rollbackLast(50);
    expect_true(!rb0.rolled_back);
    expect_eq(rb0.restored_owner_agent_id, @as(u32, 1));

    // 1 -> 2 -> 9 (target #9 fails / rejects)
    expect_true(gate.requestHandoff(2, 0xFF, 0x01, 0xAA, HandoffReason.specialist_escalation, 100).allowed);
    expect_true(gate.requestHandoff(9, 0xFF, 0x01, 0xBB, HandoffReason.load_shed, 200).allowed);
    expect_eq(gate.currentOwner(), @as(u32, 9));
    expect_eq(gate.activeDepth(), @as(usize, 2));

    // Roll back #9 -> restores control to #2, preserves hop #2 as rolled_back
    const rb1 = gate.rollbackLast(250);
    expect_true(rb1.rolled_back);
    expect_eq(rb1.rolled_back_hop_seq, @as(u32, 2));
    expect_eq(rb1.reverted_from_agent_id, @as(u32, 9));
    expect_eq(rb1.restored_owner_agent_id, @as(u32, 2));
    expect_eq(rb1.active_depth, @as(usize, 1));
    expect_eq(gate.currentOwner(), @as(u32, 2));
    expect_eq(gate.visitCount(9), @as(u8, 0)); // rolled-back visit does not count as active

    if (h1 := gate.getHop(1)) {
        expect_eq(h1.state, HandoffHopState.rolled_back);
        expect_eq(h1.rollback_ts_ms, @as(u64, 250));
    } else {
        expect_true(false);
    }

    // Re-route from #2 to fallback agent #3 succeeds cleanly
    const d_fb = gate.requestHandoff(3, 0xFF, 0x01, 0xCC, HandoffReason.fallback_recovery, 300);
    expect_true(d_fb.allowed);
    expect_eq(gate.currentOwner(), @as(u32, 3));
    expect_eq(gate.activeDepth(), @as(usize, 2));
    expect_eq(gate.historyLen(), @as(usize, 3)); // 2 committed + 1 rolled_back audit preserved
}

test("HandoffGate: zero-alloc formatTrace, telemetry stats & cap.a2a facade parity") {
    let gate = cap.a2a.HandoffGate.init(1) ?? return;
    * = gate.requestHandoff(2, 0b11, 0b01, 0x1, cap.a2a.HandoffReason.specialist_escalation, 100);
    * = gate.requestHandoff(9, 0b11, 0b01, 0x2, cap.a2a.HandoffReason.load_shed, 200);
    * = gate.rollbackLast(250);
    * = gate.requestHandoff(3, 0b11, 0b01, 0x3, cap.a2a.HandoffReason.fallback_recovery, 300);
    * = gate.requestHandoff(3, 0b11, 0b01, 0x4, cap.a2a.HandoffReason.load_shed, 350); // self_handoff
    * = gate.requestHandoff(2, 0b11, 0b01, 0x5, cap.a2a.HandoffReason.load_shed, 360); // ping_pong_loop
    * = gate.requestHandoff(4, 0b00, 0b10, 0x6, cap.a2a.HandoffReason.load_shed, 370); // capability_mismatch

    let trace_buf: [512]u8 = [0; 512];
    let trace_s: []u8 = trace_buf[0..];
    const n = gate.formatTrace(*trace_s) ?? 0;
    expect_true(n > 0);
    expect_true(containsSubstr(trace_buf[0..n], "origin=#1 owner=#3 depth=2 hops=3 |"));
    expect_true(containsSubstr(trace_buf[0..n], "#1->#2(specialist_escalation,committed)"));
    expect_true(containsSubstr(trace_buf[0..n], "#2->#9(load_shed,rolled_back)"));
    expect_true(containsSubstr(trace_buf[0..n], "#2->#3(fallback_recovery,committed)"));

    // Undersized buffer returns null cleanly
    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(gate.formatTrace(*tiny_s) == null);

    const st = gate.stats();
    expect_eq(st.origin_agent_id, @as(u32, 1));
    expect_eq(st.current_owner_id, @as(u32, 3));
    expect_eq(st.active_depth, @as(usize, 2));
    expect_eq(st.total_hops_recorded, @as(usize, 3));
    expect_eq(st.capacity, HANDOFF_GATE_MAX_HOPS);
    expect_eq(st.allowed_count, @as(u64, 3));
    expect_eq(st.rolled_back_count, @as(u64, 1));
    expect_eq(st.blocked_self_count, @as(u64, 1));
    expect_eq(st.blocked_ping_pong_count, @as(u64, 1));
    expect_eq(st.blocked_capability_count, @as(u64, 1));
}
