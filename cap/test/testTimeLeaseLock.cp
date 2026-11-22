// Origin: Pure-CP Stdlib Test Suite (lib/time/leaseLock.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Distributed Lease Lock Table & Monotonic Fencing-Token Manager:
//   1. Acquire, same-owner reentrant renew, contention (already_held + remaining_ttl_ms) & monotonic fencing tokens
//   2. Strict renew & release authorization checks (owner_mismatch, token_mismatch, expired, not_found)
//   3. Storage-layer split-brain write gate (validateFencingToken rejects stale zombie tokens even after release)
//   4. Batch sweepExpired reclamation & aggregate stats verification
//   5. Monotonic clock rollback guard (prevents zombie lease extension across backward clock jumps)
//   6. 64-slot full-table protection & automatic sweep-and-retry when expired slots become reclaimable
//   7. Boundary & u64 overflow guards (ttl==0, owner==0, saturating now+ttl) & cap.time dynamic facade

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    LEASE_LOCK_MAX_LOCKS,
    LeaseAcquireStatus,
    LeaseOpStatus,
    FencingCheckStatus,
    LeaseLockManager,
} from "cap:time";

// ============================================================================
// Test 1: Acquire, Contention, Reentrant Renew & Monotonic Fencing Tokens
// ============================================================================
test("leaseLock: acquire, contention remaining TTL, reentrant renew & monotonic fencing tokens", () => {
    let mgr = LeaseLockManager.init();

    // Owner 10 acquires resource 100 at t=1000 for 5000ms (expires at 6000)
    const a1 = mgr.acquire(100, 10, 5_000, 1_000);
    expect_eq(a1.status, LeaseAcquireStatus.acquired);
    expect_eq(a1.fencing_token, 1 as u64);
    expect_eq(a1.acquired_ms, 1_000 as u64);
    expect_eq(a1.expires_ms, 6_000 as u64);
    expect_eq(a1.remaining_ttl_ms, 5_000 as u64);

    // Owner 20 contends at t=2500 -> already_held with remaining_ttl_ms = 3500
    const a2 = mgr.acquire(100, 20, 5_000, 2_500);
    expect_eq(a2.status, LeaseAcquireStatus.already_held);
    expect_eq(a2.owner_id, 10 as u64);
    expect_eq(a2.remaining_ttl_ms, 3_500 as u64);

    // Owner 10 re-enters at t=3000 with ttl=5000 -> reentered_renewed, expires at 8000, same token 1
    const a3 = mgr.acquire(100, 10, 5_000, 3_000);
    expect_eq(a3.status, LeaseAcquireStatus.reentered_renewed);
    expect_eq(a3.fencing_token, 1 as u64);
    expect_eq(a3.expires_ms, 8_000 as u64);
    expect_eq(a3.renew_count, 1 as u32);

    // After expiry at t=8000, Owner 20 acquires with strictly higher fencing_token = 2
    const a4 = mgr.acquire(100, 20, 4_000, 8_000);
    expect_eq(a4.status, LeaseAcquireStatus.acquired);
    expect_eq(a4.owner_id, 20 as u64);
    expect_eq(a4.fencing_token, 2 as u64);
    expect_eq(a4.expires_ms, 12_000 as u64);
});

