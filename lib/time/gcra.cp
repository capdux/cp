// Origin: Pure-CP Stdlib (Zero-Alloc Industrial GCRA Smooth Burst Rate Limiter & Multi-Tenant Quota Table) | Created: 2026-11-22
// Pure CP zero-heap-allocation Generic Cell Rate Algorithm (GCRA) smooth burst rate limiter
// (Stripe / Cloudflare production design):
//   - Single-timestamp Theoretical Arrival Time (TAT) per tenant (24B/slot, zero background timers)
//   - Exact `retry_after_ms` & `reset_after_ms` mapping directly to HTTP Retry-After / X-RateLimit-* headers
//   - Weighted `cost >= 1` consumption, read-only `peek`, clock-rollback clamping, and open-addressing
//     multi-tenant table with oldest-TAT eviction bounding memory strictly to caller-owned `[]GcraTenantSlot`

const std = @import("std");

// ============================================================================
// Internal Mixing & Safe Arithmetic Helpers
// ============================================================================

fn mixHash64(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn saturatingAddI64(a: i64, b: i64): i64 {
    const max_i64: i64 = std.math.maxInt(i64);
    if (b > 0 && a > max_i64 - b) {
        return max_i64;
    }
    return a + b;
}

// ============================================================================
// 1. GCRA Configuration, Decision, Slot & Stats Models
// ============================================================================

/// Configuration for the GCRA smooth burst rate limiter.
/// - `emission_interval_ms`: time interval (ms) per single unit of quota (e.g., 100ms = 10 req/s)
/// - `burst_capacity`: maximum burst units allowed instantaneously (must be >= 1)
/// - `max_cost_per_call`: maximum single-call cost allowed (`1 <= max_cost_per_call <= burst_capacity`)
pub struct GcraConfig {
    emission_interval_ms: i64 = 100;
    burst_capacity: u32 = 10;
    max_cost_per_call: u32 = 10;

    /// Constructs and validates a `GcraConfig`, guarding against zero/negative intervals and overflow.
    static fn init(
        emission_interval_ms: i64,
        burst_capacity: u32,
        max_cost_per_call: u32,
    ): GcraConfig? {
        const cfg = GcraConfig{
            emission_interval_ms: emission_interval_ms,
            burst_capacity: burst_capacity,
            max_cost_per_call: max_cost_per_call,
        };
        if (!cfg.isValid()) {
            return null;
        }
        return cfg;
    }

    /// Validates GCRA configuration invariants and `burst_capacity * emission_interval_ms` overflow.
    fn isValid(): bool {
        if (_.emission_interval_ms <= 0 || _.burst_capacity == 0 || _.max_cost_per_call == 0) {
            return false;
        }
        if (_.max_cost_per_call > _.burst_capacity) {
            return false;
        }
        const cap_i64: i64 = _.burst_capacity as i64;
        if (cap_i64 > std.math.maxInt(i64) / _.emission_interval_ms) {
            return false;
        }
        return true;
    }

    /// Returns total burst window duration in milliseconds (`burst_capacity * emission_interval_ms`).
    fn burstWindowMs(): i64 {
        return _.emission_interval_ms * (_.burst_capacity as i64);
    }
}

/// Exact rate-limit decision returned by `GcraLimiter.checkAndConsume` and `peek`.
/// Directly maps to standard HTTP rate-limit headers:
/// - `remaining_burst` -> `X-RateLimit-Remaining`
/// - `retry_after_ms`  -> `Retry-After` (non-zero when `allowed == false` and cost is feasible)
/// - `reset_after_ms`  -> `X-RateLimit-Reset` (ms until burst bucket fully refills)
pub struct GcraDecision {
    allowed: bool = false;
    remaining_burst: u32 = 0;
    retry_after_ms: i64 = 0;
    reset_after_ms: i64 = 0;
    tat_ms: i64 = 0;
}

/// Compact per-tenant slot storing only `key_hash`, `tat_ms` (Theoretical Arrival Time), and occupancy.
pub struct GcraTenantSlot {
    key_hash: u64 = 0;
    tat_ms: i64 = 0;
    occupied: bool = false;
}

/// Aggregate operational statistics for `GcraLimiter`.
pub struct GcraStats {
    capacity: usize = 0;
    active_tenants: usize = 0;
    total_allowed: u64 = 0;
    total_rejected: u64 = 0;
    total_evictions: u64 = 0;
}

// ============================================================================
// 2. Zero-Alloc Multi-Tenant GCRA Rate Limiter
// ============================================================================

/// Zero-heap-allocation multi-tenant GCRA rate limiter backed by a caller-owned
/// `[]GcraTenantSlot` open-addressing table with oldest-TAT eviction.
pub struct GcraLimiter {
    slots: []GcraTenantSlot;
    cfg: GcraConfig;
    active_count: usize = 0;
    total_allowed: u64 = 0;
    total_rejected: u64 = 0;
    total_evictions: u64 = 0;

    /// Initializes a zero-alloc multi-tenant GCRA limiter over caller-owned `slots_buf`.
    /// Returns `null` if `slots_buf.len == 0` or `cfg` is invalid.
    static fn init(slots_buf: *[]GcraTenantSlot, cfg: GcraConfig): GcraLimiter? {
        if (slots_buf.len == 0 || !cfg.isValid()) {
            return null;
        }
        let i: usize = 0;
        while (i < slots_buf.len) : (i += 1) {
            slots_buf[i] = GcraTenantSlot{};
        }
        return GcraLimiter{
            slots: slots_buf,
            cfg: cfg,
            active_count: 0,
            total_allowed: 0,
            total_rejected: 0,
            total_evictions: 0,
        };
    }

    /// Evaluates GCRA for a given `current_tat` without mutating state.
    fn evalGcra(current_tat: i64, cost: u32, now_ms: i64): GcraDecision {
        const interval: i64 = _.cfg.emission_interval_ms;
        const burst_win: i64 = _.cfg.burstWindowMs();

        // Clock-rollback guard: under monotonic time, tat <= now_ms + burst_win always holds.
        // If tat exceeds now_ms + burst_win, a backward clock jump occurred; clamp tat so the
        // tenant cannot be locked out indefinitely nor bypass rate limits.
        const max_valid_tat: i64 = saturatingAddI64(now_ms, burst_win);
        let eff_tat: i64 = current_tat;
        if (eff_tat > max_valid_tat) {
            eff_tat = max_valid_tat;
        }
        if (eff_tat < now_ms) {
            eff_tat = now_ms;
        }

        // Reject infeasible cost exceeding max_cost_per_call
        if (cost > _.cfg.max_cost_per_call) {
            const avail_ms: i64 = max_valid_tat - eff_tat;
            const rem: u32 = if (avail_ms > 0) (avail_ms / interval) as u32 else 0;
            const reset_ms: i64 = if (eff_tat > now_ms) eff_tat - now_ms else 0;
            return GcraDecision{
                allowed: false,
                remaining_burst: rem,
                retry_after_ms: 0,
                reset_after_ms: reset_ms,
                tat_ms: eff_tat,
            };
        }

        // Zero-cost probe: allowed without advancing TAT
        if (cost == 0) {
            const avail_ms: i64 = max_valid_tat - eff_tat;
            const rem: u32 = if (avail_ms > 0) (avail_ms / interval) as u32 else 0;
            const reset_ms: i64 = if (eff_tat > now_ms) eff_tat - now_ms else 0;
            return GcraDecision{
                allowed: true,
                remaining_burst: rem,
                retry_after_ms: 0,
                reset_after_ms: reset_ms,
                tat_ms: eff_tat,
            };
        }

        const increment: i64 = interval * (cost as i64);
        const new_tat: i64 = saturatingAddI64(eff_tat, increment);
        const allow_at: i64 = new_tat - burst_win;

        if (allow_at <= now_ms) {
            // Allowed: compute remaining burst units and time until full bucket reset
            const avail_ms: i64 = max_valid_tat - new_tat;
            const rem: u32 = if (avail_ms > 0) (avail_ms / interval) as u32 else 0;
            const reset_ms: i64 = if (new_tat > now_ms) new_tat - now_ms else 0;
            return GcraDecision{
                allowed: true,
                remaining_burst: rem,
                retry_after_ms: 0,
                reset_after_ms: reset_ms,
                tat_ms: new_tat,
            };
        }

        // Rejected: exact wait duration until `cost` units become available
        const retry_ms: i64 = allow_at - now_ms;
        const avail_ms: i64 = max_valid_tat - eff_tat;
        const rem: u32 = if (avail_ms > 0) (avail_ms / interval) as u32 else 0;
        const reset_ms: i64 = if (eff_tat > now_ms) eff_tat - now_ms else 0;
        return GcraDecision{
            allowed: false,
            remaining_burst: rem,
            retry_after_ms: retry_ms,
            reset_after_ms: reset_ms,
            tat_ms: eff_tat,
        };
    }

    /// Finds the existing slot index for `key_hash` using open addressing, or `null` if absent.
    fn findSlot(key_hash: u64): usize? {
        const cap: usize = _.slots.len;
        const start: usize = (mixHash64(key_hash) % (cap as u64)) as usize;
        let step: usize = 0;
        while (step < cap) : (step += 1) {
            const idx: usize = (start + step) % cap;
            if (_.slots[idx].occupied && _.slots[idx].key_hash == key_hash) {
                return idx;
            }
        }
        return null;
    }

    /// Read-only probe: evaluates whether `key_hash` can consume `cost` units at `now_ms`
    /// without mutating tenant TAT, slots, or statistics.
    fn peek(key_hash: u64, cost: u32, now_ms: i64): GcraDecision {
        let cur_tat: i64 = now_ms;
        if (idx := _.findSlot(key_hash)) {
            cur_tat = _.slots[idx].tat_ms;
        }
        return _.evalGcra(cur_tat, cost, now_ms);
    }

    /// Checks and consumes `cost` quota units for `key_hash` at `now_ms`.
    /// On acceptance, updates the tenant's TAT (allocating an empty slot or evicting the
    /// tenant with the oldest/smallest `tat_ms` if the table is full).
    fn checkAndConsume(*_, key_hash: u64, cost: u32, now_ms: i64): GcraDecision {
        const cap: usize = _.slots.len;
        const start: usize = (mixHash64(key_hash) % (cap as u64)) as usize;

        let found_idx: usize? = null;
        let free_idx: usize? = null;
        let oldest_idx: usize = 0;
        let oldest_tat: i64 = std.math.maxInt(i64);

        let step: usize = 0;
        while (step < cap) : (step += 1) {
            const idx: usize = (start + step) % cap;
            const slot = _.slots[idx];
            if (slot.occupied) {
                if (slot.key_hash == key_hash) {
                    found_idx = idx;
                    break;
                }
                if (slot.tat_ms < oldest_tat) {
                    oldest_tat = slot.tat_ms;
                    oldest_idx = idx;
                }
            } else {
                if (free_idx == null) {
                    free_idx = idx;
                }
            }
        }

        let cur_tat: i64 = now_ms;
        if (f_idx := found_idx) {
            cur_tat = _.slots[f_idx].tat_ms;
        }

        const decision = _.evalGcra(cur_tat, cost, now_ms);
        if (!decision.allowed) {
            _.total_rejected += 1;
            return decision;
        }

        _.total_allowed += 1;
        if (cost == 0) {
            return decision;
        }

        if (f_idx := found_idx) {
            _.slots[f_idx].tat_ms = decision.tat_ms;
        } else if (e_idx := free_idx) {
            _.slots[e_idx] = GcraTenantSlot{
                key_hash: key_hash,
                tat_ms: decision.tat_ms,
                occupied: true,
            };
            _.active_count += 1;
        } else {
            // Table is full: evict tenant with oldest (smallest) TAT
            _.slots[oldest_idx] = GcraTenantSlot{
                key_hash: key_hash,
                tat_ms: decision.tat_ms,
                occupied: true,
            };
            _.total_evictions += 1;
        }
        return decision;
    }

    /// Removes and resets a single tenant's quota state. Returns `true` if the tenant existed.
    fn resetTenant(*_, key_hash: u64): bool {
        if (idx := _.findSlot(key_hash)) {
            _.slots[idx] = GcraTenantSlot{};
            if (_.active_count > 0) {
                _.active_count -= 1;
            }
            return true;
        }
        return false;
    }

    /// Clears all tenant slots and resets operational counters without reallocating.
    fn resetAll(*_) {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            _.slots[i] = GcraTenantSlot{};
        }
        _.active_count = 0;
        _.total_allowed = 0;
        _.total_rejected = 0;
        _.total_evictions = 0;
    }

    /// Returns aggregate capacity, active tenant count, allow/reject counts, and eviction count.
    fn stats(): GcraStats {
        return GcraStats{
            capacity: _.slots.len,
            active_tenants: _.active_count,
            total_allowed: _.total_allowed,
            total_rejected: _.total_rejected,
            total_evictions: _.total_evictions,
        };
    }
}
