// Origin: Pure-CP Stdlib Test Suite (lib/web/idempotency_store.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Stripe-Style API Idempotency-Key Store:
//   1. Acquire -> commit_response -> retry exact replay_completed (status code, body & keySlice)
//   2. Concurrent retry while in_progress -> conflict_in_progress (HTTP 409)
//   3. Same key with different payload_hash -> conflict_payload_mismatch (HTTP 422) in both states
//   4. Multi-tenant (tenant_id, key) strict isolation
//   5. abort_request lock release on transient 5xx & protection of completed records
//   6. lock_ttl_ms crash deadlock recovery & result_ttl_ms expiration pruning
//   7. Empty/oversized key, oversized body, invalid config & 64-slot full capacity + reuse guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    IDEM_STORE_MAX_ENTRIES,
    IDEM_KEY_MAX_LEN,
    IDEM_BODY_MAX_LEN,
    IdemEntryState,
    IdemAcquireKind,
    IdemStoreConfig,
    IdemRecord,
    IdemAcquireDecision,
    IdemStoreStats,
    ApiIdempotencyStore,
} from "cap:web";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Acquire -> Commit -> Exact Replay Completed
// ============================================================================

fn testAcquireCommitAndReplayCompleted()! {
    let store = cap.web.ApiIdempotencyStore.init(IdemStoreConfig.defaults()) ?? return error.InitFailed;

    const d1: IdemAcquireDecision = store.begin_request(101, "idem_charge_001", 0x11223344, 1_000);
    expect_eq(d1.kind == IdemAcquireKind.acquired_new, true);

    const body: []const u8 = "{\"id\":\"ch_999\",\"amount\":4200}";
    expect_eq(store.commit_response(d1.slot_idx, 201, body, 1_050), true);

    // Double commit on already-completed slot must be rejected
    expect_eq(store.commit_response(d1.slot_idx, 200, "{}", 1_060), false);

    // Retry with same tenant, key, and payload_hash -> replay_completed
    const d2 = store.begin_request(101, "idem_charge_001", 0x11223344, 5_000);
    expect_eq(d2.kind == IdemAcquireKind.replay_completed, true);
    expect_eq(d2.slot_idx, d1.slot_idx);
    expect_eq(d2.cached_status_code, 201);

    const rec: IdemRecord = store.get_record(d2.slot_idx) ?? return error.RecordMissing;
    expect_eq(rec.state == IdemEntryState.completed, true);
    expect_eq(rec.status_code, 201);
    expect_eq(sliceEq(rec.keySlice(), "idem_charge_001"), true);
    expect_eq(sliceEq(rec.responseSlice(), body), true);

    const st: IdemStoreStats = store.stats();
    expect_eq(st.active_entries, 1);
    expect_eq(st.in_progress_count, 0);
    expect_eq(st.completed_count, 1);
    expect_eq(st.total_acquired, 1);
    expect_eq(st.total_replays, 1);
}

// ============================================================================
// Test 2: Concurrent Retry While In-Progress -> conflict_in_progress (HTTP 409)
// ============================================================================

fn testConflictInProgress()! {
    let store = ApiIdempotencyStore.init(IdemStoreConfig.defaults()) ?? return error.InitFailed;

    const d1 = store.begin_request(200, "req_inflight_42", 0xAAAA_BBBB, 10_000);
    expect_eq(d1.kind == IdemAcquireKind.acquired_new, true);

    // Concurrent client retry before commit
    const d2 = store.begin_request(200, "req_inflight_42", 0xAAAA_BBBB, 10_500);
    expect_eq(d2.kind == IdemAcquireKind.conflict_in_progress, true);
    expect_eq(d2.slot_idx, d1.slot_idx);
    expect_eq(d2.cached_status_code, 0);

    const st = store.stats();
    expect_eq(st.in_progress_count, 1);
    expect_eq(st.completed_count, 0);
    expect_eq(st.total_conflicts_in_progress, 1);
}

// ============================================================================
// Test 3: Same Key + Different Payload Hash -> conflict_payload_mismatch (422)
// ============================================================================

fn testConflictPayloadMismatchInBothStates()! {
    let store = ApiIdempotencyStore.init(IdemStoreConfig.defaults()) ?? return error.InitFailed;

    const d1 = store.begin_request(300, "order_key_7", 0x0000_0001, 1_000);
    expect_eq(d1.kind == IdemAcquireKind.acquired_new, true);

    // 3a. Mismatch while in_progress
    const d_mis1 = store.begin_request(300, "order_key_7", 0x0000_0002, 1_100);
    expect_eq(d_mis1.kind == IdemAcquireKind.conflict_payload_mismatch, true);

    // Commit the original request
    expect_eq(store.commit_response(d1.slot_idx, 200, "{\"ok\":true}", 1_200), true);

    // 3b. Mismatch after completed
    const d_mis2 = store.begin_request(300, "order_key_7", 0x9999_9999, 2_000);
    expect_eq(d_mis2.kind == IdemAcquireKind.conflict_payload_mismatch, true);

    const st = store.stats();
    expect_eq(st.total_conflicts_mismatch, 2);
    expect_eq(st.total_replays, 0);
}