// ============================================================================
// Test 2: Strict Renew & Release Authorization Checks
// ============================================================================
test("leaseLock: renew and release enforce owner, fencing token and expiry checks", () => {
    let mgr = LeaseLockManager.init();
    const acq = mgr.acquire(42, 7, 5_000, 1_000);
    expect_eq(acq.status, LeaseAcquireStatus.acquired);

    // Mismatched owner or token rejected
    expect_eq(mgr.renew(42, 8, acq.fencing_token, 5_000, 2_000), LeaseOpStatus.owner_mismatch);
    expect_eq(mgr.renew(42, 7, 999, 5_000, 2_000), LeaseOpStatus.token_mismatch);
    expect_eq(mgr.release(42, 8, acq.fencing_token, 2_000), LeaseOpStatus.owner_mismatch);
    expect_eq(mgr.release(42, 7, 999, 2_000), LeaseOpStatus.token_mismatch);

    // Valid renew succeeds and bumps renew_count
    expect_eq(mgr.renew(42, 7, acq.fencing_token, 5_000, 2_000), LeaseOpStatus.ok);
    if (view := mgr.inspect(42)) {
        expect_eq(view.renew_count, 1 as u32);
        expect_eq(view.expires_ms, 7_000 as u64);
        expect_eq(view.held, true);
    } else {
        expect_true(false);
    }

    // Valid release succeeds; subsequent release returns not_found
    expect_eq(mgr.release(42, 7, acq.fencing_token, 3_000), LeaseOpStatus.ok);
    expect_eq(mgr.release(42, 7, acq.fencing_token, 3_000), LeaseOpStatus.not_found);
    expect_eq(mgr.renew(42, 7, acq.fencing_token, 5_000, 3_000), LeaseOpStatus.not_found);

    // Acquire again and let it expire -> renew/release report expired
    const acq2 = mgr.acquire(42, 7, 1_000, 4_000);
    expect_eq(acq2.fencing_token, 2 as u64);
    expect_eq(mgr.renew(42, 7, acq2.fencing_token, 1_000, 5_000), LeaseOpStatus.expired);
});

// ============================================================================
// Test 3: Storage-Layer Split-Brain Write Gate (validateFencingToken)
// ============================================================================
test("leaseLock: validateFencingToken blocks stale zombie writes before and after release", () => {
    let mgr = LeaseLockManager.init();

    // Client A acquires resource 500 -> token 1
    const a = mgr.acquire(500, 1, 2_000, 1_000);
    expect_eq(a.fencing_token, 1 as u64);
    expect_eq(mgr.validateFencingToken(500, 1), FencingCheckStatus.valid);

    // Client A stalls (GC pause); lease expires at 3000; Client B acquires -> token 2
    const b = mgr.acquire(500, 2, 5_000, 3_500);
    expect_eq(b.fencing_token, 2 as u64);

    // Delayed write from Client A carrying stale token 1 MUST be rejected!
    expect_eq(mgr.validateFencingToken(500, 1), FencingCheckStatus.stale_token);
    expect_eq(mgr.validateFencingToken(500, 2), FencingCheckStatus.valid);

    // Even after Client B releases the lock, stale token 1 remains rejected!
    expect_eq(mgr.release(500, 2, 2, 4_000), LeaseOpStatus.ok);
    expect_eq(mgr.validateFencingToken(500, 1), FencingCheckStatus.stale_token);
    expect_eq(mgr.validateFencingToken(500, 2), FencingCheckStatus.valid);

    // Token 0 is always invalid
    expect_eq(mgr.validateFencingToken(500, 0), FencingCheckStatus.invalid_token);
});

// ============================================================================
// Test 4: Batch sweepExpired Reclamation & Telemetry Stats
// ============================================================================
test("leaseLock: sweepExpired reclaims expired leases and updates stats accurately", () => {
    let mgr = LeaseLockManager.init();
    * = mgr.acquire(1, 10, 1_000, 1_000); // expires 2000
    * = mgr.acquire(2, 10, 2_000, 1_000); // expires 3000
    * = mgr.acquire(3, 10, 3_000, 1_000); // expires 4000
    * = mgr.acquire(4, 10, 5_000, 1_000); // expires 6000
    * = mgr.acquire(5, 10, 8_000, 1_000); // expires 9000

    const sw1 = mgr.sweepExpired(4_000);
    expect_eq(sw1.swept_count, 3 as usize);
    expect_eq(sw1.active_locks, 2 as usize);

    const sw2 = mgr.sweepExpired(10_000);
    expect_eq(sw2.swept_count, 2 as usize);
    expect_eq(sw2.active_locks, 0 as usize);

    const st = mgr.stats();
    expect_eq(st.active_locks, 0 as usize);
    expect_eq(st.tracked_resources, 5 as usize);
    expect_eq(st.current_fencing_seq, 5 as u64);
    expect_eq(st.total_acquires, 5 as u64);
    expect_eq(st.total_swept, 5 as u64);
});

