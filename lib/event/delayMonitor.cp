// Source: Absorbed from /data/dev/bun/src/runtime/timer/EventLoopDelayMonitor.cp | Synced: 2026-11-22T15:18:20Z
// Zero-allocation Event Loop Delay Monitor & 64-bucket logarithmic nanosecond delay histogram.

/// Number of logarithmic power-of-two nanosecond buckets in `DelayHistogram`.
pub const DELAY_HISTOGRAM_BUCKETS: usize = 64;

/// Default sampling resolution in milliseconds (matches Bun `EventLoopDelayMonitor`).
pub const DELAY_DEFAULT_RESOLUTION_MS: u32 = 10;

/// Nanoseconds per millisecond.
pub const DELAY_NS_PER_MS: u64 = 1_000_000;

/// Error set for `EventLoopDelayMonitor` and `DelayHistogram`.
pub err DelayMonitorError {
    InvalidResolution,
    InvalidPercentile,
}

/// Zero-allocation 64-bucket logarithmic nanosecond delay histogram.
/// Bucket 0 holds `0ns`, bucket `k` (`1 <= k < 63`) holds `[2^(k-1), 2^k - 1]`,
/// and bucket 63 holds `>= 2^62`.
pub struct DelayHistogram {
    buckets: [64]u64 = [0; 64],
    count: u64 = 0,
    min_ns: u64 = 0,
    max_ns: u64 = 0,
    sum_ns: u128 = 0,
    exceeds_count: u64 = 0,
    max_recordable_ns: u64 = 0,

    static fn init(): DelayHistogram {
        return DelayHistogram{};
    }

    static fn initWithMax(max_recordable_ns: u64): DelayHistogram {
        return DelayHistogram{
            max_recordable_ns: max_recordable_ns,
        };
    }

    /// Compute the logarithmic bucket index for a nanosecond sample.
    static fn bucketIndex(ns: u64): usize {
        if (ns == 0) return 0;
        let idx: usize = 1;
        let v = ns >> 1;
        while (v > 0 && idx < DELAY_HISTOGRAM_BUCKETS - 1) {
            v = v >> 1;
            idx += 1;
        }
        return idx;
    }

    /// Compute the inclusive upper bound in nanoseconds for bucket `idx`.
    static fn bucketUpperBoundNs(idx: usize): u64 {
        if (idx == 0) return 0;
        if (idx >= DELAY_HISTOGRAM_BUCKETS - 1) {
            return u64(1) << 62;
        }
        const shift: u6 = u6(idx);
        return (u64(1) << shift) - 1;
    }

    /// Record a single delay sample in nanoseconds.
    /// If `max_recordable_ns > 0` and `delay_ns > max_recordable_ns`, increments `exceeds_count`
    /// and clamps the recorded sample to `max_recordable_ns`.
    fn record(*_, delay_ns: u64) {
        let sample = delay_ns;
        if (_.max_recordable_ns > 0 && sample > _.max_recordable_ns) {
            _.exceeds_count += 1;
            sample = _.max_recordable_ns;
        }
        const b = DelayHistogram.bucketIndex(sample);
        _.buckets[b] += 1;
        if (_.count == 0) {
            _.min_ns = sample;
            _.max_ns = sample;
        } else {
            if (sample < _.min_ns) _.min_ns = sample;
            if (sample > _.max_ns) _.max_ns = sample;
        }
        _.count += 1;
        _.sum_ns += u128(sample);
    }

    /// Reset all recorded counters and buckets while keeping `max_recordable_ns`.
    fn reset(*_) {
        for (i of 0..DELAY_HISTOGRAM_BUCKETS) {
            _.buckets[i] = 0;
        }
        _.count = 0;
        _.min_ns = 0;
        _.max_ns = 0;
        _.sum_ns = 0;
        _.exceeds_count = 0;
    }

    /// Return integer arithmetic mean in nanoseconds (0 when empty).
    fn meanNs(): u64 {
        if (_.count == 0) return 0;
        return u64(_.sum_ns / u128(_.count));
    }

    /// Return the upper-bound estimate in nanoseconds at percentile `pct` (`0.0 <= pct <= 100.0`).
    /// Returns 0 when no samples have been recorded.
    fn percentile(pct: f64): u64 ! DelayMonitorError {
        if (pct < 0.0 || pct > 100.0 || pct != pct) {
            return err.InvalidPercentile;
        }
        if (_.count == 0) return 0;
        if (pct == 0.0) return _.min_ns;
        if (pct == 100.0) return _.max_ns;

        const target_f: f64 = (pct / 100.0) * @as(f64, @floatFromInt(_.count));
        let target: u64 = @intFromFloat(target_f);
        if (@as(f64, @floatFromInt(target)) < target_f) {
            target += 1;
        }
        if (target == 0) {
            target = 1;
        }
        if (target > _.count) {
            target = _.count;
        }

        let cum: u64 = 0;
        for (i of 0..DELAY_HISTOGRAM_BUCKETS) {
            cum += _.buckets[i];
            if (cum >= target) {
                const upper = DelayHistogram.bucketUpperBoundNs(i);
                return if (upper > _.max_ns) _.max_ns else upper;
            }
        }
        return _.max_ns;
    }
}

