// Origin: Pure-CP Stdlib (Zero-Alloc Stripe-Style API Idempotency-Key Lock & Response Replay Store) | Created: 2026-11-22
// Pure CP zero-heap-allocation Stripe-style API Idempotency-Key concurrency lock & response replay engine:
//   - Fixed 64-entry inline table (`IDEM_STORE_MAX_ENTRIES`), 64B keys, 256B cached response bodies (0 heap)
//   - Multi-tenant `(tenant_id, key)` isolation with strict payload hash mismatch detection (HTTP 422)
//   - In-flight concurrency lock (`in_progress` -> HTTP 409) with crash-safe `lock_ttl_ms` auto-recovery
//   - Atomic `commit_response` -> `completed` replay cache with `result_ttl_ms` expiration & `abort_request` rollback

/// Maximum concurrent active idempotency records stored inline (zero dynamic heap allocation).
pub const IDEM_STORE_MAX_ENTRIES: usize = 64;

/// Maximum byte length of an `Idempotency-Key` header value.
pub const IDEM_KEY_MAX_LEN: usize = 64;

/// Maximum byte length of a cached idempotent response body.
pub const IDEM_BODY_MAX_LEN: usize = 256;

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return a + b;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// 1. Public Enums, Configuration & Record Models
// ============================================================================

/// Lifecycle state of an active idempotency record.
pub enum IdemEntryState {
    in_progress,
    completed,
}

/// Outcome returned by `ApiIdempotencyStore.begin_request`.
pub enum IdemAcquireKind {
    acquired_new,
    replay_completed,
    conflict_in_progress,
    conflict_payload_mismatch,
    rejected_invalid,
    rejected_full,
}

/// TTL configuration for `ApiIdempotencyStore`.
pub struct IdemStoreConfig {
    /// Lock timeout in milliseconds for `in_progress` requests (prevents deadlocks on worker crash).
    lock_ttl_ms: u64 = 30_000;
    /// Retention TTL in milliseconds for `completed` cached responses.
    result_ttl_ms: u64 = 86_400_000;

    /// Returns production defaults (`lock_ttl_ms = 30_000`, `result_ttl_ms = 86_400_000`).
    static fn defaults(): IdemStoreConfig {
        return IdemStoreConfig{};
    }

    /// Validates that both TTLs are strictly positive.
    fn isValid(): bool {
        return _.lock_ttl_ms > 0 && _.result_ttl_ms > 0;
    }
}

/// Single stored idempotency record inside `ApiIdempotencyStore`.
pub struct IdemRecord {
    tenant_id: u64 = 0;
    key_buf: [IDEM_KEY_MAX_LEN]u8 = [0; IDEM_KEY_MAX_LEN];
    key_len: usize = 0;
    payload_hash: u64 = 0;
    state: IdemEntryState = IdemEntryState.in_progress;
    status_code: u16 = 0;
    response_buf: [IDEM_BODY_MAX_LEN]u8 = [0; IDEM_BODY_MAX_LEN];
    response_len: usize = 0;
    created_ms: u64 = 0;
    expires_ms: u64 = 0;
    occupied: bool = false;

    /// Returns the stored idempotency key as a read-only byte slice.
    fn keySlice(): []const u8 {
        return _.key_buf[0.._.key_len];
    }

    /// Returns the cached response payload as a read-only byte slice.
    fn responseSlice(): []const u8 {
        return _.response_buf[0.._.response_len];
    }
}

/// Decision returned by `ApiIdempotencyStore.begin_request`.
pub struct IdemAcquireDecision {
    kind: IdemAcquireKind = IdemAcquireKind.rejected_invalid;
    slot_idx: usize = 0;
    cached_status_code: u16 = 0;
}

