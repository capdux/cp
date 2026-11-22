// Origin: Pure-CP Stdlib Test Suite (lib/sys/bulkhead.cp) | Created: 2026-11-23
// Deep production regression tests for cap:sys multi-tenant bulkhead & AIMD guard (`lib/sys/bulkhead.cp`):
//   1. Multi-tenant compartment isolation: saturating compartment A never affects compartment B
//   2. FIFO waiter promotion on `release()` with automatic skipping of expired head waiters
//   3. Explicit `reapExpiredWaiters()` & auto-reap on `tryAcquire()` preventing zombie queue saturation
//   4. AIMD additive increase on healthy streak + multiplicative decrease on high latency / errors
//   5. AIMD backoff defers waiter promotion while `in_flight >= current_limit`
//   6. Invalid config, unknown compartment, zero req_id, empty release & 32-compartment capacity guards
//   7. Per-compartment snapshots, global stats aggregation, `reset()`, and `cap.sys` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    BULKHEAD_MAX_COMPARTMENTS,
    BULKHEAD_MAX_QUEUE_PER_COMP,
    BulkheadAcquireStatus,
    BulkheadConfig,
    BulkheadGuard,
} from "cap:sys";

test("bulkhead: multi-tenant isolation keeps healthy compartment unaffected when another saturates") {
    let guard = BulkheadGuard.init();
    expect_true(guard.registerCompartment(1, BulkheadConfig.fixed(2, 2, 1000)));
    expect_true(guard.registerCompartment(2, BulkheadConfig.fixed(3, 1, 1000)));

    // Saturate compartment 1: 2 granted + 2 queued + 1 rejected_full
    expect_true(guard.tryAcquire(1, 101, 100).status == BulkheadAcquireStatus.granted);
    expect_true(guard.tryAcquire(1, 102, 100).status == BulkheadAcquireStatus.granted);
    const q1 = guard.tryAcquire(1, 103, 100);
    expect_true(q1.status == BulkheadAcquireStatus.queued);
    expect_eq(q1.queue_position, @as(usize, 1));
    const q2 = guard.tryAcquire(1, 104, 100);
    expect_true(q2.status == BulkheadAcquireStatus.queued);
    expect_eq(q2.queue_position, @as(usize, 2));
    expect_true(guard.tryAcquire(1, 105, 100).status == BulkheadAcquireStatus.rejected_full);

    // Compartment 2 remains 100% healthy and grants all 3 concurrent permits immediately
    expect_true(guard.tryAcquire(2, 201, 100).status == BulkheadAcquireStatus.granted);
    expect_true(guard.tryAcquire(2, 202, 100).status == BulkheadAcquireStatus.granted);
    expect_true(guard.tryAcquire(2, 203, 100).status == BulkheadAcquireStatus.granted);

    if (s1 := guard.compartmentSnapshot(1)) {
        expect_eq(s1.in_flight, @as(u32, 2));
        expect_eq(s1.queued, @as(usize, 2));
        expect_eq(s1.rejected, @as(u64, 1));
    } else {
        expect_true(false);
    }
    if (s2 := guard.compartmentSnapshot(2)) {
        expect_eq(s2.in_flight, @as(u32, 3));
        expect_eq(s2.rejected, @as(u64, 0));
    } else {
        expect_true(false);
    }
}

test("bulkhead: FIFO promotion skips expired head waiters and promotes first valid waiter") {
    let guard = BulkheadGuard.init();
    expect_true(guard.registerCompartment(10, BulkheadConfig.fixed(1, 4, 200)));

    expect_true(guard.tryAcquire(10, 1, 100).status == BulkheadAcquireStatus.granted);
    // Enqueue req 10 at t=100 (expires at 300), req 11 at t=350 (valid until 550), req 12 at t=360
    expect_true(guard.tryAcquire(10, 10, 100).status == BulkheadAcquireStatus.queued);
    expect_true(guard.tryAcquire(10, 11, 250).status == BulkheadAcquireStatus.queued);
    expect_true(guard.tryAcquire(10, 12, 260).status == BulkheadAcquireStatus.queued);

    // Release req 1 at t=320: req 10 (elapsed 220 >= 200) is expired & skipped; req 11 (elapsed 70) promoted!
    const rel = guard.release(10, 1, 50, false, 320);
    expect_true(rel.released);
    expect_eq(rel.expired_skipped, @as(usize, 1));
    expect_eq(rel.promoted_req_id, @as(u64, 11));
    expect_eq(rel.in_flight, @as(u32, 1));

    // Next release promotes req 12
    const rel2 = guard.release(10, 11, 40, false, 350);
    expect_eq(rel2.promoted_req_id, @as(u64, 12));

    if (snap := guard.compartmentSnapshot(10)) {
        expect_eq(snap.timed_out, @as(u64, 1));
        expect_eq(snap.promoted, @as(u64, 2));
        expect_eq(snap.queued, @as(usize, 0));
    } else {
        expect_true(false);
    }
}

