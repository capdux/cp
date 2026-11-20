// Source: cp/lib/supervisor/resilience.cp (Backoff Jitter, Bulkhead & Retry Budget)
// Pure CP distributed resilience primitives: deterministic SplitMix64 BackoffPolicy,
// concurrency-limiting Bulkhead, and Envoy/Finagle sliding-window RetryBudget.

import { SupervisorError } from "./strategy.cp";

/// Backoff delay calculation mode.
pub enum BackoffKind: u8 {
    Fixed = 0,
    Linear = 1,
    Exponential = 2,
    FullJitter = 3,
    DecorrelatedJitter = 4,
}

zig {
    const SM64_GOLDEN: u64 = 0x9E3779B97F4A7C15;
    const SM64_MUL1: u64 = 0xBF58476D1CE4E5B9;
    const SM64_MUL2: u64 = 0x94D049BB133111EB;
}

/// Deterministic retry backoff & jitter policy backed by SplitMix64 PRNG.
pub struct BackoffPolicy {
    kind: BackoffKind = BackoffKind.Exponential,
    base_ms: u64 = 100,
    max_ms: u64 = 30000,
    multiplier_x100: u64 = 200,
    seed: u64 = 0x517CC1B727220A95,

    /// Initializes a `BackoffPolicy`.
    static fn init(
        kind: BackoffKind,
        base_ms: u64,
        max_ms: u64,
        multiplier_x100: u64,
        seed: u64,
    ): BackoffPolicy {
        return BackoffPolicy{
            kind: kind,
            base_ms: base_ms,
            max_ms: max_ms,
            multiplier_x100: multiplier_x100,
            seed: seed,
        };
    }

    /// Resets the internal SplitMix64 PRNG state to `new_seed`.
    fn resetSeed(*_, new_seed: u64) {
        _.seed = new_seed;
    }

    fn nextRandU64(*_): u64 {
        let out: u64 = 0;
        zig {
            self.seed +%= SM64_GOLDEN;
            var z: u64 = self.seed;
            z = (z ^ (z >> 30)) *% SM64_MUL1;
            z = (z ^ (z >> 27)) *% SM64_MUL2;
            out = z ^ (z >> 31);
        }
        return out;
    }

    fn computeExpCap(attempt: u32): u64 {
        if (_.max_ms == 0 || _.base_ms == 0) {
            return 0;
        }
        let cur: u128 = _.base_ms;
        const max_128: u128 = _.max_ms;
        if (cur >= max_128) {
            return _.max_ms;
        }
        const mult: u128 = _.multiplier_x100;
        let step: u32 = 0;
        while (step < attempt) : (step += 1) {
            cur = (cur * mult) / 100;
            if (cur >= max_128) {
                return _.max_ms;
            }
            if (cur == 0) {
                return 0;
            }
        }
        return cur as u64;
    }

    /// Computes the next backoff delay in milliseconds for `attempt` (0-indexed) and `prev_delay_ms`,
    /// always bounded by `max_ms`.
    fn nextDelayMs(*_, attempt: u32, prev_delay_ms: u64): u64 {
        if (_.max_ms == 0) {
            return 0;
        }
        switch (_.kind) {
            .Fixed => {
                return if (_.base_ms > _.max_ms) _.max_ms else _.base_ms;
            },
            .Linear => {
                const factor: u128 = (attempt as u128) + 1;
                const val: u128 = (_.base_ms as u128) * factor;
                const max_128: u128 = _.max_ms;
                return if (val > max_128) _.max_ms else val as u64;
            },
            .Exponential => {
                return _.computeExpCap(attempt);
            },
            .FullJitter => {
                const cap = _.computeExpCap(attempt);
                if (cap == 0) {
                    return 0;
                }
                const rnd = _.nextRandU64() as u128;
                const bound: u128 = (cap as u128) + 1;
                return (rnd % bound) as u64;
            },
            .DecorrelatedJitter => {
                const low: u64 = if (_.base_ms > _.max_ms) _.max_ms else _.base_ms;
                const prev: u64 = if (prev_delay_ms < low) low else prev_delay_ms;
                const hi_128: u128 = (prev as u128) * 3;
                const low_128: u128 = low;
                if (hi_128 <= low_128) {
                    return low;
                }
                const span: u128 = (hi_128 - low_128) + 1;
                const rnd: u128 = _.nextRandU64() as u128;
                const raw: u128 = low_128 + (rnd % span);
                const max_128: u128 = _.max_ms;
                return if (raw > max_128) _.max_ms else raw as u64;
            },
        }
    }
}