/// Operational telemetry snapshot for `ApiIdempotencyStore`.
pub struct IdemStoreStats {
    active_entries: usize = 0;
    in_progress_count: usize = 0;
    completed_count: usize = 0;
    total_acquired: u64 = 0;
    total_replays: u64 = 0;
    total_conflicts_in_progress: u64 = 0;
    total_conflicts_mismatch: u64 = 0;
    total_expired_evicted: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc Stripe-Style API Idempotency Store
// ============================================================================

/// Fixed-capacity (64 slots) zero-heap-allocation Stripe-style API Idempotency-Key store.
pub struct ApiIdempotencyStore {
    cfg: IdemStoreConfig = IdemStoreConfig{},
    slots: [IDEM_STORE_MAX_ENTRIES]IdemRecord = [IdemRecord{}; IDEM_STORE_MAX_ENTRIES],
    total_acquired: u64 = 0,
    total_replays: u64 = 0,
    total_conflicts_in_progress: u64 = 0,
    total_conflicts_mismatch: u64 = 0,
    total_expired_evicted: u64 = 0,

    /// Initializes a validated `ApiIdempotencyStore`, returning `null` if `cfg` is invalid.
    static fn init(cfg: IdemStoreConfig): ApiIdempotencyStore? {
        if (!cfg.isValid()) return null;
        return ApiIdempotencyStore{ cfg: cfg };
    }

    /// Evicts all occupied records whose `expires_ms <= now_ms` (including timed-out `in_progress` locks).
    /// Returns the number of records evicted.
    fn prune_expired(*_, now_ms: u64): usize {
        let evicted: usize = 0;
        let i: usize = 0;
        while (i < IDEM_STORE_MAX_ENTRIES) : (i += 1) {
            if (_.slots[i].occupied && now_ms >= _.slots[i].expires_ms) {
                _.slots[i].occupied = false;
                _.slots[i].key_len = 0;
                _.slots[i].response_len = 0;
                evicted += 1;
                _.total_expired_evicted += 1;
            }
        }
        return evicted;
    }

    /// Begins an idempotent request for `(tenant_id, key)` with request fingerprint `payload_hash`:
    ///   - Automatically prunes expired entries (including stale crashed `in_progress` locks).
    ///   - On existing `(tenant_id, key)`:
    ///       * Different `payload_hash` -> `conflict_payload_mismatch` (HTTP 422)
    ///       * Same hash & `in_progress` -> `conflict_in_progress` (HTTP 409)
    ///       * Same hash & `completed`   -> `replay_completed` with cached `status_code` & `slot_idx`
    ///   - On miss: allocates a free slot in `in_progress` state with `expires_ms = now_ms + lock_ttl_ms`.
    fn begin_request(
        *_,
        tenant_id: u64,
        key: []const u8,
        payload_hash: u64,
        now_ms: u64,
    ): IdemAcquireDecision {
        if (key.len == 0 || key.len > IDEM_KEY_MAX_LEN) {
            return IdemAcquireDecision{ kind: IdemAcquireKind.rejected_invalid };
        }

        * = _.prune_expired(now_ms);

        let found_free: bool = false;
        let free_idx: usize = 0;
        let i: usize = 0;
        while (i < IDEM_STORE_MAX_ENTRIES) : (i += 1) {
            if (!_.slots[i].occupied) {
                if (!found_free) {
                    found_free = true;
                    free_idx = i;
                }
            } else if (_.slots[i].tenant_id == tenant_id && bytesEqual(_.slots[i].key_buf[0.._.slots[i].key_len], key)) {
                if (_.slots[i].payload_hash != payload_hash) {
                    _.total_conflicts_mismatch += 1;
                    return IdemAcquireDecision{
                        kind: IdemAcquireKind.conflict_payload_mismatch,
                        slot_idx: i,
                        cached_status_code: 0,
                    };
                }
                if (_.slots[i].state == IdemEntryState.in_progress) {
                    _.total_conflicts_in_progress += 1;
                    return IdemAcquireDecision{
                        kind: IdemAcquireKind.conflict_in_progress,
                        slot_idx: i,
                        cached_status_code: 0,
                    };
                }
                _.total_replays += 1;
                return IdemAcquireDecision{
                    kind: IdemAcquireKind.replay_completed,
                    slot_idx: i,
                    cached_status_code: _.slots[i].status_code,
                };
            }
        }

        if (!found_free) {
            return IdemAcquireDecision{ kind: IdemAcquireKind.rejected_full };
        }

        let rec = IdemRecord{
            tenant_id: tenant_id,
            key_buf: [0; IDEM_KEY_MAX_LEN],
            key_len: key.len,
            payload_hash: payload_hash,
            state: IdemEntryState.in_progress,
            status_code: 0,
            response_buf: [0; IDEM_BODY_MAX_LEN],
            response_len: 0,
            created_ms: now_ms,
            expires_ms: satAddU64(now_ms, _.cfg.lock_ttl_ms),
            occupied: true,
        };
        let k: usize = 0;
        while (k < key.len) : (k += 1) {
            rec.key_buf[k] = key[k];
        }
        _.slots[free_idx] = rec;
        _.total_acquired += 1;

        return IdemAcquireDecision{
            kind: IdemAcquireKind.acquired_new,
            slot_idx: free_idx,
            cached_status_code: 0,
        };
    }

