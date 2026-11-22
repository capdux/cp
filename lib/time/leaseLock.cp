// Origin: Pure-CP Stdlib (Zero-Alloc Distributed Lease Lock Table & Monotonic Fencing-Token Manager) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation distributed lease & fencing-token lock manager:
//   - Fixed 64-slot resource lease table tracking `resource_id`, `owner_id`, strictly monotonically
//     increasing `fencing_token`, `acquired_ms`, `expires_ms`, and `renew_count`.
//   - Core lease primitives: `acquire` (with same-owner reentrant renew and `already_held` remaining TTL),
//     `renew` (strict owner + token + non-expired check), `release` (records highest committed
//     `last_fencing_token`), `validateFencingToken` (storage-layer split-brain write gate), and
//     `sweepExpired` (batch expiry reclamation).
//   - Full defensive guards: `ttl_ms == 0`, `owner_id == 0`, monotonic clock rollback protection,
//     full-table retry after sweeping expired leases, and `u64` overflow-safe `now_ms + ttl_ms`.

// ============================================================================
// Constants
// ============================================================================

pub const LEASE_LOCK_MAX_LOCKS: usize = 64;

// ============================================================================
// Internal Helpers
// ============================================================================

fn saturatingAddU64(a: u64, b: u64): u64 {
    const max_u64: u64 = 0xFFFFFFFFFFFFFFFF;
    if (a > max_u64 - b) {
        return max_u64;
    }
    return a + b;
}

// ============================================================================
// 1. Enums & Data Models
// ============================================================================

/// Outcome status of `LeaseLockManager.acquire`.
pub enum LeaseAcquireStatus {
    acquired,
    reentered_renewed,
    already_held,
    table_full,
    invalid_arg,
}

/// Outcome status of `LeaseLockManager.renew` and `LeaseLockManager.release`.
pub enum LeaseOpStatus {
    ok,
    not_found,
    expired,
    owner_mismatch,
    token_mismatch,
    invalid_arg,
}

/// Storage-layer fencing-token verification verdict.
pub enum FencingCheckStatus {
    valid,
    stale_token,
    invalid_token,
}

/// Internal fixed-capacity slot for one tracked resource lease.
pub struct LeaseSlot {
    in_use: bool = false;
    held: bool = false;
    resource_id: u64 = 0;
    owner_id: u64 = 0;
    fencing_token: u64 = 0;
    last_fencing_token: u64 = 0;
    acquired_ms: u64 = 0;
    expires_ms: u64 = 0;
    renew_count: u32 = 0;
}

/// Result returned by `LeaseLockManager.acquire`.
pub struct LeaseAcquireResult {
    status: LeaseAcquireStatus = LeaseAcquireStatus.invalid_arg;
    resource_id: u64 = 0;
    owner_id: u64 = 0;
    fencing_token: u64 = 0;
    acquired_ms: u64 = 0;
    expires_ms: u64 = 0;
    remaining_ttl_ms: u64 = 0;
    renew_count: u32 = 0;
}

/// Read-only snapshot of a resource lease slot.
pub struct LeaseView {
    resource_id: u64 = 0;
    owner_id: u64 = 0;
    fencing_token: u64 = 0;
    last_fencing_token: u64 = 0;
    acquired_ms: u64 = 0;
    expires_ms: u64 = 0;
    renew_count: u32 = 0;
    held: bool = false;
}

/// Summary returned by `LeaseLockManager.sweepExpired`.
pub struct LeaseSweepResult {
    swept_count: usize = 0;
    active_locks: usize = 0;
}

/// Aggregate telemetry snapshot of `LeaseLockManager`.
pub struct LeaseManagerStats {
    capacity: usize = LEASE_LOCK_MAX_LOCKS;
    active_locks: usize = 0;
    tracked_resources: usize = 0;
    current_fencing_seq: u64 = 0;
    last_now_ms: u64 = 0;
    total_acquires: u64 = 0;
    total_renews: u64 = 0;
    total_releases: u64 = 0;
    total_swept: u64 = 0;
}

// ============================================================================
// 2. Distributed Lease & Fencing-Token Lock Manager
// ============================================================================