/// Concurrency bulkhead limiter tracking active execution slots, optional queue room,
/// rejection counts, and high-watermark `peakActive`.
pub struct Bulkhead {
    max_concurrent: u32 = 1,
    max_queue: u32 = 0,
    active: u32 = 0,
    queued: u32 = 0,
    rejected: u64 = 0,
    peak_active: u32 = 0,

    /// Initializes a `Bulkhead` permitting at most `max_concurrent` active slots and `max_queue` queued waiters.
    static fn init(max_concurrent: u32, max_queue: u32): Bulkhead {
        return Bulkhead{
            max_concurrent: max_concurrent,
            max_queue: max_queue,
            active: 0,
            queued: 0,
            rejected: 0,
            peak_active: 0,
        };
    }

    /// Attempts to acquire an active execution slot immediately.
    /// Increments `rejectedCount()` and returns `false` when at `max_concurrent` capacity.
    fn tryAcquire(*_): bool {
        if (_.active < _.max_concurrent) {
            _.active += 1;
            if (_.active > _.peak_active) {
                _.peak_active = _.active;
            }
            return true;
        }
        _.rejected += 1;
        return false;
    }

    /// Acquires an active slot or returns `SupervisorError.BulkheadFull`.
    fn acquire(*_): void ! SupervisorError {
        if (!_.tryAcquire()) {
            return SupervisorError.BulkheadFull;
        }
    }

    /// Attempts to reserve a waiting queue slot when active slots are saturated.
    fn tryEnqueue(*_): bool {
        if (_.queued < _.max_queue) {
            _.queued += 1;
            return true;
        }
        _.rejected += 1;
        return false;
    }

    /// Releases one active slot (promoting one queued waiter if any are waiting).
    /// Named `releaseSlot` to avoid ARC `.release()` rewrite.
    fn releaseSlot(*_) {
        if (_.active == 0) {
            return;
        }
        if (_.queued > 0) {
            _.queued -= 1;
        } else {
            _.active -= 1;
        }
    }

    /// Returns the number of currently active slots.
    fn activeCount(): u32 {
        return _.active;
    }

    /// Returns the number of remaining immediate execution slots.
    fn availableSlots(): u32 {
        return if (_.max_concurrent > _.active) _.max_concurrent - _.active else 0;
    }

    /// Returns the number of currently queued waiters.
    fn queuedCount(): u32 {
        return _.queued;
    }

    /// Returns the total number of rejected acquire/enqueue attempts.
    fn rejectedCount(): u64 {
        return _.rejected;
    }

    /// Returns the historical peak number of concurrent active slots.
    fn peakActive(): u32 {
        return _.peak_active;
    }
}

struct BudgetBucket {
    bucket_id: u64 = 0,
    valid: bool = false,
    requests: u64 = 0,
    retries: u64 = 0,
}

/// Sliding-window retry budget guard (Envoy/Finagle style) preventing retry storms across agent call chains.
pub struct RetryBudget {
    window_ms: u64 = 10000,
    min_retries_per_window: u32 = 3,
    retry_ratio_bps: u32 = 2000,
    bucket_ms: u64 = 80,
    num_buckets: usize = 125,
    buckets: [128]BudgetBucket = [BudgetBucket{}; 128],
    total_requests: u64 = 0,
    total_retries: u64 = 0,
    rejected_retries: u64 = 0,

    /// Initializes a `RetryBudget` with sliding window `window_ms` (clamped >= 1),
    /// minimum guaranteed retries `min_retries_per_window`, and ratio cap `retry_ratio_bps` (10000 = 100%).
    static fn init(
        window_ms: u64,
        min_retries_per_window: u32,
        retry_ratio_bps: u32,
    ): RetryBudget {
        const eff_window: u64 = if (window_ms == 0) 1 else window_ms;
        let b_ms: u64 = 1;
        let n_buckets: usize = 125;
        if (eff_window <= 128) {
            b_ms = 1;
            n_buckets = eff_window as usize;
        } else {
            b_ms = (eff_window + 124) / 125;
            n_buckets = 125;
        }
        return RetryBudget{
            window_ms: eff_window,
            min_retries_per_window: min_retries_per_window,
            retry_ratio_bps: retry_ratio_bps,
            bucket_ms: b_ms,
            num_buckets: n_buckets,
            buckets: [BudgetBucket{}; 128],
            total_requests: 0,
            total_retries: 0,
            rejected_retries: 0,
        };
    }

