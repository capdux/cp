// Source: Absorbed from /data/dev/bun/src/http/HTTPContext.cp | Synced: 2026-11-22T11:12:13Z (Weighted Sliding Window Rate Limiter)
// Pure-CP zero-allocation weighted sliding window counter & leaky bucket rate limiters (pure integer arithmetic).

pub struct SlidingWindowCounter {
    limit: u64;
    window_ms: u64;
    cur_window_start_ms: u64;
    prev_count: u64;
    cur_count: u64;

    pub static fn init(limit: u64, window_ms: u64, now_ms: u64): SlidingWindowCounter {
        const w: u64 = if (window_ms == 0) 1 else window_ms;
        return SlidingWindowCounter{
            limit: limit,
            window_ms: w,
            cur_window_start_ms: now_ms,
            prev_count: 0,
            cur_count: 0,
        };
    }

    fn advance(*_, now_ms: u64) {
        if (now_ms <= _.cur_window_start_ms) {
            return;
        }
        const elapsed = now_ms - _.cur_window_start_ms;
        if (elapsed < _.window_ms) {
            return;
        }
        const windows_passed = elapsed / _.window_ms;
        if (windows_passed == 1) {
            _.prev_count = _.cur_count;
        } else {
            _.prev_count = 0;
        }
        _.cur_count = 0;
        _.cur_window_start_ms += windows_passed * _.window_ms;
    }

    /// Estimated weighted request count in the sliding window `[now_ms - window_ms, now_ms]` (ceiling integer).
    pub fn estimatedCount(*_, now_ms: u64): u64 {
        _.advance(now_ms);
        let elapsed: u64 = 0;
        if (now_ms > _.cur_window_start_ms) {
            elapsed = now_ms - _.cur_window_start_ms;
        }
        if (elapsed >= _.window_ms) {
            return _.cur_count;
        }
        const overlap_ms = _.window_ms - elapsed;
        // Weighted previous count = ceil(prev_count * overlap_ms / window_ms) using u128 to prevent overflow
        const weighted_prev: u64 = (((_.prev_count as u128) * (overlap_ms as u128) + ((_.window_ms - 1) as u128)) / (_.window_ms as u128)) as u64;
        return _.cur_count + weighted_prev;
    }

    /// Try to admit `cost` requests at `now_ms`. Returns true if within `limit`, false if rate-limited.
    pub fn tryAcquire(*_, cost: u64, now_ms: u64): bool {
        if (cost == 0) {
            return true;
        }
        const est = _.estimatedCount(now_ms);
        if (cost > _.limit || est > _.limit - cost) {
            return false;
        }
        _.cur_count += cost;
        return true;
    }

    /// Remaining request budget at `now_ms`.
    pub fn remaining(*_, now_ms: u64): u64 {
        const est = _.estimatedCount(now_ms);
        if (est >= _.limit) {
            return 0;
        }
        return _.limit - est;
    }
}

/// Generic Cell Rate Algorithm (GCRA) virtual scheduling rate limiter (millisecond resolution).
/// Enforces smooth emission interval `emission_interval_ms` with burst tolerance `burst_tolerance_ms`.
pub struct SlidingGcraLimiter {
    emission_interval_ms: u64;
    burst_tolerance_ms: u64;
    tat_ms: u64; // Theoretical Arrival Time

    pub static fn init(emission_interval_ms: u64, burst_capacity: u64, now_ms: u64): SlidingGcraLimiter {
        const ei: u64 = if (emission_interval_ms == 0) 1 else emission_interval_ms;
        const burst: u64 = if (burst_capacity > 1) (burst_capacity - 1) * ei else 0;
        return SlidingGcraLimiter{
            emission_interval_ms: ei,
            burst_tolerance_ms: burst,
            tat_ms: now_ms,
        };
    }

    /// Attempt to admit 1 event at `now_ms`. Returns `0` if admitted, or retry-after milliseconds `> 0` if rejected.
    pub fn checkAndAcquire(*_, now_ms: u64): u64 {
        const allow_at: u64 = if (_.tat_ms > _.burst_tolerance_ms) _.tat_ms - _.burst_tolerance_ms else 0;
        if (now_ms < allow_at) {
            return allow_at - now_ms;
        }
        const new_tat = if (now_ms > _.tat_ms) now_ms + _.emission_interval_ms else _.tat_ms + _.emission_interval_ms;
        _.tat_ms = new_tat;
        return 0;
    }
}
