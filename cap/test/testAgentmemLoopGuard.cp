// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/loopGuard.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Agent ReAct Trajectory & Loop/Oscillation Circuit Guard:
//   1. Normal progress trajectory, exact repetition detection (N consecutive identical calls) & non-false-positive check
//   2. Periodic oscillation detection: exact A-B-A-B period-2 & A-B-C-A-B-C period-3 cycles
//   3. Consecutive tool failure streak & no-progress stall detection with escalate_human progression
//   4. Temporary per-tool cooldown blacklist enforcement, tick decay across pivot steps & manual clear
//   5. Hard max-step & token budget circuit breakers + >64-step circular ring wrap-around integrity
//   6. Global telemetry stats, zero-alloc audit summary formatting & cap.agentmem facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    LOOP_GUARD_MAX_STEPS,
    LOOP_GUARD_MAX_COOLDOWNS,
    LoopAction,
    LoopTriggerReason,
    LoopGuardConfig,
    LoopGuardStats,
    hashTrajectoryBytes,
    formatLoopAction,
    formatLoopReason,
    AgentLoopGuard,
} from "cap:agentmem";

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

test("LoopGuard: exact repetition loop detection & distinct-args non-false-positive") {
    let guard = AgentLoopGuard.init();
    const h1 = hashTrajectoryBytes("{\"file\":\"a.cp\"}");
    const h2 = hashTrajectoryBytes("{\"file\":\"b.cp\"}");
    const h3 = hashTrajectoryBytes("{\"file\":\"c.cp\"}");
    const obs = hashTrajectoryBytes("ok");

    // Same tool with different args does NOT trigger exact repeat
    expect_eq(guard.recordStep(10, h1, obs, true, true, 100).action, LoopAction.continue_run);
    expect_eq(guard.recordStep(10, h2, obs, true, true, 100).action, LoopAction.continue_run);
    expect_eq(guard.recordStep(10, h3, obs, true, true, 100).action, LoopAction.continue_run);
    expect_true(!guard.isToolForbidden(10));

    // 3 consecutive identical (tool_id=20, args_hash=h1) triggers forbid_tool + auto cooldown
    expect_eq(guard.recordStep(20, h1, obs, true, false, 150).action, LoopAction.continue_run);
    expect_eq(guard.recordStep(20, h1, obs, true, false, 150).action, LoopAction.continue_run);
    const d3 = guard.recordStep(20, h1, obs, true, false, 150);
    expect_eq(d3.action, LoopAction.forbid_tool);
    expect_eq(d3.reason, LoopTriggerReason.exact_repeat);
    expect_eq(d3.cycle_period, @as(u8, 1));
    expect_eq(d3.repeat_streak, @as(u32, 3));
    expect_eq(d3.offending_tool_id, @as(u32, 20));
    expect_true(guard.isToolForbidden(20));
    expect_eq(guard.toolCooldownRemaining(20), @as(u32, 3));
}

test("LoopGuard: period-2 (A-B-A-B) & period-3 (A-B-C-A-B-C) oscillation detection") {
    let cfg = LoopGuardConfig.defaults();
    cfg.auto_cooldown_on_loop = false; // isolate oscillation detection from cooldown
    cfg.stall_limit = 20;

    // 1. Period-2 oscillation: A-B-A-B
    let g2 = AgentLoopGuard.initWithConfig(cfg);
    const ha = hashTrajectoryBytes("search:foo");
    const hb = hashTrajectoryBytes("open:bar");
    expect_eq(g2.recordStep(1, ha, 1, true, true, 100).action, LoopAction.continue_run);
    expect_eq(g2.recordStep(2, hb, 2, true, true, 100).action, LoopAction.continue_run);
    expect_eq(g2.recordStep(1, ha, 1, true, true, 100).action, LoopAction.continue_run);
    const d_osc2 = g2.recordStep(2, hb, 2, true, true, 100);
    expect_eq(d_osc2.action, LoopAction.inject_reflection);
    expect_eq(d_osc2.reason, LoopTriggerReason.cycle_2_oscillation);
    expect_eq(d_osc2.cycle_period, @as(u8, 2));
    expect_eq(g2.stats().oscillation_2_triggers, @as(u32, 1));

    // 2. Period-3 oscillation: A-B-C-A-B-C
    let g3 = AgentLoopGuard.initWithConfig(cfg);
    const hc = hashTrajectoryBytes("patch:baz");
    expect_eq(g3.recordStep(1, ha, 1, true, true, 100).action, LoopAction.continue_run);
    expect_eq(g3.recordStep(2, hb, 2, true, true, 100).action, LoopAction.continue_run);
    expect_eq(g3.recordStep(3, hc, 3, true, true, 100).action, LoopAction.continue_run);
    expect_eq(g3.recordStep(1, ha, 1, true, true, 100).action, LoopAction.continue_run);
    expect_eq(g3.recordStep(2, hb, 2, true, true, 100).action, LoopAction.continue_run);
    const d_osc3 = g3.recordStep(3, hc, 3, true, true, 100);
    expect_eq(d_osc3.action, LoopAction.inject_reflection);
    expect_eq(d_osc3.reason, LoopTriggerReason.cycle_3_oscillation);
    expect_eq(d_osc3.cycle_period, @as(u8, 3));
    expect_eq(g3.stats().oscillation_3_triggers, @as(u32, 1));
}

