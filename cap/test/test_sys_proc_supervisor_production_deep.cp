// Origin: Pure-CP Stdlib Test Suite (lib/sys/proc_supervisor.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Process / Sidecar Supervisor FSM:
//   1. Start -> pollNextAction(spawn) -> probe threshold -> healthy -> sustained uptime backoff reset
//   2. Exponential backoff doubling, max_backoff_ms cap & bounded deterministic jitter
//   3. Probe failure degradation -> graceful stop -> force_stop_after_grace timeout -> restart flow
//   4. Sliding-window quarantine isolation & manual resetQuarantine recovery
//   5. Restart policies (on_failure, always, never) & explicit requestGracefulStop
//   6. Monotonic clock rollback protection (no underflow or premature triggers)
//   7. Invalid config, unknown slot, invalid state & 32-slot capacity saturation guards
//   8. cap.sys namespace access & supervisor-wide stats() telemetry

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PROC_SUP_MAX_SLOTS,
    ProcSupState,
    ProcSupActionKind,
    ProcSupRestartPolicy,
    ProcSupStatus,
    ProcSupConfig,
    ProcSupervisor,
} from "cap:sys";

// ============================================================================
// Test 1: Startup -> Spawn -> Probe Healthy -> Sustained Uptime Backoff Reset
// ============================================================================

fn testStartupProbeHealthyAndBackoffReset()! {
    let sup = ProcSupervisor.init();
    const slot = sup.registerService(ProcSupConfig{
        service_id: 10,
        initial_backoff_ms: 1000,
        max_backoff_ms: 8000,
        jitter_pct: 0,
        reset_backoff_after_ms: 10000,
        probe_success_threshold: 2,
        probe_failure_threshold: 3,
    }) ?? return error.RegisterFailed;

    expect_eq(sup.startService(slot, 1000) == ProcSupStatus.ok, true);
    const a1 = sup.pollNextAction(1000);
    expect_eq(a1.kind == ProcSupActionKind.spawn, true);
    expect_eq(a1.slot_id, slot);
    expect_eq(a1.service_id, 10);

    // Simulate one failure exit so consecutive_failures == 1
    const st_fail = sup.recordProcessExit(slot, 1, 1500) ?? return error.MissingExit;
    expect_eq(st_fail == ProcSupState.backoff_waiting, true);
    expect_eq((sup.getSlot(slot) ?? return error.MissingSlot).consecutive_failures, 1);

    // Respawn after backoff (1500 + 1000 = 2500)
    expect_eq(sup.pollNextAction(2499).kind == ProcSupActionKind.none, true);
    expect_eq(sup.pollNextAction(2500).kind == ProcSupActionKind.spawn, true);

    // 2 successful probes transition starting -> healthy
    expect_eq((sup.recordProbeResult(slot, true, 3000) ?? ProcSupState.stopped) == ProcSupState.starting, true);
    expect_eq((sup.recordProbeResult(slot, true, 3500) ?? ProcSupState.stopped) == ProcSupState.healthy, true);

    // After sustained healthy uptime >= 10000ms (3500 + 10000 = 13500), backoff counter auto-resets to 0
    expect_eq((sup.recordProbeResult(slot, true, 13500) ?? ProcSupState.stopped) == ProcSupState.healthy, true);
    expect_eq((sup.getSlot(slot) ?? return error.MissingSlot2).consecutive_failures, 0);
}

// ============================================================================
// Test 2: Exponential Backoff Doubling, Cap & Bounded Deterministic Jitter
// ============================================================================