test("bulkhead: reapExpiredWaiters clears stale queue slots and unblocks new traffic") {
    let guard = BulkheadGuard.init();
    expect_true(guard.registerCompartment(20, BulkheadConfig.fixed(1, 2, 100)));

    expect_true(guard.tryAcquire(20, 1, 1000).status == BulkheadAcquireStatus.granted);
    expect_true(guard.tryAcquire(20, 2, 1000).status == BulkheadAcquireStatus.queued);
    expect_true(guard.tryAcquire(20, 3, 1020).status == BulkheadAcquireStatus.queued);

    // At t=1105: req 2 (elapsed 105 >= 100) expired, req 3 (elapsed 85 < 100) still alive
    const reaped = guard.reapExpiredWaiters(20, 1105);
    expect_eq(reaped, @as(usize, 1));

    // Queue now has 1 free slot -> req 4 queues cleanly instead of rejected_full
    const q4 = guard.tryAcquire(20, 4, 1105);
    expect_true(q4.status == BulkheadAcquireStatus.queued);
    expect_eq(q4.queue_position, @as(usize, 2));
}

test("bulkhead: AIMD additive increase on healthy streak and multiplicative decrease on stress") {
    let guard = BulkheadGuard.init();
    // initial=4, min=2, hard_max=6, target_latency=100ms, backoff=750 permille, streak=4
    let cfg = BulkheadConfig.adaptiveDefaults(4, 2, 6, 100);
    cfg.healthy_streak_for_inc = 4;
    cfg.backoff_permille = 750;
    expect_true(guard.registerCompartment(30, cfg));

    // 4 consecutive healthy low-latency completions -> limit 4 -> 5
    let step: u64 = 1;
    while (step <= 4) {
        expect_true(guard.tryAcquire(30, step, 100).status == BulkheadAcquireStatus.granted);
        const rel = guard.release(30, step, 50, false, 150);
        if (step < 4) {
            expect_eq(rel.current_limit, @as(u32, 4));
        } else {
            expect_eq(rel.current_limit, @as(u32, 5));
        }
        step += 1;
    }

    // 1 high-latency completion (250ms > 100ms) -> multiplicative backoff: floor(5 * 0.75) = 3
    expect_true(guard.tryAcquire(30, 10, 200).status == BulkheadAcquireStatus.granted);
    const rel_slow = guard.release(30, 10, 250, false, 450);
    expect_eq(rel_slow.current_limit, @as(u32, 3));

    // 1 failed completion -> floor(3 * 0.75) = 2 (clamped at min_concurrent=2)
    expect_true(guard.tryAcquire(30, 11, 500).status == BulkheadAcquireStatus.granted);
    const rel_fail = guard.release(30, 11, 20, true, 520);
    expect_eq(rel_fail.current_limit, @as(u32, 2));

    // Another failure stays clamped at min_concurrent=2
    expect_true(guard.tryAcquire(30, 12, 600).status == BulkheadAcquireStatus.granted);
    const rel_floor = guard.release(30, 12, 20, true, 620);
    expect_eq(rel_floor.current_limit, @as(u32, 2));

    if (snap := guard.compartmentSnapshot(30)) {
        expect_eq(snap.aimd_increases, @as(u64, 1));
        expect_eq(snap.aimd_decreases, @as(u64, 2));
    } else {
        expect_true(false);
    }
}