    /// Commits a completed response for an `in_progress` slot, transitioning it to `completed`
    /// and extending its TTL to `now_ms + result_ttl_ms`.
    /// Returns `false` if `slot_idx` is invalid, not `in_progress`, or `response_body.len > IDEM_BODY_MAX_LEN`.
    fn commit_response(
        *_,
        slot_idx: usize,
        status_code: u16,
        response_body: []const u8,
        now_ms: u64,
    ): bool {
        if (slot_idx >= IDEM_STORE_MAX_ENTRIES) return false;
        if (!_.slots[slot_idx].occupied) return false;
        if (_.slots[slot_idx].state != IdemEntryState.in_progress) return false;
        if (response_body.len > IDEM_BODY_MAX_LEN) return false;

        let b: usize = 0;
        while (b < response_body.len) : (b += 1) {
            _.slots[slot_idx].response_buf[b] = response_body[b];
        }
        _.slots[slot_idx].response_len = response_body.len;
        _.slots[slot_idx].status_code = status_code;
        _.slots[slot_idx].state = IdemEntryState.completed;
        _.slots[slot_idx].expires_ms = satAddU64(now_ms, _.cfg.result_ttl_ms);
        return true;
    }

    /// Aborts an `in_progress` request (e.g., on transient 5xx failure), releasing the lock
    /// so the client can safely retry with the same `Idempotency-Key`.
    fn abort_request(*_, slot_idx: usize): bool {
        if (slot_idx >= IDEM_STORE_MAX_ENTRIES) return false;
        if (!_.slots[slot_idx].occupied) return false;
        if (_.slots[slot_idx].state != IdemEntryState.in_progress) return false;

        _.slots[slot_idx].occupied = false;
        _.slots[slot_idx].key_len = 0;
        _.slots[slot_idx].response_len = 0;
        return true;
    }

    /// Returns a copy of the active `IdemRecord` at `slot_idx`, or `null` if out of bounds or unoccupied.
    fn get_record(slot_idx: usize): IdemRecord? {
        if (slot_idx >= IDEM_STORE_MAX_ENTRIES) return null;
        if (!_.slots[slot_idx].occupied) return null;
        return _.slots[slot_idx];
    }

    /// Returns a snapshot of live occupancy and cumulative store telemetry.
    fn stats(): IdemStoreStats {
        let active: usize = 0;
        let in_prog: usize = 0;
        let comp: usize = 0;
        let i: usize = 0;
        while (i < IDEM_STORE_MAX_ENTRIES) : (i += 1) {
            if (_.slots[i].occupied) {
                active += 1;
                if (_.slots[i].state == IdemEntryState.in_progress) {
                    in_prog += 1;
                } else {
                    comp += 1;
                }
            }
        }
        return IdemStoreStats{
            active_entries: active,
            in_progress_count: in_prog,
            completed_count: comp,
            total_acquired: _.total_acquired,
            total_replays: _.total_replays,
            total_conflicts_in_progress: _.total_conflicts_in_progress,
            total_conflicts_mismatch: _.total_conflicts_mismatch,
            total_expired_evicted: _.total_expired_evicted,
        };
    }
}