// ============================================================================
// Test 5: Monotonic Clock Rollback Protection
// ============================================================================
test("leaseLock: monotonic clock rollback prevents zombie lease extension", () => {
    let mgr = LeaseLockManager.init();
    const acq = mgr.acquire(77, 1, 5_000, 10_000); // expires at 15_000
    expect_eq(acq.status, LeaseAcquireStatus.acquired);

    // Advance clock to 16_000 (past expiry)
    const sw = mgr.sweepExpired(16_000);
    expect_eq(sw.swept_count, 1 as usize);

    // Clock rolls backward to 12_000: renew MUST NOT succeed
    expect_eq(mgr.renew(77, 1, acq.fencing_token, 5_000, 12_000), LeaseOpStatus.not_found);

    // Re-acquire at rolled-back timestamp 12_000 -> clamped to monotonic 16_000
    const acq2 = mgr.acquire(77, 1, 5_000, 12_000);
    expect_eq(acq2.status, LeaseAcquireStatus.acquired);
    expect_eq(acq2.acquired_ms, 16_000 as u64);
    expect_eq(acq2.expires_ms, 21_000 as u64);
    expect_eq(acq2.fencing_token, 2 as u64);
});

// ============================================================================
// Test 6: Full 64-Slot Table Guard & Auto-Sweep Retry
// ============================================================================
test("leaseLock: 64-slot full-table guard sweeps expired leases before rejecting", () => {
    let mgr = LeaseLockManager.init();

    // Fill all 64 slots: first 32 expire at t=2000, next 32 expire at t=10000
    let r: u64 = 0;
    while (r < (LEASE_LOCK_MAX_LOCKS as u64)) : (r += 1) {
        const ttl: u64 = if (r < 32) 1_000 else 9_000;
        const res = mgr.acquire(1000 + r, 1, ttl, 1_000);
        expect_eq(res.status, LeaseAcquireStatus.acquired);
    }

    // At t=1500 all 64 locks are still active -> 65th resource rejected with table_full
    const full_res = mgr.acquire(9999, 2, 5_000, 1_500);
    expect_eq(full_res.status, LeaseAcquireStatus.table_full);

    // At t=2500 the first 32 locks are expired -> acquire auto-sweeps and succeeds!
    const retry_res = mgr.acquire(9999, 2, 5_000, 2_500);
    expect_eq(retry_res.status, LeaseAcquireStatus.acquired);
    expect_eq(retry_res.resource_id, 9999 as u64);
    expect_eq(retry_res.fencing_token, 65 as u64);
    expect_eq(mgr.stats().active_locks, 33 as usize);
});

// ============================================================================
// Test 7: Boundary & Overflow Guards + cap.time Dynamic Facade
// ============================================================================
test("leaseLock: invalid args, u64 saturating expiry & cap.time dynamic facade", () => {
    let mgr = cap.time.LeaseLockManager.init();

    // Zero ttl_ms or zero owner_id rejected
    expect_eq(mgr.acquire(1, 0, 1_000, 100).status, LeaseAcquireStatus.invalid_arg);
    expect_eq(mgr.acquire(1, 1, 0, 100).status, LeaseAcquireStatus.invalid_arg);
    expect_eq(mgr.renew(1, 0, 1, 1_000, 100), LeaseOpStatus.invalid_arg);
    expect_eq(mgr.renew(1, 1, 0, 1_000, 100), LeaseOpStatus.invalid_arg);
    expect_eq(mgr.renew(1, 1, 1, 0, 100), LeaseOpStatus.invalid_arg);
    expect_eq(mgr.release(1, 0, 1, 100), LeaseOpStatus.invalid_arg);
    expect_eq(mgr.release(1, 1, 0, 100), LeaseOpStatus.invalid_arg);

    // u64 overflow guard on now_ms + ttl_ms saturates cleanly at max u64
    const near_max: u64 = 0xFFFFFFFFFFFFFFF0;
    const sat = mgr.acquire(888, 5, 1_000, near_max);
    expect_eq(sat.status, LeaseAcquireStatus.acquired);
    expect_eq(sat.expires_ms, 0xFFFFFFFFFFFFFFFF as u64);
    expect_eq(sat.remaining_ttl_ms, 15 as u64);
});