    fn prune(*_, now_ms: u64) {
        const cur_id: u64 = now_ms / _.bucket_ms;
        const nb_u64: u64 = _.num_buckets as u64;
        let i: usize = 0;
        while (i < _.num_buckets) : (i += 1) {
            if (_.buckets[i].valid) {
                const bid = _.buckets[i].bucket_id;
                if (bid > cur_id || (bid + nb_u64) <= cur_id) {
                    _.total_requests -= _.buckets[i].requests;
                    _.total_retries -= _.buckets[i].retries;
                    _.buckets[i].valid = false;
                    _.buckets[i].requests = 0;
                    _.buckets[i].retries = 0;
                }
            }
        }
    }

    fn touchBucket(*_, now_ms: u64): usize {
        _.prune(now_ms);
        const cur_id: u64 = now_ms / _.bucket_ms;
        const idx: usize = (cur_id % (_.num_buckets as u64)) as usize;
        if (!_.buckets[idx].valid || _.buckets[idx].bucket_id != cur_id) {
            if (_.buckets[idx].valid) {
                _.total_requests -= _.buckets[idx].requests;
                _.total_retries -= _.buckets[idx].retries;
            }
            _.buckets[idx].bucket_id = cur_id;
            _.buckets[idx].valid = true;
            _.buckets[idx].requests = 0;
            _.buckets[idx].retries = 0;
        }
        return idx;
    }

    /// Records a primary (non-retry) request at `now_ms`, expanding retry capacity proportionally.
    fn recordRequest(*_, now_ms: u64) {
        const idx = _.touchBucket(now_ms);
        _.buckets[idx].requests += 1;
        _.total_requests += 1;
    }

    fn maxAllowedRetries(): u64 {
        const ratio_cap: u128 = ((_.total_requests as u128) * (_.retry_ratio_bps as u128)) / 10000;
        const min_cap: u128 = _.min_retries_per_window as u128;
        const eff: u128 = if (ratio_cap > min_cap) ratio_cap else min_cap;
        return eff as u64;
    }

    /// Attempts to consume one retry token at `now_ms`.
    /// Returns `true` if within `max(min_retries_per_window, window_requests * retry_ratio_bps / 10000)`,
    /// or `false` if the retry budget is exhausted.
    fn tryConsumeRetry(*_, now_ms: u64): bool {
        const idx = _.touchBucket(now_ms);
        const allowed = _.maxAllowedRetries();
        if (_.total_retries >= allowed) {
            _.rejected_retries += 1;
            return false;
        }
        _.buckets[idx].retries += 1;
        _.total_retries += 1;
        return true;
    }

    /// Returns the current retry-to-request ratio in basis points (`retries * 10000 / requests`) at `now_ms`.
    fn currentRatioBps(*_, now_ms: u64): u32 {
        _.prune(now_ms);
        if (_.total_requests == 0) {
            return if (_.total_retries == 0) 0 else 10000;
        }
        const bps: u128 = ((_.total_retries as u128) * 10000) / (_.total_requests as u128);
        return if (bps > 0xFFFFFFFF) 0xFFFFFFFF else bps as u32;
    }

    /// Returns the active primary request count in the current sliding window.
    fn windowRequests(*_, now_ms: u64): u64 {
        _.prune(now_ms);
        return _.total_requests;
    }

    /// Returns the active consumed retry count in the current sliding window.
    fn windowRetries(*_, now_ms: u64): u64 {
        _.prune(now_ms);
        return _.total_retries;
    }

    /// Returns the remaining retry tokens available in the current sliding window.
    fn remainingRetries(*_, now_ms: u64): u64 {
        _.prune(now_ms);
        const allowed = _.maxAllowedRetries();
        return if (allowed > _.total_retries) allowed - _.total_retries else 0;
    }

    /// Returns the cumulative number of rejected retry attempts.
    fn rejectedRetries(): u64 {
        return _.rejected_retries;
    }
}