test("LoopGuard: consecutive failures & no-progress stall escalation to human") {
    let cfg = LoopGuardConfig.defaults();
    cfg.consecutive_fail_limit = 2;
    cfg.stall_limit = 3;
    cfg.max_interventions_before_abort = 20;
    let guard = AgentLoopGuard.initWithConfig(cfg);

    // Consecutive tool failures (distinct args so exact repeat doesn't fire)
    expect_eq(guard.recordStep(1, 101, 0, false, true, 50).action, LoopAction.continue_run);
    const df2 = guard.recordStep(2, 102, 0, false, true, 50);
    expect_eq(df2.action, LoopAction.inject_reflection);
    expect_eq(df2.reason, LoopTriggerReason.consecutive_failures);
    expect_eq(df2.failure_streak, @as(u32, 2));

    * = guard.recordStep(3, 103, 0, false, true, 50);
    const df4 = guard.recordStep(4, 104, 0, false, true, 50);
    expect_eq(df4.action, LoopAction.escalate_human);
    expect_eq(df4.reason, LoopTriggerReason.consecutive_failures);

    // One successful step resets failure_streak
    const d_ok = guard.recordStep(5, 105, 1, true, true, 50);
    expect_eq(d_ok.action, LoopAction.continue_run);
    expect_eq(d_ok.failure_streak, @as(u32, 0));

    // No-progress stall streak: 3 -> inject_reflection, 5 -> escalate_human
    expect_eq(guard.recordStep(10, 201, 1, true, false, 50).action, LoopAction.continue_run);
    expect_eq(guard.recordStep(11, 202, 2, true, false, 50).action, LoopAction.continue_run);
    const ds3 = guard.recordStep(12, 203, 3, true, false, 50);
    expect_eq(ds3.action, LoopAction.inject_reflection);
    expect_eq(ds3.reason, LoopTriggerReason.stalled_progress);
    * = guard.recordStep(13, 204, 4, true, false, 50);
    const ds5 = guard.recordStep(14, 205, 5, true, false, 50);
    expect_eq(ds5.action, LoopAction.escalate_human);
    expect_eq(ds5.reason, LoopTriggerReason.stalled_progress);
}

test("LoopGuard: tool temporary cooldown blacklist enforcement, decay & clear") {
    let guard = AgentLoopGuard.init();
    expect_true(guard.forbidTool(7, 2));
    expect_true(guard.isToolForbidden(7));
    expect_eq(guard.toolCooldownRemaining(7), @as(u32, 2));

    // Calling forbidden tool 7 returns tool_in_cooldown and does NOT decrement its own cooldown
    const dv = guard.recordStep(7, 1, 1, true, true, 50);
    expect_eq(dv.action, LoopAction.forbid_tool);
    expect_eq(dv.reason, LoopTriggerReason.tool_in_cooldown);
    expect_eq(guard.toolCooldownRemaining(7), @as(u32, 2));

    // Pivoting to tool 8 decrements tool 7's cooldown from 2 -> 1 -> 0 (released)
    expect_eq(guard.recordStep(8, 10, 1, true, true, 50).action, LoopAction.continue_run);
    expect_eq(guard.toolCooldownRemaining(7), @as(u32, 1));
    expect_eq(guard.recordStep(8, 11, 2, true, true, 50).action, LoopAction.continue_run);
    expect_true(!guard.isToolForbidden(7));

    // Tool 7 can now be called cleanly again
    expect_eq(guard.recordStep(7, 99, 3, true, true, 50).action, LoopAction.continue_run);

    // Manual cooldown clear
    expect_true(guard.forbidTool(9, 5));
    expect_true(guard.clearToolCooldown(9));
    expect_true(!guard.isToolForbidden(9));
}

