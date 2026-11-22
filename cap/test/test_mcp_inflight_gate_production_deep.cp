// Origin: Pure-CP Stdlib Test Suite (lib/mcp/inflight_gate.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc MCP In-Flight Concurrency Gate:
//   1. Normal admit/complete lifecycle & concurrency counter conservation
//   2. Per-method cap isolation (tools_call full while resources_read admits) vs global total cap
//   3. Active duplicate req_id rejection & post-completion ID reuse
//   4. Idempotent request_cancel & batch-wide cancel_batch isolation
//   5. sweep_expired exact deadline reclamation & concurrency quota restoration
//   6. Batched small-buffer sweep_expired (non-lossy drain) & empty output slice safety
//   7. Invalid McpGateLimits & req_id == 0 / timeout_ms == 0 boundary guards

import { test, expect_eq } from "cap:test";
import {
    MCP_GATE_MAX_INFLIGHT,
    McpMethodClass,
    McpGateLimits,
    McpAdmitOutcome,
    McpGateStats,
    McpInflightGate,
} from "cap:mcp";

// ============================================================================
// Test 1: Normal Admit/Complete Lifecycle & Counter Conservation
// ============================================================================
test("inflight_gate: admit, complete & concurrency counter conservation", () => {
    expect_eq(MCP_GATE_MAX_INFLIGHT, 64);

    let gate = McpInflightGate.init(McpGateLimits.defaults()) ?? return;

    const a1: McpAdmitOutcome = gate.admit_request(1, 10, McpMethodClass.tools_call, 5_000, 1_000);
    const a2: McpAdmitOutcome = gate.admit_request(2, 10, McpMethodClass.resources_read, 5_000, 1_000);
    expect_eq(a1.admitted, true);
    expect_eq(a2.admitted, true);

    const st1: McpGateStats = gate.stats();
    expect_eq(st1.active_total, 2);
    expect_eq(st1.active_tools_call, 1);
    expect_eq(st1.total_admitted, 2);
    expect_eq(st1.total_completed, 0);

    expect_eq(gate.complete_request(1), true);
    expect_eq(gate.complete_request(1), false); // already completed
    expect_eq(gate.complete_request(999), false); // unknown ID

    const st2 = gate.stats();
    expect_eq(st2.active_total, 1);
    expect_eq(st2.active_tools_call, 0);
    expect_eq(st2.total_completed, 1);

    expect_eq(gate.complete_request(2), true);
    expect_eq(gate.stats().active_total, 0);
    expect_eq(gate.stats().total_completed, 2);
});

// ============================================================================
// Test 2: Per-Method Cap Isolation vs Global Total Concurrency Cap
// ============================================================================
test("inflight_gate: per-method cap isolation & global total cap enforcement", () => {
    const limits = McpGateLimits{
        max_total_inflight: 4,
        max_tools_call: 2,
        max_resources_read: 3,
        max_prompts_get: 2,
        max_sampling_create: 1,
    };
    let gate = McpInflightGate.init(limits) ?? return;

    // Fill tools_call cap (2/2)
    expect_eq(gate.admit_request(10, 1, McpMethodClass.tools_call, 5_000, 100).admitted, true);
    expect_eq(gate.admit_request(11, 1, McpMethodClass.tools_call, 5_000, 100).admitted, true);

    // 3rd tools_call rejected by per-method cap, NOT total cap
    const r_tool = gate.admit_request(12, 1, McpMethodClass.tools_call, 5_000, 100);
    expect_eq(r_tool.admitted, false);
    expect_eq(r_tool.rejected_method_cap, true);
    expect_eq(r_tool.rejected_total_cap, false);

    // resources_read & prompts_get still admitted because total is 2 < 4
    expect_eq(gate.admit_request(20, 1, McpMethodClass.resources_read, 5_000, 100).admitted, true);
    expect_eq(gate.admit_request(30, 1, McpMethodClass.prompts_get, 5_000, 100).admitted, true);

    // Now total is 4/4 -> even other method classes rejected by total cap
    const r_total = gate.admit_request(40, 1, McpMethodClass.other, 5_000, 100);
    expect_eq(r_total.admitted, false);
    expect_eq(r_total.rejected_total_cap, true);
    expect_eq(r_total.rejected_method_cap, false);

    expect_eq(gate.stats().active_total, 4);
    expect_eq(gate.stats().active_tools_call, 2);
    expect_eq(gate.stats().total_rejected, 2);
});