// ============================================================================
// Test 4: Strict Multi-Tenant (tenant_id, key) Isolation
// ============================================================================

fn testMultiTenantIsolation()! {
    let store = ApiIdempotencyStore.init(IdemStoreConfig.defaults()) ?? return error.InitFailed;

    const d_t1 = store.begin_request(1, "shared_client_key", 0x1111, 1_000);
    const d_t2 = store.begin_request(2, "shared_client_key", 0x2222, 1_000);
    expect_eq(d_t1.kind == IdemAcquireKind.acquired_new, true);
    expect_eq(d_t2.kind == IdemAcquireKind.acquired_new, true);
    expect_eq(d_t1.slot_idx != d_t2.slot_idx, true);

    expect_eq(store.commit_response(d_t1.slot_idx, 200, "tenant1_resp", 1_010), true);
    expect_eq(store.commit_response(d_t2.slot_idx, 202, "tenant2_resp", 1_020), true);

    const r1 = store.begin_request(1, "shared_client_key", 0x1111, 2_000);
    const r2 = store.begin_request(2, "shared_client_key", 0x2222, 2_000);
    expect_eq(r1.kind == IdemAcquireKind.replay_completed, true);
    expect_eq(r2.kind == IdemAcquireKind.replay_completed, true);
    expect_eq(r1.cached_status_code, 200);
    expect_eq(r2.cached_status_code, 202);

    const rec1 = store.get_record(r1.slot_idx) ?? return error.Missing;
    const rec2 = store.get_record(r2.slot_idx) ?? return error.Missing;
    expect_eq(sliceEq(rec1.responseSlice(), "tenant1_resp"), true);
    expect_eq(sliceEq(rec2.responseSlice(), "tenant2_resp"), true);
}

// ============================================================================
// Test 5: abort_request Releases In-Progress Lock & Protects Completed Records
// ============================================================================

fn testAbortRequestLockRelease()! {
    let store = ApiIdempotencyStore.init(IdemStoreConfig.defaults()) ?? return error.InitFailed;

    const d1 = store.begin_request(500, "retryable_5xx_key", 0x5555, 1_000);
    expect_eq(d1.kind == IdemAcquireKind.acquired_new, true);

    // Downstream transient 503 -> abort lock
    expect_eq(store.abort_request(d1.slot_idx), true);
    expect_eq(store.get_record(d1.slot_idx) == null, true);

    // Second abort on freed slot returns false
    expect_eq(store.abort_request(d1.slot_idx), false);

    // Client retries -> acquires cleanly again and commits
    const d2 = store.begin_request(500, "retryable_5xx_key", 0x5555, 1_500);
    expect_eq(d2.kind == IdemAcquireKind.acquired_new, true);
    expect_eq(store.commit_response(d2.slot_idx, 200, "recovered", 1_600), true);

    // Completed record cannot be aborted
    expect_eq(store.abort_request(d2.slot_idx), false);
    expect_eq(store.stats().completed_count, 1);
}

// ============================================================================
// Test 6: lock_ttl_ms Crash Deadlock Recovery & result_ttl_ms Expiration
// ============================================================================

fn testTtlCrashRecoveryAndExpiration()! {
    const cfg = IdemStoreConfig{
        lock_ttl_ms: 5_000,
        result_ttl_ms: 20_000,
    };
    let store = ApiIdempotencyStore.init(cfg) ?? return error.InitFailed;

    // 6a. Worker crashes after acquiring lock at t=1000 (lock expires at t=6000)
    const d1 = store.begin_request(600, "crashed_job", 0xABC1, 1_000);
    expect_eq(d1.kind == IdemAcquireKind.acquired_new, true);

    // At t=5999 still locked -> conflict_in_progress
    const d_mid = store.begin_request(600, "crashed_job", 0xABC1, 5_999);
    expect_eq(d_mid.kind == IdemAcquireKind.conflict_in_progress, true);

    // At t=6000 lock expired -> automatically evicted & re-acquired cleanly!
    const d2 = store.begin_request(600, "crashed_job", 0xABC1, 6_000);
    expect_eq(d2.kind == IdemAcquireKind.acquired_new, true);
    expect_eq(store.stats().total_expired_evicted, 1);

    // Commit at t=6100 -> result expires at t=26100
    expect_eq(store.commit_response(d2.slot_idx, 200, "done", 6_100), true);

    // At t=26099 still replays
    const d_rep = store.begin_request(600, "crashed_job", 0xABC1, 26_099);
    expect_eq(d_rep.kind == IdemAcquireKind.replay_completed, true);

    // Explicit prune at t=26100 evicts expired completed result
    expect_eq(store.prune_expired(26_100), 1);
    expect_eq(store.stats().active_entries, 0);
    expect_eq(store.stats().total_expired_evicted, 2);
}