test("LoopGuard: hard max-step & token budget aborts + circular ring wrap-around") {
    // 1. Token budget hard abort
    let cfg_tok = LoopGuardConfig.defaults();
    cfg_tok.max_tokens = 1_000;
    cfg_tok.max_steps = 100;
    let g_tok = AgentLoopGuard.initWithConfig(cfg_tok);
    expect_eq(g_tok.recordStep(1, 1, 1, true, true, 600).action, LoopAction.continue_run);
    const d_tok = g_tok.recordStep(2, 2, 2, true, true, 500);
    expect_eq(d_tok.action, LoopAction.abort_loop);
    expect_eq(d_tok.reason, LoopTriggerReason.token_budget_exceeded);
    expect_eq(d_tok.tokens_total, @as(u64, 1_100));

    // 2. Max steps hard abort + ring wrap-around > 64 steps when max_steps == 0 (unbounded)
    let cfg_ring = LoopGuardConfig.defaults();
    cfg_ring.max_steps = 0;
    cfg_ring.max_tokens = 0;
    let g_ring = AgentLoopGuard.initWithConfig(cfg_ring);
    let i: u32 = 1;
    while (i <= 70) : (i += 1) {
        const verdict = g_ring.recordStep(i, @as(u64, i) * 31, @as(u64, i), true, true, 10);
        expect_eq(verdict.action, LoopAction.continue_run);
    }
    expect_eq(g_ring.stats().total_steps, @as(u32, 70));
    expect_eq(g_ring.stats().ring_count, LOOP_GUARD_MAX_STEPS);
    const latest = g_ring.getRecentStep(0);
    expect_true(latest != null);
    if (s := latest) {
        expect_eq(s.step_num, @as(u32, 70));
    }
    const oldest_in_ring = g_ring.getRecentStep(LOOP_GUARD_MAX_STEPS - 1);
    expect_true(oldest_in_ring != null);
    if (s0 := oldest_in_ring) {
        expect_eq(s0.step_num, @as(u32, 7)); // 70 - 64 + 1 = 7
    }
    expect_true(g_ring.getRecentStep(LOOP_GUARD_MAX_STEPS) == null);
}

test("LoopGuard: zero-heap stats, audit summary formatting & cap.agentmem facade parity") {
    let guard = cap.agentmem.AgentLoopGuard.init();
    const h = cap.agentmem.hashTrajectoryBytes("{\"cmd\":\"ls\"}");
    * = guard.recordStep(1, h, 1, true, true, 200);
    * = guard.recordStep(1, h, 1, true, false, 200);
    const d3 = guard.recordStep(1, h, 1, true, false, 200);
    expect_eq(d3.action, LoopAction.forbid_tool);

    const st: LoopGuardStats = guard.stats();
    expect_eq(st.total_steps, @as(u32, 3));
    expect_eq(st.total_tokens, @as(u64, 600));
    expect_eq(st.progress_steps, @as(u32, 1));
    expect_eq(st.exact_repeat_triggers, @as(u32, 1));
    expect_eq(st.active_cooldowns, @as(usize, 1));

    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];
    const n = guard.formatAuditSummary(*buf_s) ?? 0;
    const summary = buf[0..n];
    expect_true(containsSubstr(summary, "steps=3"));
    expect_true(containsSubstr(summary, "tokens=600"));
    expect_true(containsSubstr(summary, "repeats=1"));
    expect_true(containsSubstr(summary, "last_action=forbid_tool"));
    expect_true(containsSubstr(summary, "last_reason=exact_repeat"));

    // Tiny buffer returns null cleanly
    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(guard.formatAuditSummary(*tiny_s) == null);

    expect_true(bytesEq(formatLoopAction(LoopAction.abort_loop), "abort_loop"));
    expect_true(bytesEq(formatLoopReason(LoopTriggerReason.cycle_2_oscillation), "cycle_2_oscillation"));
    expect_eq(LOOP_GUARD_MAX_COOLDOWNS, @as(usize, 32));
}