// ============================================================================
// Test 3: Duplicate Active req_id Rejection & Post-Completion ID Reuse
// ============================================================================
test("inflight_gate: duplicate active req_id rejection & post-completion reuse", () => {
    let gate = McpInflightGate.init(McpGateLimits.defaults()) ?? return;

    expect_eq(gate.admit_request(42, 1, McpMethodClass.tools_call, 5_000, 100).admitted, true);

    // Duplicate while active -> rejected_duplicate_id
    const dup = gate.admit_request(42, 2, McpMethodClass.resources_read, 5_000, 110);
    expect_eq(dup.admitted, false);
    expect_eq(dup.rejected_duplicate_id, true);
    expect_eq(gate.stats().active_total, 1);

    // Complete 42, then reuse req_id == 42 cleanly
    expect_eq(gate.complete_request(42), true);
    const reuse = gate.admit_request(42, 2, McpMethodClass.resources_read, 5_000, 120);
    expect_eq(reuse.admitted, true);
    expect_eq(gate.stats().active_total, 1);
});

// ============================================================================
// Test 4: Idempotent request_cancel & Batch-Wide cancel_batch
// ============================================================================
test("inflight_gate: idempotent request_cancel & batch cancel_batch isolation", () => {
    let gate = McpInflightGate.init(McpGateLimits.defaults()) ?? return;

    // Batch 100: reqs 1, 2, 3; Batch 200: req 4
    expect_eq(gate.admit_request(1, 100, McpMethodClass.tools_call, 5_000, 100).admitted, true);
    expect_eq(gate.admit_request(2, 100, McpMethodClass.tools_call, 5_000, 100).admitted, true);
    expect_eq(gate.admit_request(3, 100, McpMethodClass.resources_read, 5_000, 100).admitted, true);
    expect_eq(gate.admit_request(4, 200, McpMethodClass.tools_call, 5_000, 100).admitted, true);

    // Single cancel on req 1 (second call is idempotent, does not double-count)
    expect_eq(gate.is_canceled(1), false);
    expect_eq(gate.request_cancel(1), true);
    expect_eq(gate.request_cancel(1), true);
    expect_eq(gate.is_canceled(1), true);
    expect_eq(gate.stats().active_cancel_pending, 1);
    expect_eq(gate.stats().total_canceled, 1);

    // cancel_batch(100) cancels only remaining running reqs 2 and 3 (returns 2), spares batch 200 (req 4)
    expect_eq(gate.cancel_batch(100), 2);
    expect_eq(gate.is_canceled(2), true);
    expect_eq(gate.is_canceled(3), true);
    expect_eq(gate.is_canceled(4), false);
    expect_eq(gate.stats().active_cancel_pending, 3);
    expect_eq(gate.stats().total_canceled, 3);

    // Completing canceled req 1 decrements active_cancel_pending
    expect_eq(gate.complete_request(1), true);
    expect_eq(gate.stats().active_cancel_pending, 2);

    // Unknown req_id and batch_id == 0 guards
    expect_eq(gate.request_cancel(999), false);
    expect_eq(gate.cancel_batch(0), 0);
});