fn testExponentialBackoffAndJitter()! {
    let sup = ProcSupervisor.init();
    const s_exact = sup.registerService(ProcSupConfig{
        service_id: 21,
        initial_backoff_ms: 1000,
        max_backoff_ms: 3500,
        jitter_pct: 0,
        max_restarts_in_window: 8,
        restart_window_ms: 100000,
    }) ?? return error.RegisterExact;

    // 1st failure: 1000ms; 2nd: 2000ms; 3rd: capped at 3500ms
    expect_eq(sup.startService(s_exact, 0) == ProcSupStatus.ok, true);
    expect_eq(sup.pollNextAction(0).kind == ProcSupActionKind.spawn, true);
    * = sup.recordProcessExit(s_exact, 2, 100);
    expect_eq((sup.getSlot(s_exact) ?? return error.S1).next_restart_ms, 1100);

    expect_eq(sup.pollNextAction(1100).kind == ProcSupActionKind.spawn, true);
    * = sup.recordProcessExit(s_exact, 2, 1200);
    expect_eq((sup.getSlot(s_exact) ?? return error.S2).next_restart_ms, 3200);

    expect_eq(sup.pollNextAction(3200).kind == ProcSupActionKind.spawn, true);
    * = sup.recordProcessExit(s_exact, 2, 3300);
    expect_eq((sup.getSlot(s_exact) ?? return error.S3).next_restart_ms, 6800); // 3300 + 3500 cap

    // Deterministic jitter within [base, min(max, base * 1.2)]
    const s_jit = sup.registerService(ProcSupConfig{
        service_id: 22,
        initial_backoff_ms: 1000,
        max_backoff_ms: 5000,
        jitter_pct: 20,
        max_restarts_in_window: 8,
    }) ?? return error.RegisterJit;
    expect_eq(sup.startService(s_jit, 10000) == ProcSupStatus.ok, true);
    expect_eq(sup.pollNextAction(10000).kind == ProcSupActionKind.spawn, true);
    * = sup.recordProcessExit(s_jit, 1, 10100);
    const delay = (sup.getSlot(s_jit) ?? return error.SJ).next_restart_ms - 10100;
    expect_eq(delay >= 1000 && delay <= 1200, true);
}

// ============================================================================
// Test 3: Probe Failure -> Degraded -> Graceful Stop -> Force Stop Timeout
// ============================================================================

fn testProbeFailureGracefulAndForceStop()! {
    let sup = ProcSupervisor.init();
    const slot = sup.registerService(ProcSupConfig{
        service_id: 30,
        initial_backoff_ms: 500,
        max_backoff_ms: 4000,
        jitter_pct: 0,
        probe_success_threshold: 1,
        probe_failure_threshold: 2,
        graceful_stop_timeout_ms: 3000,
    }) ?? return error.Reg30;

    expect_eq(sup.startService(slot, 1000) == ProcSupStatus.ok, true);
    expect_eq(sup.pollNextAction(1000).kind == ProcSupActionKind.spawn, true);
    expect_eq((sup.recordProbeResult(slot, true, 1100) ?? ProcSupState.stopped) == ProcSupState.healthy, true);

    // 1st failed probe -> degraded
    expect_eq((sup.recordProbeResult(slot, false, 2000) ?? ProcSupState.stopped) == ProcSupState.degraded, true);
    // 2nd failed probe reaches threshold -> stopping
    expect_eq((sup.recordProbeResult(slot, false, 2500) ?? ProcSupState.stopped) == ProcSupState.stopping, true);

    // Immediate poll emits send_graceful_stop
    const a_stop = sup.pollNextAction(2500);
    expect_eq(a_stop.kind == ProcSupActionKind.send_graceful_stop, true);
    expect_eq(a_stop.slot_id, slot);

    // Before deadline (2500 + 3000 = 5500): no action
    expect_eq(sup.pollNextAction(5499).kind == ProcSupActionKind.none, true);
    // At deadline: force_stop_after_grace
    const a_force = sup.pollNextAction(5500);
    expect_eq(a_force.kind == ProcSupActionKind.force_stop_after_grace, true);

    // Once process exits, it schedules automatic restart (backoff_waiting)
    expect_eq((sup.recordProcessExit(slot, 137, 5600) ?? ProcSupState.stopped) == ProcSupState.backoff_waiting, true);
}

// ============================================================================
// Test 4: Sliding-Window Quarantine Isolation & Manual resetQuarantine
// ============================================================================