test("bulkhead: AIMD backoff defers waiter promotion while in_flight still exceeds reduced limit") {
    let guard = BulkheadGuard.init();
    let cfg = BulkheadConfig.adaptiveDefaults(4, 2, 8, 100);
    cfg.backoff_permille = 500; // halve limit on failure: 4 -> 2
    expect_true(guard.registerCompartment(40, cfg));

    // Fill 4 in-flight + 1 queued waiter (req 5)
    expect_true(guard.tryAcquire(40, 1, 100).status == BulkheadAcquireStatus.granted);
    expect_true(guard.tryAcquire(40, 2, 100).status == BulkheadAcquireStatus.granted);
    expect_true(guard.tryAcquire(40, 3, 100).status == BulkheadAcquireStatus.granted);
    expect_true(guard.tryAcquire(40, 4, 100).status == BulkheadAcquireStatus.granted);
    expect_true(guard.tryAcquire(40, 5, 100).status == BulkheadAcquireStatus.queued);

    // Release req 1 with failure: in_flight drops 4 -> 3, limit drops 4 -> 2 => 3 >= 2, so req 5 NOT promoted yet!
    const r1 = guard.release(40, 1, 50, true, 150);
    expect_eq(r1.current_limit, @as(u32, 2));
    expect_eq(r1.in_flight, @as(u32, 3));
    expect_eq(r1.promoted_req_id, @as(u64, 0));

    // Release req 2 healthy: in_flight drops 3 -> 2 == limit 2 -> still not promoted
    const r2 = guard.release(40, 2, 40, false, 160);
    expect_eq(r2.in_flight, @as(u32, 2));
    expect_eq(r2.promoted_req_id, @as(u64, 0));

    // Release req 3 healthy: in_flight drops 2 -> 1 < limit 2 -> req 5 is now promoted (in_flight=2)!
    const r3 = guard.release(40, 3, 40, false, 170);
    expect_eq(r3.promoted_req_id, @as(u64, 5));
    expect_eq(r3.in_flight, @as(u32, 2));
}

test("bulkhead: invalid configs, unknown compartment, empty release & 32-compartment capacity guards") {
    let guard = BulkheadGuard.init();

    // Invalid configs
    expect_eq(guard.registerCompartment(0, BulkheadConfig.fixed(2, 2, 100)), false);
    expect_eq(guard.registerCompartment(1, BulkheadConfig.fixed(0, 2, 100)), false);
    expect_eq(guard.registerCompartment(1, BulkheadConfig.fixed(2, BULKHEAD_MAX_QUEUE_PER_COMP + 1, 100)), false);
    let bad_aimd = BulkheadConfig.adaptiveDefaults(4, 6, 8, 100); // min > initial
    expect_eq(guard.registerCompartment(1, bad_aimd), false);

    // Unknown compartment & zero req_id
    expect_true(guard.tryAcquire(999, 1, 0).status == BulkheadAcquireStatus.unknown_compartment);
    expect_true(guard.tryAcquire(1, 0, 0).status == BulkheadAcquireStatus.invalid_arg);
    expect_eq(guard.release(999, 1, 10, false, 0).released, false);

    // Fill all 32 compartments
    let cid: u32 = 1;
    while (cid <= @as(u32, BULKHEAD_MAX_COMPARTMENTS)) {
        expect_true(guard.registerCompartment(cid, BulkheadConfig.fixed(2, 2, 100)));
        cid += 1;
    }
    expect_eq(guard.registerCompartment(999, BulkheadConfig.fixed(2, 2, 100)), false);
    // Duplicate comp_id rejected
    expect_eq(guard.registerCompartment(1, BulkheadConfig.fixed(2, 2, 100)), false);
    // Release when in_flight == 0 returns released == false
    expect_eq(guard.release(1, 1, 10, false, 0).released, false);

    // Remove & reuse slot
    expect_true(guard.removeCompartment(15));
    expect_eq(guard.removeCompartment(15), false);
    expect_true(guard.registerCompartment(999, BulkheadConfig.fixed(2, 2, 100)));
}

test("bulkhead: global stats aggregation, reset, and cap.sys facade parity") {
    let guard = cap.sys.BulkheadGuard.init();
    expect_true(guard.registerCompartment(1, cap.sys.BulkheadConfig.fixed(1, 1, 1000)));
    expect_true(guard.registerCompartment(2, cap.sys.BulkheadConfig.fixed(1, 0, 1000)));

    * = guard.tryAcquire(1, 10, 100); // granted
    * = guard.tryAcquire(1, 11, 100); // queued
    * = guard.tryAcquire(1, 12, 100); // rejected
    * = guard.tryAcquire(2, 20, 100); // granted
    * = guard.tryAcquire(2, 21, 100); // rejected (max_queue=0)

    const st = guard.stats();
    expect_eq(st.capacity, BULKHEAD_MAX_COMPARTMENTS);
    expect_eq(st.active_compartments, @as(usize, 2));
    expect_eq(st.total_in_flight, @as(u64, 2));
    expect_eq(st.total_queued, @as(u64, 1));
    expect_eq(st.total_granted, @as(u64, 2));
    expect_eq(st.total_rejected, @as(u64, 2));

    guard.reset();
    const st2 = guard.stats();
    expect_eq(st2.active_compartments, @as(usize, 0));
    expect_eq(st2.total_granted, @as(u64, 0));
}
