// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Tenant Sliding Window Counter Rate Limiter) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation Cloudflare-style hybrid Sliding Window Counter rate limiter:
//   - Fixed-capacity multi-tenant table (`RATE_WINDOW_MAX_KEYS = 128` keyed by `key_hash: u64`)
//   - Exact 128-bit integer weighted interpolation:
//       effective_count = prev_count * (1 - elapsed_in_curr / window_ms) + curr_count
//     eliminating fixed-window 2x boundary bursts with zero floating-point rounding drift
//   - Weighted `checkAndConsume(*_, key_hash, cost, now_ms)`, read-only `peek(key_hash, now_ms)`,
//     and per-tenant quota overrides `setKeyQuota(*_, key_hash, limit, window_ms)`
//   - Exact 1ms-accurate `retry_after_ms` & `reset_at_ms`, automatic stale-window zeroing,
//     expired-first + LRU eviction when full, and zero-heap cumulative telemetry snapshots

/// Maximum number of concurrent tenant keys tracked inline by `RateWindowLimiter`.
pub const RATE_WINDOW_MAX_KEYS: usize = 128;

// ============================================================================
// 1. Configuration, Decision & Telemetry Models (100% Zero-Heap)
// ============================================================================

/// Default quota configuration for `RateWindowLimiter`.
pub struct RateWindowConfig {
    default_limit: u64 = 100;
    default_window_ms: u64 = 1000;

    /// Constructs a validated `RateWindowConfig`, or `null` if limit or window is zero.
    static fn init(default_limit: u64, default_window_ms: u64): RateWindowConfig? {
        if (default_limit == 0 || default_window_ms == 0) {
            return null;
        }
        return RateWindowConfig{
            default_limit: default_limit,
            default_window_ms: default_window_ms,
        };
    }

    /// Returns `true` if both `default_limit` and `default_window_ms` are positive.
    fn isValid(): bool {
        return _.default_limit > 0 && _.default_window_ms > 0;
    }
}

/// Result returned by `checkAndConsume` and `peek`.
pub struct RateWindowDecision {
    allowed: bool = false;
    /// Maximum additional integer units immediately consumable at `now_ms`.
    remaining: u64 = 0;
    /// Ceiling of the weighted sliding-window effective usage at `now_ms`.
    effective_used: u64 = 0;
    /// Active quota limit applied to this tenant.
    limit: u64 = 0;
    /// Exact milliseconds to wait before a request of the same `cost` can succeed (`0` when allowed).
    retry_after_ms: u64 = 0;
    /// Timestamp (ms) at which the tenant's sliding window usage fully decays to zero.
    reset_at_ms: i64 = 0;
}

/// Inline fixed-size per-tenant sliding window state slot.
pub struct RateWindowSlot {
    occupied: bool = false;
    key_hash: u64 = 0;
    window_index: i64 = 0;
    prev_count: u64 = 0;
    curr_count: u64 = 0;
    last_seen_ms: i64 = 0;
    last_access_seq: u64 = 0;
    has_custom_quota: bool = false;
    custom_limit: u64 = 0;
    custom_window_ms: u64 = 0;
}

/// Cumulative zero-heap telemetry snapshot for `RateWindowLimiter`.
pub struct RateWindowStats {
    capacity: usize = RATE_WINDOW_MAX_KEYS;
    active_keys: usize = 0;
    total_requests: u64 = 0;
    total_allowed: u64 = 0;
    total_rejected: u64 = 0;
    total_evictions: u64 = 0;
}

// ============================================================================
// 2. Internal Pure-Integer Math Helpers (Zero Floating-Point Drift)
// ============================================================================

struct WindowProjection {
    window_index: i64 = 0;
    elapsed_ms: u64 = 0;
    prev_count: u64 = 0;
    curr_count: u64 = 0;
    eff_now_ms: i64 = 0;
}

fn clampNonNegative(ts_ms: i64): i64 {
    if (ts_ms < 0) {
        return 0;
    }
    return ts_ms;
}

fn ceilDivU128(num: u128, den: u128): u128 {
    if (den == 0) {
        return 0;
    }
    if (num == 0) {
        return 0;
    }
    return (num - 1) / den + 1;
}

fn clampU128ToU64(v: u128): u64 {
    const max_u64: u128 = 0xFFFFFFFFFFFFFFFF;
    if (v > max_u64) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return @as(u64, @intCast(v));
}