// ============================================================================
// Test 5: sweep_expired Exact Deadline & Quota Reclamation
// ============================================================================
test("inflight_gate: sweep_expired reclaims timed-out requests and restores quotas", () => {
    const limits = McpGateLimits{
        max_total_inflight: 4,
        max_tools_call: 2,
        max_resources_read: 2,
        max_prompts_get: 2,
        max_sampling_create: 1,
    };
    let gate = McpInflightGate.init(limits) ?? return;

    // req 10: deadline 2000; req 11: deadline 3000
    expect_eq(gate.admit_request(10, 1, McpMethodClass.tools_call, 1_000, 1_000).admitted, true);
    expect_eq(gate.admit_request(11, 1, McpMethodClass.tools_call, 2_000, 1_000).admitted, true);
    expect_eq(gate.admit_request(12, 1, McpMethodClass.tools_call, 1_000, 1_000).rejected_method_cap, true);

    let expired: [4]u64 = [0; 4];
    let exp_s: []u64 = expired[0..];

    // At t=1999: neither expired
    expect_eq(gate.sweep_expired(1_999, exp_s), 0);

    // At t=2000: req 10 expired (now_ms >= deadline_ms), req 11 still active
    expect_eq(gate.sweep_expired(2_000, exp_s), 1);
    expect_eq(expired[0], 10);
    expect_eq(gate.stats().active_total, 1);
    expect_eq(gate.stats().active_tools_call, 1);
    expect_eq(gate.stats().total_timed_out, 1);

    // Reclaimed tools_call quota immediately allows admitting req 12
    expect_eq(gate.admit_request(12, 1, McpMethodClass.tools_call, 5_000, 2_000).admitted, true);
    expect_eq(gate.stats().active_tools_call, 2);
});

// ============================================================================
// Test 6: Batched Small-Buffer sweep_expired (Non-Lossy Drain)
// ============================================================================
test("inflight_gate: small output buffer batched sweep_expired loses zero IDs", () => {
    let gate = McpInflightGate.init(McpGateLimits.defaults()) ?? return;

    // Admit 5 requests all expiring at t=2000
    let id: u64 = 101;
    while (id <= 105) : (id += 1) {
        expect_eq(gate.admit_request(id, 1, McpMethodClass.other, 1_000, 1_000).admitted, true);
    }

    // Empty output slice sweeps 0 and preserves all 5 slots
    let empty_buf: [0]u64 = [];
    let empty_s: []u64 = empty_buf[0..];
    expect_eq(gate.sweep_expired(3_000, empty_s), 0);
    expect_eq(gate.stats().active_total, 5);

    // Small 2-item buffer drains all 5 across 3 calls (2 + 2 + 1)
    let small: [2]u64 = [0; 2];
    let small_s: []u64 = small[0..];
    const n1 = gate.sweep_expired(3_000, small_s);
    const n2 = gate.sweep_expired(3_000, small_s);
    const n3 = gate.sweep_expired(3_000, small_s);
    const n4 = gate.sweep_expired(3_000, small_s);

    expect_eq(n1, 2);
    expect_eq(n2, 2);
    expect_eq(n3, 1);
    expect_eq(n4, 0);
    expect_eq(gate.stats().active_total, 0);
    expect_eq(gate.stats().total_timed_out, 5);
});

// ============================================================================
// Test 7: Invalid Limits & req_id == 0 / timeout_ms == 0 Boundary Guards
// ============================================================================
test("inflight_gate: invalid limits and zero req_id/timeout boundary guards", () => {
    let bad_zero = McpGateLimits.defaults();
    bad_zero.max_total_inflight = 0;
    expect_eq(bad_zero.isValid(), false);
    expect_eq(McpInflightGate.init(bad_zero) == null, true);

    let bad_over = McpGateLimits.defaults();
    bad_over.max_total_inflight = MCP_GATE_MAX_INFLIGHT + 1;
    expect_eq(McpInflightGate.init(bad_over) == null, true);

    let bad_method = McpGateLimits.defaults();
    bad_method.max_total_inflight = 8;
    bad_method.max_tools_call = 9; // exceeds max_total_inflight
    expect_eq(McpInflightGate.init(bad_method) == null, true);

    let gate = McpInflightGate.init(McpGateLimits.defaults()) ?? return;
    expect_eq(gate.admit_request(0, 1, McpMethodClass.tools_call, 1_000, 100).admitted, false);
    expect_eq(gate.admit_request(1, 1, McpMethodClass.tools_call, 0, 100).admitted, false);
    expect_eq(gate.stats().active_total, 0);
    expect_eq(gate.stats().total_rejected, 2);
});