/// Zero-heap-allocation distributed lease lock table with monotonic fencing tokens.
pub struct LeaseLockManager {
    slots: [64]LeaseSlot = [LeaseSlot{}; 64];
    fencing_seq: u64 = 0;
    last_now_ms: u64 = 0;
    total_acquires: u64 = 0;
    total_renews: u64 = 0;
    total_releases: u64 = 0;
    total_swept: u64 = 0;

    /// Initializes a fresh empty `LeaseLockManager`.
    static fn init(): LeaseLockManager {
        return LeaseLockManager{};
    }

    /// Acquires a lease on `resource_id` for `owner_id` with duration `ttl_ms` at `now_ms`.
    /// - Rejects `ttl_ms == 0` or `owner_id == 0` with `LeaseAcquireStatus.invalid_arg`.
    /// - If the resource is free or expired, grants a new lease with a strictly higher `fencing_token`.
    /// - If already actively held by the same `owner_id`, re-enters and renews the lease.
    /// - If actively held by a different `owner_id`, returns `already_held` with `remaining_ttl_ms`.
    /// - If all 64 slots are occupied, automatically sweeps expired leases before returning `table_full`.
    fn acquire(
        *_,
        resource_id: u64,
        owner_id: u64,
        ttl_ms: u64,
        now_ms: u64,
    ): LeaseAcquireResult {
        if (ttl_ms == 0 || owner_id == 0) {
            return LeaseAcquireResult{
                status: LeaseAcquireStatus.invalid_arg,
                resource_id: resource_id,
            };
        }

        const effective_now = _.clampNow(now_ms);
        const existing_idx = _.findResourceSlot(resource_id);

        if (existing_idx < LEASE_LOCK_MAX_LOCKS) {
            // Check if existing lease is still active at effective_now
            if (_.slots[existing_idx].held && effective_now < _.slots[existing_idx].expires_ms) {
                if (_.slots[existing_idx].owner_id == owner_id) {
                    // Same owner reentrant renew
                    const new_exp = saturatingAddU64(effective_now, ttl_ms);
                    _.slots[existing_idx].expires_ms = new_exp;
                    _.slots[existing_idx].renew_count += 1;
                    _.total_renews += 1;
                    return LeaseAcquireResult{
                        status: LeaseAcquireStatus.reentered_renewed,
                        resource_id: resource_id,
                        owner_id: owner_id,
                        fencing_token: _.slots[existing_idx].fencing_token,
                        acquired_ms: _.slots[existing_idx].acquired_ms,
                        expires_ms: new_exp,
                        remaining_ttl_ms: new_exp - effective_now,
                        renew_count: _.slots[existing_idx].renew_count,
                    };
                } else {
                    const rem = _.slots[existing_idx].expires_ms - effective_now;
                    return LeaseAcquireResult{
                        status: LeaseAcquireStatus.already_held,
                        resource_id: resource_id,
                        owner_id: _.slots[existing_idx].owner_id,
                        fencing_token: _.slots[existing_idx].fencing_token,
                        acquired_ms: _.slots[existing_idx].acquired_ms,
                        expires_ms: _.slots[existing_idx].expires_ms,
                        remaining_ttl_ms: rem,
                        renew_count: _.slots[existing_idx].renew_count,
                    };
                }
            }

            // Existing slot is either released or expired -> grant fresh lease with higher fencing_token
            return _.grantInSlot(existing_idx, resource_id, owner_id, ttl_ms, effective_now);
        }

        // Brand new resource_id: find free or non-held slot; if none, sweep expired leases and retry
        let slot_idx = _.findAllocatableSlot();
        if (slot_idx >= LEASE_LOCK_MAX_LOCKS) {
            * = _.sweepExpired(effective_now);
            slot_idx = _.findAllocatableSlot();
            if (slot_idx >= LEASE_LOCK_MAX_LOCKS) {
                return LeaseAcquireResult{
                    status: LeaseAcquireStatus.table_full,
                    resource_id: resource_id,
                };
            }
        }

        return _.grantInSlot(slot_idx, resource_id, owner_id, ttl_ms, effective_now);
    }