fn projectSlot(slot: RateWindowSlot, window_ms: u64, now_ms: i64, initialized: bool): WindowProjection {
    const clean_now: i64 = clampNonNegative(now_ms);
    const w_i64: i64 = @as(i64, @intCast(window_ms));
    if (!initialized) {
        const win_idx: i64 = @divTrunc(clean_now, w_i64);
        const elapsed: u64 = @as(u64, @intCast(clean_now - win_idx * w_i64));
        return WindowProjection{
            window_index: win_idx,
            elapsed_ms: elapsed,
            prev_count: 0,
            curr_count: 0,
            eff_now_ms: clean_now,
        };
    }

    // Clamp backwards clock jumps to last_seen_ms to preserve monotonic window semantics.
    const eff_now: i64 = if (clean_now < slot.last_seen_ms) slot.last_seen_ms else clean_now;
    const curr_win: i64 = @divTrunc(eff_now, w_i64);
    const elapsed: u64 = @as(u64, @intCast(eff_now - curr_win * w_i64));

    if (curr_win <= slot.window_index) {
        return WindowProjection{
            window_index: slot.window_index,
            elapsed_ms: elapsed,
            prev_count: slot.prev_count,
            curr_count: slot.curr_count,
            eff_now_ms: eff_now,
        };
    }
    if (curr_win == slot.window_index + 1) {
        return WindowProjection{
            window_index: curr_win,
            elapsed_ms: elapsed,
            prev_count: slot.curr_count,
            curr_count: 0,
            eff_now_ms: eff_now,
        };
    }
    // Two or more windows have elapsed: both previous and current counters expire to zero.
    return WindowProjection{
        window_index: curr_win,
        elapsed_ms: elapsed,
        prev_count: 0,
        curr_count: 0,
        eff_now_ms: eff_now,
    };
}

fn computeResetAtMs(win_idx: i64, window_ms: u64, prev_count: u64, curr_count: u64, eff_now_ms: i64): i64 {
    const w_i64: i64 = @as(i64, @intCast(window_ms));
    if (curr_count > 0) {
        return (win_idx + 2) * w_i64;
    }
    if (prev_count > 0) {
        return (win_idx + 1) * w_i64;
    }
    return eff_now_ms;
}

fn computeRetryAfterMs(
    prev_count: u64,
    curr_count: u64,
    elapsed_ms: u64,
    window_ms: u64,
    limit: u64,
    cost: u64,
): u64 {
    if (cost > limit) {
        return window_ms;
    }

    const w: u128 = @as(u128, window_ms);
    const l: u128 = @as(u128, limit);
    const c: u128 = @as(u128, cost);
    const prev: u128 = @as(u128, prev_count);
    const curr: u128 = @as(u128, curr_count);
    const rem_in_curr: u128 = if (w > @as(u128, elapsed_ms)) w - @as(u128, elapsed_ms) else 0;

    const scaled_used: u128 = prev * rem_in_curr + curr * w;
    const scaled_limit: u128 = l * w;
    const scaled_needed: u128 = scaled_used + c * w;

    if (scaled_needed <= scaled_limit) {
        return 0;
    }

    // Case 1: Current window still has room once prev_count decays sufficiently.
    if (curr + c <= l && prev > 0) {
        const excess: u128 = scaled_needed - scaled_limit;
        const wait_ms: u128 = ceilDivU128(excess, prev);
        return clampU128ToU64(wait_ms);
    }

    // Case 2: Must wait until next window starts, plus any decay of old curr_count (which becomes new prev_count).
    const to_next_win: u128 = rem_in_curr;
    if (curr == 0) {
        return clampU128ToU64(to_next_win);
    }
    const next_excess: u128 = (curr + c - l) * w;
    const extra_ms: u128 = ceilDivU128(next_excess, curr);
    return clampU128ToU64(to_next_win + extra_ms);
}

// ============================================================================
// 3. Multi-Tenant Sliding Window Counter Rate Limiter Engine
// ============================================================================

/// Zero-heap multi-tenant sliding window counter rate limiter with per-key quota overrides and LRU eviction.
pub struct RateWindowLimiter {
    cfg: RateWindowConfig = RateWindowConfig{},
    slots: [RATE_WINDOW_MAX_KEYS]RateWindowSlot = [RateWindowSlot{}; RATE_WINDOW_MAX_KEYS];
    active_keys: usize = 0;
    access_seq: u64 = 0;
    total_requests: u64 = 0;
    total_allowed: u64 = 0;
    total_rejected: u64 = 0;
    total_evictions: u64 = 0;