fn testQuarantineAndManualReset()! {
    let sup = ProcSupervisor.init();
    const slot = sup.registerService(ProcSupConfig{
        service_id: 40,
        initial_backoff_ms: 100,
        max_backoff_ms: 1000,
        jitter_pct: 0,
        max_restarts_in_window: 3,
        restart_window_ms: 10000,
    }) ?? return error.Reg40;

    expect_eq(sup.startService(slot, 1000) == ProcSupStatus.ok, true);
    // 1st failure
    expect_eq(sup.pollNextAction(1000).kind == ProcSupActionKind.spawn, true);
    expect_eq((sup.recordProcessExit(slot, 1, 1050) ?? ProcSupState.stopped) == ProcSupState.backoff_waiting, true);
    // 2nd failure
    expect_eq(sup.pollNextAction(1200).kind == ProcSupActionKind.spawn, true);
    expect_eq((sup.recordProcessExit(slot, 1, 1250) ?? ProcSupState.stopped) == ProcSupState.backoff_waiting, true);
    // 3rd failure inside 10s window -> quarantined!
    expect_eq(sup.pollNextAction(1500).kind == ProcSupActionKind.spawn, true);
    expect_eq((sup.recordProcessExit(slot, 1, 1550) ?? ProcSupState.stopped) == ProcSupState.quarantined, true);

    // Quarantined service will NOT auto-spawn even far in the future
    expect_eq(sup.pollNextAction(999999).kind == ProcSupActionKind.none, true);

    // Manual resetQuarantine clears window & failure streak and resumes immediately
    expect_eq(sup.resetQuarantine(slot, 20000) == ProcSupStatus.ok, true);
    expect_eq(sup.pollNextAction(20000).kind == ProcSupActionKind.spawn, true);
    expect_eq((sup.getSlot(slot) ?? return error.SQ).consecutive_failures, 0);
}

// ============================================================================
// Test 5: Restart Policies (on_failure, always, never) & Graceful Stop
// ============================================================================

fn testRestartPoliciesAndGracefulStop()! {
    let sup = ProcSupervisor.init();
    const s_onfail = sup.registerService(ProcSupConfig{ service_id: 51, restart_policy: ProcSupRestartPolicy.on_failure }) ?? return error.R1;
    const s_always = sup.registerService(ProcSupConfig{ service_id: 52, restart_policy: ProcSupRestartPolicy.always }) ?? return error.R2;
    const s_never = sup.registerService(ProcSupConfig{ service_id: 53, restart_policy: ProcSupRestartPolicy.never }) ?? return error.R3;

    * = sup.startService(s_onfail, 100);
    * = sup.startService(s_always, 100);
    * = sup.startService(s_never, 100);
    * = sup.pollNextAction(100);
    * = sup.pollNextAction(100);
    * = sup.pollNextAction(100);

    // Clean exit (code 0): on_failure -> stopped, always -> backoff_waiting
    expect_eq((sup.recordProcessExit(s_onfail, 0, 200) ?? ProcSupState.healthy) == ProcSupState.stopped, true);
    expect_eq((sup.recordProcessExit(s_always, 0, 200) ?? ProcSupState.stopped) == ProcSupState.backoff_waiting, true);
    // Failure exit (code 1) under never -> stopped
    expect_eq((sup.recordProcessExit(s_never, 1, 200) ?? ProcSupState.healthy) == ProcSupState.stopped, true);

    // Explicit requestGracefulStop on always-restarting service -> stopped after exit
    * = sup.pollNextAction(5000);
    expect_eq(sup.requestGracefulStop(s_always, 5100) == ProcSupStatus.ok, true);
    expect_eq(sup.pollNextAction(5100).kind == ProcSupActionKind.send_graceful_stop, true);
    expect_eq((sup.recordProcessExit(s_always, 0, 5200) ?? ProcSupState.healthy) == ProcSupState.stopped, true);
}

// ============================================================================
// Test 6: Monotonic Clock Rollback Protection
// ============================================================================

