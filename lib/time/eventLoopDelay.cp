// Source: Absorbed from /data/dev/bun/src/runtime/timer/EventLoopDelayMonitor.cp | Synced: 2026-11-22T20:01:48Z (Event loop delay monitor & zero-alloc log2 nanosecond delay histogram aligned with Node perf_hooks.monitorEventLoopDelay)
// Pure-CP zero-allocation event loop delay monitor with fixed-bucket log2 nanosecond histogram.

/// Default sampling resolution in milliseconds (aligned with Bun `EventLoopDelayMonitor` & Node `perf_hooks`).
pub const EVENT_LOOP_DELAY_DEFAULT_RESOLUTION_MS: u32 = 10;

/// Nanoseconds per millisecond.
pub const EVENT_LOOP_DELAY_NS_PER_MS: u64 = 1_000_000;

/// Default maximum recordable delay (1 hour in nanoseconds).
pub const EVENT_LOOP_DELAY_MAX_RECORDABLE_NS: u64 = 3_600_000_000_000;

/// Number of fixed log2 histogram buckets (bucket 0 = 0ns, bucket k = [2^(k-1), 2^k - 1] ns).
pub const EVENT_LOOP_DELAY_HIST_BUCKETS: usize = 44;

/// Error set for `EventLoopDelayMonitor`.
pub err EventLoopDelayError {
    InvalidResolution,
    NotEnabled,
    InvalidPercentile,
}

/// Result returned by each `EventLoopDelayMonitor.onFire(now_ns)` tick.
pub struct EventLoopDelaySample {
    recorded: bool = false,
    delay_ns: u64 = 0,
    next_fire_ns: u64 = 0,
    exceeded_cap: bool = false,
}

/// Immutable summary snapshot of recorded event loop delays.
pub struct EventLoopDelayStats {
    count: u64 = 0,
    exceeds: u64 = 0,
    min_ns: u64 = 0,
    max_ns: u64 = 0,
    mean_ns: u64 = 0,
    p50_ns: u64 = 0,
    p95_ns: u64 = 0,
    p99_ns: u64 = 0,
}

fn delayBucketIndex(val: u64): usize {
    if (val == 0) return 0;
    let v = val;
    let idx: usize = 1;
    while (v > 1 && idx + 1 < EVENT_LOOP_DELAY_HIST_BUCKETS) {
        v = v >> 1;
        idx += 1;
    }
    return idx;
}

fn delayBucketUpperBound(idx: usize): u64 {
    if (idx == 0) return 0;
    const shift: u6 = @as(u6, @intCast(idx - 1));
    const low: u64 = @as(u64, 1) << shift;
    if (idx == 1) return 1;
    return low + (low - 1);
}

/// Zero-allocation event loop delay monitor aligned with Bun `EventLoopDelayMonitor.cp`.
pub struct EventLoopDelayMonitor {
    resolution_ms: u32 = EVENT_LOOP_DELAY_DEFAULT_RESOLUTION_MS,
    max_recordable_ns: u64 = EVENT_LOOP_DELAY_MAX_RECORDABLE_NS,
    last_fire_ns: u64 = 0,
    next_fire_ns: u64 = 0,
    enabled: bool = false,
    count: u64 = 0,
    exceeds: u64 = 0,
    sum_ns: u128 = 0,
    min_ns: u64 = 0,
    max_ns: u64 = 0,
    buckets: [EVENT_LOOP_DELAY_HIST_BUCKETS]u64 = [0; EVENT_LOOP_DELAY_HIST_BUCKETS],

    /// Creates a monitor with the given sampling resolution in milliseconds (`resolution_ms >= 1`).
    static fn init(resolution_ms: u32): EventLoopDelayMonitor ! EventLoopDelayError {
        return EventLoopDelayMonitor.initWithMax(resolution_ms, EVENT_LOOP_DELAY_MAX_RECORDABLE_NS);
    }

    /// Creates a monitor with custom sampling resolution and maximum recordable delay cap.
    static fn initWithMax(
        resolution_ms: u32,
        max_recordable_ns: u64,
    ): EventLoopDelayMonitor ! EventLoopDelayError {
        if (resolution_ms == 0) return EventLoopDelayError.InvalidResolution;
        const cap = if (max_recordable_ns == 0) EVENT_LOOP_DELAY_MAX_RECORDABLE_NS else max_recordable_ns;
        return EventLoopDelayMonitor{
            resolution_ms: resolution_ms,
            max_recordable_ns: cap,
        };
    }

    /// Expected tick interval in nanoseconds (`resolution_ms * 1_000_000`).
    fn expectedIntervalNs(): u64 {
        return @as(u64, _.resolution_ms) * EVENT_LOOP_DELAY_NS_PER_MS;
    }

    /// Enables monitoring and returns the first scheduled timer deadline (`now_ns + resolution`).
    /// Idempotent when already enabled (returns existing `next_fire_ns`).
    fn enable(*_, now_ns: u64): u64 {
        if (_.enabled) return _.next_fire_ns;
        _.enabled = true;
        _.last_fire_ns = 0;
        _.next_fire_ns = now_ns + _.expectedIntervalNs();
        return _.next_fire_ns;
    }