    /// Initializes a multi-tenant limiter with default `(limit, window_ms)`, or `null` if invalid.
    static fn init(default_limit: u64, default_window_ms: u64): RateWindowLimiter? {
        const cfg = RateWindowConfig.init(default_limit, default_window_ms) ?? return null;
        return RateWindowLimiter{
            cfg: cfg,
        };
    }

    /// Initializes a multi-tenant limiter from a validated `RateWindowConfig`, or `null` if invalid.
    static fn initWithConfig(cfg: RateWindowConfig): RateWindowLimiter? {
        if (!cfg.isValid()) {
            return null;
        }
        return RateWindowLimiter{
            cfg: cfg,
        };
    }

    /// Returns the number of currently occupied tenant slots (`0..=128`).
    fn activeCount(): usize {
        return _.active_keys;
    }

    /// Returns a zero-heap cumulative telemetry snapshot.
    fn stats(): RateWindowStats {
        return RateWindowStats{
            capacity: RATE_WINDOW_MAX_KEYS,
            active_keys: _.active_keys,
            total_requests: _.total_requests,
            total_allowed: _.total_allowed,
            total_rejected: _.total_rejected,
            total_evictions: _.total_evictions,
        };
    }

    /// Resets all tenant slots and cumulative statistics.
    fn resetAll(*_) {
        let i: usize = 0;
        while (i < RATE_WINDOW_MAX_KEYS) {
            _.slots[i] = RateWindowSlot{};
            i += 1;
        }
        _.active_keys = 0;
        _.access_seq = 0;
        _.total_requests = 0;
        _.total_allowed = 0;
        _.total_rejected = 0;
        _.total_evictions = 0;
    }