    /// Renews an active lease iff `owner_id` and `fencing_token` match and the lease has not expired.
    fn renew(
        *_,
        resource_id: u64,
        owner_id: u64,
        fencing_token: u64,
        ttl_ms: u64,
        now_ms: u64,
    ): LeaseOpStatus {
        if (ttl_ms == 0 || owner_id == 0 || fencing_token == 0) {
            return LeaseOpStatus.invalid_arg;
        }
        const effective_now = _.clampNow(now_ms);
        const idx = _.findResourceSlot(resource_id);
        if (idx >= LEASE_LOCK_MAX_LOCKS || !_.slots[idx].held) {
            return LeaseOpStatus.not_found;
        }
        if (effective_now >= _.slots[idx].expires_ms) {
            _.slots[idx].held = false;
            _.slots[idx].owner_id = 0;
            _.slots[idx].expires_ms = 0;
            _.total_swept += 1;
            return LeaseOpStatus.expired;
        }
        if (_.slots[idx].owner_id != owner_id) {
            return LeaseOpStatus.owner_mismatch;
        }
        if (_.slots[idx].fencing_token != fencing_token) {
            return LeaseOpStatus.token_mismatch;
        }

        _.slots[idx].expires_ms = saturatingAddU64(effective_now, ttl_ms);
        _.slots[idx].renew_count += 1;
        if (fencing_token > _.slots[idx].last_fencing_token) {
            _.slots[idx].last_fencing_token = fencing_token;
        }
        _.total_renews += 1;
        return LeaseOpStatus.ok;
    }

    /// Releases an active lease iff `owner_id` and `fencing_token` match and records `last_fencing_token`.
    fn release(
        *_,
        resource_id: u64,
        owner_id: u64,
        fencing_token: u64,
        now_ms: u64,
    ): LeaseOpStatus {
        if (owner_id == 0 || fencing_token == 0) {
            return LeaseOpStatus.invalid_arg;
        }
        const effective_now = _.clampNow(now_ms);
        const idx = _.findResourceSlot(resource_id);
        if (idx >= LEASE_LOCK_MAX_LOCKS || !_.slots[idx].held) {
            return LeaseOpStatus.not_found;
        }
        if (effective_now >= _.slots[idx].expires_ms) {
            _.slots[idx].held = false;
            _.slots[idx].owner_id = 0;
            _.slots[idx].expires_ms = 0;
            _.total_swept += 1;
            return LeaseOpStatus.expired;
        }
        if (_.slots[idx].owner_id != owner_id) {
            return LeaseOpStatus.owner_mismatch;
        }
        if (_.slots[idx].fencing_token != fencing_token) {
            return LeaseOpStatus.token_mismatch;
        }

        if (fencing_token > _.slots[idx].last_fencing_token) {
            _.slots[idx].last_fencing_token = fencing_token;
        }
        _.slots[idx].held = false;
        _.slots[idx].owner_id = 0;
        _.slots[idx].expires_ms = 0;
        _.slots[idx].renew_count = 0;
        _.total_releases += 1;
        return LeaseOpStatus.ok;
    }

    /// Storage-layer split-brain write gate: rejects any delayed write carrying a `fencing_token`
    /// smaller than the highest `last_fencing_token` observed for `resource_id`.
    fn validateFencingToken(resource_id: u64, fencing_token: u64): FencingCheckStatus {
        if (fencing_token == 0) {
            return FencingCheckStatus.invalid_token;
        }
        const idx = _.findResourceSlot(resource_id);
        if (idx < LEASE_LOCK_MAX_LOCKS) {
            if (fencing_token < _.slots[idx].last_fencing_token) {
                return FencingCheckStatus.stale_token;
            }
        }
        return FencingCheckStatus.valid;
    }

    /// Batch-sweeps all expired leases at `now_ms`, preserving each resource's `last_fencing_token`.
    fn sweepExpired(*_, now_ms: u64): LeaseSweepResult {
        const effective_now = _.clampNow(now_ms);
        let swept: usize = 0;
        let active: usize = 0;
        let i: usize = 0;
        while (i < LEASE_LOCK_MAX_LOCKS) : (i += 1) {
            if (_.slots[i].in_use && _.slots[i].held) {
                if (effective_now >= _.slots[i].expires_ms) {
                    _.slots[i].held = false;
                    _.slots[i].owner_id = 0;
                    _.slots[i].expires_ms = 0;
                    _.slots[i].renew_count = 0;
                    swept += 1;
                } else {
                    active += 1;
                }
            }
        }
        _.total_swept += swept as u64;
        return LeaseSweepResult{
            swept_count: swept,
            active_locks: active,
        };
    }