/// Deterministic event loop delay monitor absorbed from Bun `EventLoopDelayMonitor`.
/// Tracks scheduling jitter `(actual_interval - expected_resolution)` across periodic timer ticks
/// without any dynamic heap allocation.
pub struct EventLoopDelayMonitor {
    resolution_ms: u32 = DELAY_DEFAULT_RESOLUTION_MS,
    last_fire_ns: u64 = 0,
    next_fire_ns: u64 = 0,
    enabled: bool = false,
    histogram: DelayHistogram = DelayHistogram{},

    /// Create a disabled monitor with `resolution_ms` (must be >= 1).
    static fn init(resolution_ms: u32): EventLoopDelayMonitor ! DelayMonitorError {
        if (resolution_ms == 0) {
            return err.InvalidResolution;
        }
        return EventLoopDelayMonitor{
            resolution_ms: resolution_ms,
        };
    }

    /// Create a disabled monitor with an upper clamp bound on recorded delay samples.
    static fn initWithMax(resolution_ms: u32, max_recordable_ns: u64): EventLoopDelayMonitor ! DelayMonitorError {
        if (resolution_ms == 0) {
            return err.InvalidResolution;
        }
        return EventLoopDelayMonitor{
            resolution_ms: resolution_ms,
            histogram: DelayHistogram.initWithMax(max_recordable_ns),
        };
    }

    /// Expected tick interval in nanoseconds (`resolution_ms * 1_000_000`).
    fn expectedIntervalNs(): u64 {
        return u64(_.resolution_ms) * DELAY_NS_PER_MS;
    }

    /// Enable monitoring at monotonic timestamp `now_ns`.
    /// Returns `true` if transitioned from disabled -> enabled, `false` if already enabled.
    fn enable(*_, now_ns: u64): bool {
        if (_.enabled) return false;
        _.enabled = true;
        _.last_fire_ns = 0;
        _.next_fire_ns = now_ns + _.expectedIntervalNs();
        return true;
    }

    /// Disable monitoring and clear timer schedule state (preserves recorded histogram).
    /// Returns `true` if transitioned from enabled -> disabled, `false` if already disabled.
    fn disable(*_): bool {
        if (!_.enabled) return false;
        _.enabled = false;
        _.last_fire_ns = 0;
        _.next_fire_ns = 0;
        return true;
    }

    /// Return whether the monitor is currently active.
    fn isEnabled(): bool {
        return _.enabled;
    }

    /// Return whether the monitor timer is due to fire at `now_ns`.
    fn shouldFire(now_ns: u64): bool {
        return _.enabled && now_ns >= _.next_fire_ns;
    }

    /// Handle a timer fire at monotonic timestamp `now_ns` (matches Bun `EventLoopDelayMonitor.onFire`).
    /// - When disabled, returns `null`.
    /// - On the first fire after `enable()`, establishes the baseline `last_fire_ns` and returns `null`.
    /// - On subsequent fires, computes `actual_ns = now_ns - last_fire_ns` (clamped at 0 if clock stepped back),
    ///   records `delay_ns = actual_ns - expected_ns` if `actual_ns > expected_ns` (else `0`),
    ///   reschedules `next_fire_ns = now_ns + expected_ns`, and returns the measured `delay_ns`.
    fn onFire(*_, now_ns: u64): u64? {
        if (!_.enabled) return null;

        const expected_ns = _.expectedIntervalNs();
        let measured_delay: u64? = null;

        if (_.last_fire_ns > 0) {
            if (now_ns > _.last_fire_ns) {
                const actual_ns = now_ns - _.last_fire_ns;
                const delay_ns: u64 = if (actual_ns > expected_ns) (actual_ns - expected_ns) else 0;
                _.histogram.record(delay_ns);
                measured_delay = delay_ns;
            } else {
                _.histogram.record(0);
                measured_delay = 0;
            }
        }

        _.last_fire_ns = now_ns;
        _.next_fire_ns = now_ns + expected_ns;
        return measured_delay;
    }

    /// Reset histogram statistics without changing enabled state or schedule.
    fn reset(*_) {
        _.histogram.reset();
    }
}