    fn findSlot(key_hash: u64): usize? {
        let i: usize = 0;
        while (i < RATE_WINDOW_MAX_KEYS) {
            if (_.slots[i].occupied && _.slots[i].key_hash == key_hash) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn effectiveLimitFor(slot: RateWindowSlot): u64 {
        if (slot.has_custom_quota && slot.custom_limit > 0) {
            return slot.custom_limit;
        }
        return _.cfg.default_limit;
    }

    fn effectiveWindowFor(slot: RateWindowSlot): u64 {
        if (slot.has_custom_quota && slot.custom_window_ms > 0) {
            return slot.custom_window_ms;
        }
        return _.cfg.default_window_ms;
    }

    fn isSlotExpiredAt(slot: RateWindowSlot, now_ms: i64): bool {
        if (!slot.occupied) {
            return true;
        }
        const clean_now: i64 = clampNonNegative(now_ms);
        if (clean_now <= slot.last_seen_ms) {
            return false;
        }
        const win_ms: u64 = _.effectiveWindowFor(slot);
        const w_i64: i64 = @as(i64, @intCast(win_ms));
        const curr_win: i64 = @divTrunc(clean_now, w_i64);
        return curr_win >= slot.window_index + 2;
    }

    /// Finds an empty slot, or evicts the oldest expired slot (preferred) / global LRU slot when full.
    fn acquireOrEvictSlot(*_, key_hash: u64, now_ms: i64): usize {
        if (existing := _.findSlot(key_hash)) {
            return existing;
        }

        // First pass: look for a free slot.
        let i: usize = 0;
        while (i < RATE_WINDOW_MAX_KEYS) {
            if (!_.slots[i].occupied) {
                _.slots[i] = RateWindowSlot{
                    occupied: true,
                    key_hash: key_hash,
                };
                _.active_keys += 1;
                return i;
            }
            i += 1;
        }

        // Table full: prefer evicting the least-recently-used expired cold slot first.
        let expired_victim: usize = RATE_WINDOW_MAX_KEYS;
        let expired_min_seq: u64 = 0xFFFFFFFFFFFFFFFF;
        let lru_victim: usize = 0;
        let lru_min_seq: u64 = 0xFFFFFFFFFFFFFFFF;

        let j: usize = 0;
        while (j < RATE_WINDOW_MAX_KEYS) {
            const seq: u64 = _.slots[j].last_access_seq;
            if (seq < lru_min_seq) {
                lru_min_seq = seq;
                lru_victim = j;
            }
            if (_.isSlotExpiredAt(_.slots[j], now_ms)) {
                if (seq < expired_min_seq) {
                    expired_min_seq = seq;
                    expired_victim = j;
                }
            }
            j += 1;
        }

        const victim: usize = if (expired_victim < RATE_WINDOW_MAX_KEYS) expired_victim else lru_victim;
        _.slots[victim] = RateWindowSlot{
            occupied: true,
            key_hash: key_hash,
        };
        _.total_evictions += 1;
        return victim;
    }

    /// Configures a per-tenant quota override (`limit`, `window_ms`) for `key_hash`.
    /// Returns `false` if `limit == 0` or `window_ms == 0`.
    fn setKeyQuota(*_, key_hash: u64, limit: u64, window_ms: u64): bool {
        if (limit == 0 || window_ms == 0) {
            return false;
        }
        const idx = _.acquireOrEvictSlot(key_hash, 0);
        _.access_seq += 1;
        const changed_window: bool = if (_.slots[idx].has_custom_quota)
            (_.slots[idx].custom_window_ms != window_ms)
        else
            (_.cfg.default_window_ms != window_ms);
        _.slots[idx].has_custom_quota = true;
        _.slots[idx].custom_limit = limit;
        _.slots[idx].custom_window_ms = window_ms;
        _.slots[idx].last_access_seq = _.access_seq;
        // If window duration changed on an active slot, align window_index safely and clear stale prev_count.
        if (changed_window && _.slots[idx].last_seen_ms > 0) {
            const w_i64: i64 = @as(i64, @intCast(window_ms));
            _.slots[idx].window_index = @divTrunc(_.slots[idx].last_seen_ms, w_i64);
            _.slots[idx].prev_count = 0;
        }
        return true;
    }

    /// Clears a tenant's custom quota override so it falls back to the limiter default quota.
    fn clearKeyQuota(*_, key_hash: u64): bool {
        const idx = _.findSlot(key_hash) ?? return false;
        _.slots[idx].has_custom_quota = false;
        _.slots[idx].custom_limit = 0;
        _.slots[idx].custom_window_ms = 0;
        return true;
    }

    /// Removes a tenant slot from the table. Returns `true` if found and removed.
    fn removeKey(*_, key_hash: u64): bool {
        const idx = _.findSlot(key_hash) ?? return false;
        _.slots[idx] = RateWindowSlot{};
        if (_.active_keys > 0) {
            _.active_keys -= 1;
        }
        return true;
    }

    /// Proactively purges all expired cold tenant slots (without custom quotas) at `now_ms`.
    fn purgeExpired(*_, now_ms: i64): usize {
        let purged: usize = 0;
        let i: usize = 0;
        while (i < RATE_WINDOW_MAX_KEYS) {
            if (_.slots[i].occupied && !_.slots[i].has_custom_quota && _.isSlotExpiredAt(_.slots[i], now_ms)) {
                _.slots[i] = RateWindowSlot{};
                if (_.active_keys > 0) {
                    _.active_keys -= 1;
                }
                purged += 1;
            }
            i += 1;
        }
        return purged;
    }

    /// Read-only probe of a tenant's sliding window state at `now_ms` without mutating counters or telemetry.
    fn peek(key_hash: u64, now_ms: i64): RateWindowDecision {
        const clean_now: i64 = clampNonNegative(now_ms);
        const idx_opt = _.findSlot(key_hash);
        if (idx_opt == null) {
            return RateWindowDecision{
                allowed: true,
                remaining: _.cfg.default_limit,
                effective_used: 0,
                limit: _.cfg.default_limit,
                retry_after_ms: 0,
                reset_at_ms: clean_now,
            };
        }

        const idx: usize = idx_opt ?? 0;
        const slot = _.slots[idx];
        const limit: u64 = _.effectiveLimitFor(slot);
        const window_ms: u64 = _.effectiveWindowFor(slot);
        const initialized: bool = (slot.last_seen_ms > 0) || (slot.prev_count > 0) || (slot.curr_count > 0);
        const proj = projectSlot(slot, window_ms, now_ms, initialized);

        const w: u128 = @as(u128, window_ms);
        const l: u128 = @as(u128, limit);
        const rem_in_curr: u128 = if (w > @as(u128, proj.elapsed_ms)) w - @as(u128, proj.elapsed_ms) else 0;
        const scaled_used: u128 = @as(u128, proj.prev_count) * rem_in_curr + @as(u128, proj.curr_count) * w;
        const scaled_limit: u128 = l * w;

        const eff_used: u64 = clampU128ToU64(ceilDivU128(scaled_used, w));
        const remaining: u64 = if (scaled_used < scaled_limit) clampU128ToU64((scaled_limit - scaled_used) / w) else 0;
        const can_take_one: bool = (scaled_used + w) <= scaled_limit;
        const retry_ms: u64 = if (can_take_one) 0 else computeRetryAfterMs(proj.prev_count, proj.curr_count, proj.elapsed_ms, window_ms, limit, 1);
        const reset_at: i64 = computeResetAtMs(proj.window_index, window_ms, proj.prev_count, proj.curr_count, proj.eff_now_ms);

        return RateWindowDecision{
            allowed: can_take_one,
            remaining: remaining,
            effective_used: eff_used,
            limit: limit,
            retry_after_ms: retry_ms,
            reset_at_ms: reset_at,
        };
    }

    /// Evaluates the hybrid sliding window counter for `key_hash` at `now_ms` and consumes `cost` units if allowed.
    fn checkAndConsume(*_, key_hash: u64, cost: u64, now_ms: i64): RateWindowDecision {
        _.total_requests += 1;
        const idx = _.acquireOrEvictSlot(key_hash, now_ms);
        _.access_seq += 1;

        const limit: u64 = _.effectiveLimitFor(_.slots[idx]);
        const window_ms: u64 = _.effectiveWindowFor(_.slots[idx]);
        const initialized: bool = (_.slots[idx].last_seen_ms > 0)
            || (_.slots[idx].prev_count > 0)
            || (_.slots[idx].curr_count > 0);
        const proj = projectSlot(_.slots[idx], window_ms, now_ms, initialized);

        // Advance slot window state and touch LRU sequence.
        _.slots[idx].window_index = proj.window_index;
        _.slots[idx].prev_count = proj.prev_count;
        _.slots[idx].curr_count = proj.curr_count;
        _.slots[idx].last_seen_ms = proj.eff_now_ms;
        _.slots[idx].last_access_seq = _.access_seq;

        const w: u128 = @as(u128, window_ms);
        const l: u128 = @as(u128, limit);
        const c: u128 = @as(u128, cost);
        const rem_in_curr: u128 = if (w > @as(u128, proj.elapsed_ms)) w - @as(u128, proj.elapsed_ms) else 0;
        const scaled_used: u128 = @as(u128, proj.prev_count) * rem_in_curr + @as(u128, proj.curr_count) * w;
        const scaled_limit: u128 = l * w;
        const scaled_after: u128 = scaled_used + c * w;

        if (scaled_after <= scaled_limit) {
            _.slots[idx].curr_count += cost;
            _.total_allowed += 1;

            const eff_used: u64 = clampU128ToU64(ceilDivU128(scaled_after, w));
            const remaining: u64 = clampU128ToU64((scaled_limit - scaled_after) / w);
            const reset_at: i64 = computeResetAtMs(
                proj.window_index,
                window_ms,
                _.slots[idx].prev_count,
                _.slots[idx].curr_count,
                proj.eff_now_ms,
            );
            return RateWindowDecision{
                allowed: true,
                remaining: remaining,
                effective_used: eff_used,
                limit: limit,
                retry_after_ms: 0,
                reset_at_ms: reset_at,
            };
        }

        _.total_rejected += 1;
        const eff_used: u64 = clampU128ToU64(ceilDivU128(scaled_used, w));
        const remaining: u64 = if (scaled_used < scaled_limit) clampU128ToU64((scaled_limit - scaled_used) / w) else 0;
        const retry_ms: u64 = computeRetryAfterMs(
            proj.prev_count,
            proj.curr_count,
            proj.elapsed_ms,
            window_ms,
            limit,
            cost,
        );
        const reset_at: i64 = computeResetAtMs(
            proj.window_index,
            window_ms,
            proj.prev_count,
            proj.curr_count,
            proj.eff_now_ms,
        );
        return RateWindowDecision{
            allowed: false,
            remaining: remaining,
            effective_used: eff_used,
            limit: limit,
            retry_after_ms: retry_ms,
            reset_at_ms: reset_at,
        };
    }
}