fn testClockRollbackProtection()! {
    let sup = ProcSupervisor.init();
    const slot = sup.registerService(ProcSupConfig{
        service_id: 60,
        initial_backoff_ms: 1000,
        jitter_pct: 0,
    }) ?? return error.Reg60;

    * = sup.startService(slot, 10000);
    expect_eq(sup.pollNextAction(10000).kind == ProcSupActionKind.spawn, true);
    * = sup.recordProcessExit(slot, 1, 10500);
    // Pass rolled-back timestamp (5000 < 10500): must clamp to 10500 and NOT trigger early spawn!
    expect_eq(sup.pollNextAction(5000).kind == ProcSupActionKind.none, true);
    expect_eq((sup.getSlot(slot) ?? return error.S60).last_observed_ms, 10500);
}

// ============================================================================
// Test 7: Invalid Config, Unknown Slot & 32-Slot Capacity Saturation Guards
// ============================================================================

fn testConfigSlotAndCapacityGuards()! {
    let sup = ProcSupervisor.init();

    // Invalid configs must be rejected
    expect_eq(sup.registerService(ProcSupConfig{ initial_backoff_ms: 0 }) == null, true);
    expect_eq(sup.registerService(ProcSupConfig{ initial_backoff_ms: 5000, max_backoff_ms: 1000 }) == null, true);
    expect_eq(sup.registerService(ProcSupConfig{ jitter_pct: 80 }) == null, true);
    expect_eq(sup.registerService(ProcSupConfig{ max_restarts_in_window: 0 }) == null, true);
    expect_eq(sup.registerService(ProcSupConfig{ probe_failure_threshold: 0 }) == null, true);

    // Unknown slot operations
    expect_eq(sup.startService(99, 0) == ProcSupStatus.slot_not_found, true);
    expect_eq(sup.requestGracefulStop(99, 0) == ProcSupStatus.slot_not_found, true);
    expect_eq(sup.resetQuarantine(99, 0) == ProcSupStatus.slot_not_found, true);
    expect_eq(sup.recordProbeResult(99, true, 0) == null, true);
    expect_eq(sup.recordProcessExit(99, 0, 0) == null, true);

    // Fill all 32 slots
    let i: usize = 0;
    while (i < PROC_SUP_MAX_SLOTS) : (i += 1) {
        const s = sup.registerService(ProcSupConfig{ service_id: (i as u32) + 1 });
        expect_eq(s != null, true);
    }
    // 33rd service rejected
    expect_eq(sup.registerService(ProcSupConfig{ service_id: 999 }) == null, true);
}

// ============================================================================
// Test 8: cap.sys Namespace & Supervisor Telemetry stats()
// ============================================================================

fn testNamespaceAndStats()! {
    let sup = cap.sys.ProcSupervisor.init();
    const s0 = sup.registerService(ProcSupConfig{ service_id: 1, probe_success_threshold: 1 }) ?? return error.S0;
    const s1 = sup.registerService(ProcSupConfig{ service_id: 2 }) ?? return error.S1;

    * = sup.startService(s0, 100);
    * = sup.pollNextAction(100);
    * = sup.recordProbeResult(s0, true, 200);

    const st = sup.stats();
    expect_eq(st.total_slots, 2);
    expect_eq(st.healthy_count, 1);
    expect_eq(st.stopped_count, 1);
    expect_eq(st.quarantined_count, 0);
    * = s1;
}

// ============================================================================
// Test Entrypoints
// ============================================================================

test("proc_supervisor: startup -> spawn -> probe healthy -> backoff reset", testStartupProbeHealthyAndBackoffReset);
test("proc_supervisor: exponential backoff doubling, cap & bounded jitter", testExponentialBackoffAndJitter);
test("proc_supervisor: probe failure -> degraded -> graceful stop -> force stop", testProbeFailureGracefulAndForceStop);
test("proc_supervisor: sliding-window quarantine & manual resetQuarantine", testQuarantineAndManualReset);
test("proc_supervisor: restart policies (on_failure, always, never) & graceful stop", testRestartPoliciesAndGracefulStop);
test("proc_supervisor: monotonic clock rollback protection", testClockRollbackProtection);
test("proc_supervisor: invalid config, unknown slot & 32-slot capacity guards", testConfigSlotAndCapacityGuards);
test("proc_supervisor: cap.sys namespace & supervisor stats() telemetry", testNamespaceAndStats);