    /// Returns a read-only view of `resource_id`, or `null` if untracked.
    fn inspect(resource_id: u64): LeaseView? {
        const idx = _.findResourceSlot(resource_id);
        if (idx >= LEASE_LOCK_MAX_LOCKS) {
            return null;
        }
        const s = _.slots[idx];
        return LeaseView{
            resource_id: s.resource_id,
            owner_id: s.owner_id,
            fencing_token: s.fencing_token,
            last_fencing_token: s.last_fencing_token,
            acquired_ms: s.acquired_ms,
            expires_ms: s.expires_ms,
            renew_count: s.renew_count,
            held: s.held,
        };
    }

    /// Returns aggregate lock-table telemetry.
    fn stats(): LeaseManagerStats {
        let active: usize = 0;
        let tracked: usize = 0;
        let i: usize = 0;
        while (i < LEASE_LOCK_MAX_LOCKS) : (i += 1) {
            if (_.slots[i].in_use) {
                tracked += 1;
                if (_.slots[i].held) {
                    active += 1;
                }
            }
        }
        return LeaseManagerStats{
            capacity: LEASE_LOCK_MAX_LOCKS,
            active_locks: active,
            tracked_resources: tracked,
            current_fencing_seq: _.fencing_seq,
            last_now_ms: _.last_now_ms,
            total_acquires: _.total_acquires,
            total_renews: _.total_renews,
            total_releases: _.total_releases,
            total_swept: _.total_swept,
        };
    }

    // ========================================================================
    // Internal Helpers
    // ========================================================================

    fn clampNow(*_, now_ms: u64): u64 {
        if (now_ms >= _.last_now_ms) {
            _.last_now_ms = now_ms;
            return now_ms;
        }
        return _.last_now_ms;
    }

    fn findResourceSlot(resource_id: u64): usize {
        let i: usize = 0;
        while (i < LEASE_LOCK_MAX_LOCKS) : (i += 1) {
            if (_.slots[i].in_use && _.slots[i].resource_id == resource_id) {
                return i;
            }
        }
        return LEASE_LOCK_MAX_LOCKS;
    }

    fn findAllocatableSlot(): usize {
        // 1. Prefer completely unused slot
        let i: usize = 0;
        while (i < LEASE_LOCK_MAX_LOCKS) : (i += 1) {
            if (!_.slots[i].in_use) {
                return i;
            }
        }
        // 2. Otherwise reuse released/swept slot with smallest last_fencing_token
        let best_idx: usize = LEASE_LOCK_MAX_LOCKS;
        let best_tok: u64 = 0xFFFFFFFFFFFFFFFF;
        let j: usize = 0;
        while (j < LEASE_LOCK_MAX_LOCKS) : (j += 1) {
            if (!_.slots[j].held) {
                if (best_idx == LEASE_LOCK_MAX_LOCKS || _.slots[j].last_fencing_token < best_tok) {
                    best_idx = j;
                    best_tok = _.slots[j].last_fencing_token;
                }
            }
        }
        return best_idx;
    }

    fn grantInSlot(
        *_,
        idx: usize,
        resource_id: u64,
        owner_id: u64,
        ttl_ms: u64,
        effective_now: u64,
    ): LeaseAcquireResult {
        _.fencing_seq += 1;
        const tok = _.fencing_seq;
        const exp = saturatingAddU64(effective_now, ttl_ms);

        _.slots[idx] = LeaseSlot{
            in_use: true,
            held: true,
            resource_id: resource_id,
            owner_id: owner_id,
            fencing_token: tok,
            last_fencing_token: tok,
            acquired_ms: effective_now,
            expires_ms: exp,
            renew_count: 0,
        };
        _.total_acquires += 1;

        return LeaseAcquireResult{
            status: LeaseAcquireStatus.acquired,
            resource_id: resource_id,
            owner_id: owner_id,
            fencing_token: tok,
            acquired_ms: effective_now,
            expires_ms: exp,
            remaining_ttl_ms: exp - effective_now,
            renew_count: 0,
        };
    }
}