    /// Disables monitoring and clears timer scheduling state while preserving collected histogram stats.
    fn disable(*_) {
        if (!_.enabled) return;
        _.enabled = false;
        _.last_fire_ns = 0;
        _.next_fire_ns = 0;
    }

    /// Returns true if the monitor is currently active.
    fn isEnabled(): bool {
        return _.enabled;
    }

    /// Clears all recorded histogram buckets and summary statistics without changing enabled state.
    fn reset(*_) {
        _.count = 0;
        _.exceeds = 0;
        _.sum_ns = 0;
        _.min_ns = 0;
        _.max_ns = 0;
        let i: usize = 0;
        while (i < EVENT_LOOP_DELAY_HIST_BUCKETS) {
            _.buckets[i] = 0;
            i += 1;
        }
    }

    /// Directly records a single nanosecond delay sample into the histogram.
    /// Delays above `max_recordable_ns` increment `exceeds` and are clamped to `max_recordable_ns`.
    fn recordDelay(*_, delay_ns: u64): bool {
        let clamped = delay_ns;
        let exceeded = false;
        if (clamped > _.max_recordable_ns) {
            clamped = _.max_recordable_ns;
            _.exceeds += 1;
            exceeded = true;
        }
        if (_.count == 0) {
            _.min_ns = clamped;
            _.max_ns = clamped;
        } else {
            if (clamped < _.min_ns) _.min_ns = clamped;
            if (clamped > _.max_ns) _.max_ns = clamped;
        }
        _.count += 1;
        _.sum_ns += @as(u128, clamped);
        const b = delayBucketIndex(clamped);
        _.buckets[b] += 1;
        return exceeded;
    }

    /// Advances the monitor on each timer firing at monotonic timestamp `now_ns`.
    /// Aligned with Bun `EventLoopDelayMonitor.onFire`:
    /// - First firing establishes `last_fire_ns` baseline without recording a delay sample.
    /// - Subsequent firings compute `actual_ns = now_ns - last_fire_ns` (clamped to 0 on clock regression)
    ///   and record `actual_ns - expected_ns` whenever `actual_ns > expected_ns`.
    fn onFire(*_, now_ns: u64): EventLoopDelaySample ! EventLoopDelayError {
        if (!_.enabled) return EventLoopDelayError.NotEnabled;

        const expected_ns = _.expectedIntervalNs();
        let recorded = false;
        let delay_ns: u64 = 0;
        let exceeded = false;

        if (_.last_fire_ns > 0) {
            const actual_ns = if (now_ns > _.last_fire_ns) (now_ns - _.last_fire_ns) else 0;
            if (actual_ns > expected_ns) {
                delay_ns = actual_ns - expected_ns;
                exceeded = _.recordDelay(delay_ns);
                recorded = true;
            }
        }

        _.last_fire_ns = now_ns;
        _.next_fire_ns = now_ns + expected_ns;

        return EventLoopDelaySample{
            recorded: recorded,
            delay_ns: delay_ns,
            next_fire_ns: _.next_fire_ns,
            exceeded_cap: exceeded,
        };
    }

    /// Arithmetic mean of recorded delays in nanoseconds (`0` when empty).
    fn meanNs(): u64 {
        if (_.count == 0) return 0;
        return @as(u64, @intCast(_.sum_ns / @as(u128, _.count)));
    }

    /// Computes approximate percentile `p` (`0.0 <= p <= 100.0`) from the log2 histogram buckets.
    fn percentileNs(p: f64): u64 ! EventLoopDelayError {
        if (p < 0.0 || p > 100.0) return EventLoopDelayError.InvalidPercentile;
        if (_.count == 0) return 0;
        if (p == 0.0) return _.min_ns;
        if (p == 100.0) return _.max_ns;

        const scaled = (@as(f64, @floatFromInt(_.count)) * p) / 100.0;
        const raw_target: u64 = @as(u64, @intFromFloat(scaled));
        const clamped_low: u64 = if (raw_target == 0) 1 else raw_target;
        const target: u64 = if (clamped_low > _.count) _.count else clamped_low;

        let acc: u64 = 0;
        let i: usize = 0;
        while (i < EVENT_LOOP_DELAY_HIST_BUCKETS) {
            acc += _.buckets[i];
            if (acc >= target) {
                const upper = delayBucketUpperBound(i);
                return if (upper > _.max_ns) _.max_ns else upper;
            }
            i += 1;
        }
        return _.max_ns;
    }

    /// Returns a full summary snapshot (count, exceeds, min, max, mean, p50, p95, p99).
    fn stats(): EventLoopDelayStats {
        const p50: u64 = _.percentileNs(50.0) catch @as(u64, 0);
        const p95: u64 = _.percentileNs(95.0) catch @as(u64, 0);
        const p99: u64 = _.percentileNs(99.0) catch @as(u64, 0);
        return EventLoopDelayStats{
            count: _.count,
            exceeds: _.exceeds,
            min_ns: _.min_ns,
            max_ns: _.max_ns,
            mean_ns: _.meanNs(),
            p50_ns: p50,
            p95_ns: p95,
            p99_ns: p99,
        };
    }
}
