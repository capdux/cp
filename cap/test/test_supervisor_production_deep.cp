// Source: cp/cap/test/test_supervisor_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:supervisor & cap.supervisor

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SupervisorError,
    RestartStrategy,
    ChildRestartPolicy,
    ExitReason,
    ChildState,
    SupervisionDecisionKind,
    ChildSpec,
    SupervisorConfig,
    SupervisionDecision,
    SupervisorTree,
    BackoffKind,
    BackoffPolicy,
    Bulkhead,
    RetryBudget,
} from "cap:supervisor";

fn testChildRestartPoliciesAndErrors(): !void {
    const alloc = std.heap.page_allocator;

    // InvalidConfig: period_ms == 0
    const bad_cfg_res = SupervisorTree.init(alloc, SupervisorConfig{
        strategy: RestartStrategy.OneForOne,
        max_restarts: 3,
        period_ms: 0,
    });
    const bad_cfg_tag = switch (bad_cfg_res) {
        SupervisorError.InvalidConfig => "invalid_cfg",
        else => "other",
        ok => "ok",
    };
    expect_eq(bad_cfg_tag, "invalid_cfg");

    let tree = SupervisorTree.init(alloc, SupervisorConfig{
        strategy: RestartStrategy.OneForOne,
        max_restarts: 10,
        period_ms: 5000,
    })?;
    defer tree.deinit();

    // Empty ID rejected
    const empty_res = tree.addChild(ChildSpec{ id: "" });
    const empty_tag = switch (empty_res) {
        SupervisorError.InvalidConfig => "invalid",
        else => "other",
        ok => "ok",
    };
    expect_eq(empty_tag, "invalid");

    tree.addChild(ChildSpec{ id: "perm", restart_policy: ChildRestartPolicy.Permanent })?;
    tree.addChild(ChildSpec{ id: "trans_norm", restart_policy: ChildRestartPolicy.Transient })?;
    tree.addChild(ChildSpec{ id: "trans_shut", restart_policy: ChildRestartPolicy.Transient })?;
    tree.addChild(ChildSpec{ id: "trans_abn", restart_policy: ChildRestartPolicy.Transient })?;
    tree.addChild(ChildSpec{ id: "temp_norm", restart_policy: ChildRestartPolicy.Temporary })?;
    tree.addChild(ChildSpec{ id: "temp_shut", restart_policy: ChildRestartPolicy.Temporary })?;
    tree.addChild(ChildSpec{ id: "temp_abn", restart_policy: ChildRestartPolicy.Temporary })?;
    expect_eq(tree.runningCount(), 7 as usize);

    // Duplicate ID rejected
    const dup_res = tree.addChild(ChildSpec{ id: "perm" });
    const dup_tag = switch (dup_res) {
        SupervisorError.DuplicateChildId => "dup",
        else => "other",
        ok => "ok",
    };
    expect_eq(dup_tag, "dup");

    // 1. Temporary never restarts on Normal / Shutdown / Abnormal
    let d_tn = tree.handleChildExit("temp_norm", ExitReason.Normal, 100)?;
    defer d_tn.deinit();
    expect_eq(d_tn.kind, SupervisionDecisionKind.Ignore);
    expect_eq(d_tn.restart_order.len, 0 as usize);
    expect_eq(tree.childState("temp_norm").?, ChildState.Terminated);

    let d_ts = tree.handleChildExit("temp_shut", ExitReason.Shutdown, 110)?;
    defer d_ts.deinit();
    expect_eq(d_ts.kind, SupervisionDecisionKind.Ignore);
    expect_eq(tree.childState("temp_shut").?, ChildState.Terminated);

    let d_ta = tree.handleChildExit("temp_abn", ExitReason.Abnormal, 120)?;
    defer d_ta.deinit();
    expect_eq(d_ta.kind, SupervisionDecisionKind.Ignore);
    expect_eq(d_ta.window_restarts, 0 as u32);
    expect_eq(tree.childState("temp_abn").?, ChildState.Terminated);

    // 2. Transient restarts ONLY on Abnormal
    let d_trn = tree.handleChildExit("trans_norm", ExitReason.Normal, 130)?;
    defer d_trn.deinit();
    expect_eq(d_trn.kind, SupervisionDecisionKind.Ignore);
    expect_eq(tree.childState("trans_norm").?, ChildState.Terminated);

    let d_trs = tree.handleChildExit("trans_shut", ExitReason.Shutdown, 140)?;
    defer d_trs.deinit();
    expect_eq(d_trs.kind, SupervisionDecisionKind.Ignore);
    expect_eq(tree.childState("trans_shut").?, ChildState.Terminated);

    let d_tra = tree.handleChildExit("trans_abn", ExitReason.Abnormal, 150)?;
    defer d_tra.deinit();
    expect_eq(d_tra.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(d_tra.restart_order.len, 1 as usize);
    expect_eq(d_tra.restart_order[0], "trans_abn");
    expect_eq(d_tra.window_restarts, 1 as u32);
    expect_eq(tree.childState("trans_abn").?, ChildState.Running);

    // 3. Permanent ALWAYS restarts (Normal, Shutdown, Abnormal)
    let d_pn = tree.handleChildExit("perm", ExitReason.Normal, 160)?;
    defer d_pn.deinit();
    expect_eq(d_pn.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(d_pn.restart_order[0], "perm");

    let d_ps = tree.handleChildExit("perm", ExitReason.Shutdown, 170)?;
    defer d_ps.deinit();
    expect_eq(d_ps.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(d_ps.restart_order[0], "perm");

    let d_pa = tree.handleChildExit("perm", ExitReason.Abnormal, 180)?;
    defer d_pa.deinit();
    expect_eq(d_pa.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(d_pa.restart_order[0], "perm");
    expect_eq(d_pa.window_restarts, 4 as u32);
    expect_eq(tree.childRestartCount("perm").?, 3 as u64);
    expect_eq(tree.runningCount(), 2 as usize);

    // 4. removeChild & ChildNotFound
    tree.removeChild("temp_norm")?;
    expect_eq(tree.childState("temp_norm") == null, true);
    const miss_res = tree.removeChild("nonexistent");
    const miss_tag = switch (miss_res) {
        SupervisorError.ChildNotFound => "not_found",
        else => "other",
        ok => "ok",
    };
    expect_eq(miss_tag, "not_found");
}

fn testOtpTopologiesOrder(): !void {
    const alloc = std.heap.page_allocator;

    // 1. OneForOne: only failed child restarts, siblings untouched
    let t1 = SupervisorTree.init(alloc, SupervisorConfig{
        strategy: RestartStrategy.OneForOne,
        max_restarts: 5,
        period_ms: 5000,
    })?;
    defer t1.deinit();
    t1.addChild(ChildSpec{ id: "db", restart_policy: .Permanent })?;
    t1.addChild(ChildSpec{ id: "cache", restart_policy: .Permanent })?;
    t1.addChild(ChildSpec{ id: "api", restart_policy: .Permanent })?;

    let d1 = t1.handleChildExit("cache", ExitReason.Abnormal, 1000)?;
    defer d1.deinit();
    expect_eq(d1.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(d1.stop_order.len, 0 as usize);
    expect_eq(d1.restart_order.len, 1 as usize);
    expect_eq(d1.restart_order[0], "cache");
    expect_eq(t1.childRestartCount("db").?, 0 as u64);
    expect_eq(t1.childRestartCount("cache").?, 1 as u64);
    expect_eq(t1.childRestartCount("api").?, 0 as u64);

    // 2. OneForAll: reverse stop all active siblings, forward restart all non-Temporary
    let t2 = SupervisorTree.init(alloc, SupervisorConfig{
        strategy: RestartStrategy.OneForAll,
        max_restarts: 5,
        period_ms: 5000,
    })?;
    defer t2.deinit();
    t2.addChild(ChildSpec{ id: "db", restart_policy: .Permanent })?;
    t2.addChild(ChildSpec{ id: "temp_job", restart_policy: .Temporary })?;
    t2.addChild(ChildSpec{ id: "cache", restart_policy: .Transient })?;
    t2.addChild(ChildSpec{ id: "api", restart_policy: .Permanent })?;

    let d2 = t2.handleChildExit("cache", ExitReason.Abnormal, 1000)?;
    defer d2.deinit();
    expect_eq(d2.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(d2.stop_order.len, 3 as usize);
    expect_eq(d2.stop_order[0], "api");
    expect_eq(d2.stop_order[1], "temp_job");
    expect_eq(d2.stop_order[2], "db");
    expect_eq(d2.restart_order.len, 3 as usize);
    expect_eq(d2.restart_order[0], "db");
    expect_eq(d2.restart_order[1], "cache");
    expect_eq(d2.restart_order[2], "api");
    expect_eq(t2.childState("temp_job").?, ChildState.Terminated);
    expect_eq(t2.runningCount(), 3 as usize);

    // 3. RestForOne: reverse stop exited child's downstream siblings, forward restart from exited child
    let t3 = SupervisorTree.init(alloc, SupervisorConfig{
        strategy: RestartStrategy.RestForOne,
        max_restarts: 5,
        period_ms: 5000,
    })?;
    defer t3.deinit();
    t3.addChild(ChildSpec{ id: "db", restart_policy: .Permanent })?;
    t3.addChild(ChildSpec{ id: "cache", restart_policy: .Permanent })?;
    t3.addChild(ChildSpec{ id: "temp_sidecar", restart_policy: .Temporary })?;
    t3.addChild(ChildSpec{ id: "api", restart_policy: .Permanent })?;
    t3.addChild(ChildSpec{ id: "metrics", restart_policy: .Transient })?;

    let d3 = t3.handleChildExit("cache", ExitReason.Abnormal, 2000)?;
    defer d3.deinit();
    expect_eq(d3.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(d3.stop_order.len, 3 as usize);
    expect_eq(d3.stop_order[0], "metrics");
    expect_eq(d3.stop_order[1], "api");
    expect_eq(d3.stop_order[2], "temp_sidecar");
    expect_eq(d3.restart_order.len, 3 as usize);
    expect_eq(d3.restart_order[0], "cache");
    expect_eq(d3.restart_order[1], "api");
    expect_eq(d3.restart_order[2], "metrics");
    expect_eq(t3.childRestartCount("db").?, 0 as u64);
    expect_eq(t3.childRestartCount("cache").?, 1 as u64);
    expect_eq(t3.childRestartCount("api").?, 1 as u64);
    expect_eq(t3.childRestartCount("metrics").?, 1 as u64);
    expect_eq(t3.childState("temp_sidecar").?, ChildState.Terminated);
    expect_eq(t3.runningCount(), 4 as usize);
}

fn testSlidingWindowStormProtection(): !void {
    const alloc = std.heap.page_allocator;
    let tree = SupervisorTree.init(alloc, SupervisorConfig{
        strategy: RestartStrategy.OneForAll,
        max_restarts: 2,
        period_ms: 1000,
    })?;
    defer tree.deinit();

    tree.addChild(ChildSpec{ id: "a", restart_policy: .Permanent })?;
    tree.addChild(ChildSpec{ id: "b", restart_policy: .Permanent })?;
    tree.addChild(ChildSpec{ id: "c", restart_policy: .Permanent })?;

    // 2 restarts inside [1000, 1400]
    let r1 = tree.handleChildExit("a", ExitReason.Abnormal, 1000)?;
    defer r1.deinit();
    expect_eq(r1.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(r1.window_restarts, 1 as u32);

    let r2 = tree.handleChildExit("b", ExitReason.Abnormal, 1400)?;
    defer r2.deinit();
    expect_eq(r2.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(r2.window_restarts, 2 as u32);

    // At t = 2001, window is [1001, 2001]: the t = 1000 restart slid out!
    expect_eq(tree.windowRestartCount(2001), 1 as u32);
    let r3 = tree.handleChildExit("a", ExitReason.Abnormal, 2001)?;
    defer r3.deinit();
    expect_eq(r3.kind, SupervisionDecisionKind.RestartChildren);
    expect_eq(r3.window_restarts, 2 as u32);
    expect_eq(tree.isEscalated(), false);

    // At t = 2300, window [1300, 2300] already has 2 restarts (1400, 2001) -> 3rd triggers EscalateShutdown!
    let r4 = tree.handleChildExit("b", ExitReason.Abnormal, 2300)?;
    defer r4.deinit();
    expect_eq(r4.kind, SupervisionDecisionKind.EscalateShutdown);
    expect_eq(r4.window_restarts, 3 as u32);
    expect_eq(r4.restart_order.len, 0 as usize);
    expect_eq(r4.stop_order.len, 2 as usize);
    expect_eq(r4.stop_order[0], "c");
    expect_eq(r4.stop_order[1], "a");
    expect_eq(tree.isEscalated(), true);
    expect_eq(tree.runningCount(), 0 as usize);
    expect_eq(tree.childState("a").?, ChildState.Stopped);
    expect_eq(tree.childState("b").?, ChildState.Terminated);
    expect_eq(tree.childState("c").?, ChildState.Stopped);

    // Further exits after escalation return MaxRestartsExceeded
    const after_res = tree.handleChildExit("a", ExitReason.Abnormal, 2400);
    const after_tag = switch (after_res) {
        SupervisorError.MaxRestartsExceeded => "escalated",
        else => "other",
        ok => "ok",
    };
    expect_eq(after_tag, "escalated");
}

fn testResiliencePrimitives(): !void {
    // 1. BackoffPolicy: 5 strategies + max_ms cap + deterministic jitter
    let b_fixed = BackoffPolicy.init(BackoffKind.Fixed, 100, 250, 200, 1);
    expect_eq(b_fixed.nextDelayMs(0, 0), 100 as u64);
    expect_eq(b_fixed.nextDelayMs(5, 100), 100 as u64);

    let b_linear = BackoffPolicy.init(BackoffKind.Linear, 100, 350, 200, 1);
    expect_eq(b_linear.nextDelayMs(0, 0), 100 as u64);
    expect_eq(b_linear.nextDelayMs(1, 100), 200 as u64);
    expect_eq(b_linear.nextDelayMs(2, 200), 300 as u64);
    expect_eq(b_linear.nextDelayMs(3, 300), 350 as u64);

    let b_exp = BackoffPolicy.init(BackoffKind.Exponential, 100, 500, 200, 1);
    expect_eq(b_exp.nextDelayMs(0, 0), 100 as u64);
    expect_eq(b_exp.nextDelayMs(1, 100), 200 as u64);
    expect_eq(b_exp.nextDelayMs(2, 200), 400 as u64);
    expect_eq(b_exp.nextDelayMs(3, 400), 500 as u64);
    expect_eq(b_exp.nextDelayMs(30, 500), 500 as u64);

    let b_fj1 = BackoffPolicy.init(BackoffKind.FullJitter, 100, 1000, 200, 42);
    let b_fj2 = BackoffPolicy.init(BackoffKind.FullJitter, 100, 1000, 200, 42);
    const fj_a0 = b_fj1.nextDelayMs(2, 0);
    const fj_b0 = b_fj2.nextDelayMs(2, 0);
    expect_eq(fj_a0, fj_b0);
    expect_eq(fj_a0 <= 400, true);
    const fj_a1 = b_fj1.nextDelayMs(3, fj_a0);
    b_fj1.resetSeed(42);
    expect_eq(b_fj1.nextDelayMs(2, 0), fj_a0);
    expect_eq(b_fj1.nextDelayMs(3, fj_a0), fj_a1);

    let b_dj = BackoffPolicy.init(BackoffKind.DecorrelatedJitter, 100, 600, 200, 99);
    let prev: u64 = 100;
    let step: u32 = 0;
    while (step < 6) : (step += 1) {
        const d = b_dj.nextDelayMs(step, prev);
        expect_eq(d >= 100 && d <= 600, true);
        prev = d;
    }

    // 2. Bulkhead: saturation rejection, queue promotion, releaseSlot, peakActive
    let bh = Bulkhead.init(2, 1);
    expect_eq(bh.availableSlots(), 2 as u32);
    expect_eq(bh.tryAcquire(), true);
    expect_eq(bh.tryAcquire(), true);
    expect_eq(bh.activeCount(), 2 as u32);
    expect_eq(bh.availableSlots(), 0 as u32);
    expect_eq(bh.peakActive(), 2 as u32);

    expect_eq(bh.tryAcquire(), false);
    expect_eq(bh.rejectedCount(), 1 as u64);

    const acq_res = bh.acquire();
    const acq_tag = switch (acq_res) {
        SupervisorError.BulkheadFull => "full",
        else => "other",
        ok => "ok",
    };
    expect_eq(acq_tag, "full");
    expect_eq(bh.rejectedCount(), 2 as u64);

    expect_eq(bh.tryEnqueue(), true);
    expect_eq(bh.queuedCount(), 1 as u32);
    expect_eq(bh.tryEnqueue(), false);
    expect_eq(bh.rejectedCount(), 3 as u64);

    bh.releaseSlot(); // promotes queued waiter
    expect_eq(bh.activeCount(), 2 as u32);
    expect_eq(bh.queuedCount(), 0 as u32);

    bh.releaseSlot();
    expect_eq(bh.activeCount(), 1 as u32);
    expect_eq(bh.availableSlots(), 1 as u32);
    expect_eq(bh.peakActive(), 2 as u32);

    // 3. RetryBudget: low-volume baseline & high-volume ratio cap against retry storms
    let budget = RetryBudget.init(1000, 2, 2000); // 1000ms window, min 2 retries, 20% ratio
    // Low volume (0 requests): 2 baseline retries allowed, 3rd blocked
    expect_eq(budget.tryConsumeRetry(100), true);
    expect_eq(budget.tryConsumeRetry(150), true);
    expect_eq(budget.tryConsumeRetry(200), false);
    expect_eq(budget.rejectedRetries(), 1 as u64);

    // Advance past 1000ms window -> previous retries expire
    expect_eq(budget.windowRetries(2000), 0 as u64);

    // High volume: 20 requests -> 20% = 4 retries allowed
    let req_i: usize = 0;
    while (req_i < 20) : (req_i += 1) {
        budget.recordRequest(2000);
    }
    expect_eq(budget.windowRequests(2000), 20 as u64);
    expect_eq(budget.remainingRetries(2000), 4 as u64);
    expect_eq(budget.tryConsumeRetry(2010), true);
    expect_eq(budget.tryConsumeRetry(2020), true);
    expect_eq(budget.tryConsumeRetry(2030), true);
    expect_eq(budget.tryConsumeRetry(2040), true);
    expect_eq(budget.currentRatioBps(2040), 2000 as u32);
    // 5th retry exceeds 20% ratio -> rejected
    expect_eq(budget.tryConsumeRetry(2050), false);
    expect_eq(budget.rejectedRetries(), 2 as u64);

    // Slide past window again -> clean reset
    expect_eq(budget.windowRequests(3100), 0 as u64);
    expect_eq(budget.currentRatioBps(3100), 0 as u32);
}

fn testCapSupervisorDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    let tree = cap.supervisor.SupervisorTree.init(alloc, cap.supervisor.SupervisorConfig{
        strategy: cap.supervisor.RestartStrategy.RestForOne,
        max_restarts: 2,
        period_ms: 3000,
    })?;
    defer tree.deinit();

    tree.addChild(cap.supervisor.ChildSpec{ id: "router", restart_policy: .Permanent })?;
    tree.addChild(cap.supervisor.ChildSpec{ id: "agent", restart_policy: .Permanent })?;

    let decision = tree.handleChildExit("router", cap.supervisor.ExitReason.Abnormal, 500)?;
    defer decision.deinit();
    expect_eq(decision.kind, cap.supervisor.SupervisionDecisionKind.RestartChildren);
    expect_eq(decision.stop_order.len, 1 as usize);
    expect_eq(decision.stop_order[0], "agent");
    expect_eq(decision.restart_order.len, 2 as usize);
    expect_eq(decision.restart_order[0], "router");
    expect_eq(decision.restart_order[1], "agent");

    let bh = cap.supervisor.Bulkhead.init(1, 0);
    expect_eq(bh.tryAcquire(), true);
    expect_eq(bh.tryAcquire(), false);
    bh.releaseSlot();
    expect_eq(bh.availableSlots(), 1 as u32);

    let pol = cap.supervisor.BackoffPolicy.init(cap.supervisor.BackoffKind.Exponential, 50, 200, 200, 7);
    expect_eq(pol.nextDelayMs(1, 0), 100 as u64);

    let rb = cap.supervisor.RetryBudget.init(1000, 1, 1000);
    rb.recordRequest(100);
    expect_eq(rb.tryConsumeRetry(110), true);
    expect_eq(rb.tryConsumeRetry(120), false);
}

fn main() {
    test("cap:supervisor Permanent/Transient/Temporary policies & error guards", () => {
        testChildRestartPoliciesAndErrors()?;
    });

    test("cap:supervisor OneForOne/OneForAll/RestForOne stop & restart topology order", () => {
        testOtpTopologiesOrder()?;
    });

    test("cap:supervisor sliding-window max_restarts storm protection & EscalateShutdown", () => {
        testSlidingWindowStormProtection()?;
    });

    test("cap:supervisor BackoffPolicy, Bulkhead & RetryBudget resilience primitives", () => {
        testResiliencePrimitives()?;
    });

    test("cap.supervisor lazy dynamic hub mount via import cap from \"cap\"", () => {
        testCapSupervisorDynamicMount()?;
    });
}