// ============================================================================
// Test 7: Invalid Config, Empty/Oversized Key, Oversized Body & 64-Slot Full Guards
// ============================================================================

fn testBoundsInvalidAndCapacityGuards()! {
    // 7a. Invalid config (zero TTLs)
    expect_eq(ApiIdempotencyStore.init(IdemStoreConfig{ lock_ttl_ms: 0, result_ttl_ms: 1000 }) == null, true);
    expect_eq(ApiIdempotencyStore.init(IdemStoreConfig{ lock_ttl_ms: 1000, result_ttl_ms: 0 }) == null, true);

    let store = ApiIdempotencyStore.init(IdemStoreConfig{ lock_ttl_ms: 1000, result_ttl_ms: 5000 }) ?? return error.InitFailed;

    // 7b. Empty key & oversized key (> 64 bytes)
    const empty_key: []const u8 = "";
    const d_empty = store.begin_request(1, empty_key, 1, 100);
    expect_eq(d_empty.kind == IdemAcquireKind.rejected_invalid, true);

    const long_key_buf: [IDEM_KEY_MAX_LEN + 1]u8 = ['k'; IDEM_KEY_MAX_LEN + 1];
    const d_long = store.begin_request(1, long_key_buf[0..], 1, 100);
    expect_eq(d_long.kind == IdemAcquireKind.rejected_invalid, true);

    // 7c. Oversized response body (> 256 bytes) rejected without mutating in_progress state
    const d_ok = store.begin_request(1, "body_limit_key", 1, 100);
    expect_eq(d_ok.kind == IdemAcquireKind.acquired_new, true);
    const big_body: [IDEM_BODY_MAX_LEN + 1]u8 = ['x'; IDEM_BODY_MAX_LEN + 1];
    expect_eq(store.commit_response(d_ok.slot_idx, 200, big_body[0..], 110), false);
    expect_eq(store.abort_request(d_ok.slot_idx), true);

    // 7d. Fill all 64 slots -> 65th rejected_full, then expires and reuses slot cleanly
    let i: usize = 0;
    while (i < IDEM_STORE_MAX_ENTRIES) : (i += 1) {
        let key_buf: [4]u8 = ['k', 'e', 'y', (33 + i) as u8];
        const acq = store.begin_request(900 + (i as u64), key_buf[0..], i as u64, 10_000);
        expect_eq(acq.kind == IdemAcquireKind.acquired_new, true);
    }
    expect_eq(store.stats().active_entries, IDEM_STORE_MAX_ENTRIES);

    const d_full = store.begin_request(9999, "overflow_65th", 777, 10_500);
    expect_eq(d_full.kind == IdemAcquireKind.rejected_full, true);

    // After lock_ttl_ms (t=11000), all 64 locks expire automatically -> new request succeeds
    const d_reused = store.begin_request(9999, "overflow_65th", 777, 11_000);
    expect_eq(d_reused.kind == IdemAcquireKind.acquired_new, true);
    expect_eq(store.stats().active_entries, 1);
    expect_eq(store.stats().total_expired_evicted, IDEM_STORE_MAX_ENTRIES as u64);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("web idempotency_store: acquire -> commit -> exact replay_completed with cached status and body", testAcquireCommitAndReplayCompleted);
test("web idempotency_store: concurrent retry during in_progress returns conflict_in_progress", testConflictInProgress);
test("web idempotency_store: payload_hash mismatch returns conflict_payload_mismatch in both states", testConflictPayloadMismatchInBothStates);
test("web idempotency_store: strict multi-tenant (tenant_id, key) isolation", testMultiTenantIsolation);
test("web idempotency_store: abort_request releases lock on 5xx and protects completed entries", testAbortRequestLockRelease);
test("web idempotency_store: lock_ttl_ms deadlock recovery and result_ttl_ms expiration pruning", testTtlCrashRecoveryAndExpiration);
test("web idempotency_store: invalid config, empty/oversized key/body & 64-slot full capacity reuse", testBoundsInvalidAndCapacityGuards);
