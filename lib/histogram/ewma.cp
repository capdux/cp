// Source: cp/lib/histogram/ewma.cp (Exponentially Weighted Moving Average & Sliding Window Counter)
// Pure CP zero-C-dependency rate estimators:
// - `Ewma`: 1m/5m/15m standard Dropwizard/Linux EWMA or custom-alpha moving rate estimator.
// - `SlidingWindowCounter`: fixed ring-buffer time-slice counter with single-shot allocation.

const std = @import("std");
import { HistogramError } from "./hdr.cp";

/// Exponentially Weighted Moving Average (EWMA) rate estimator.
pub struct Ewma {
    alpha: f64,
    interval_secs: f64,
    rate_val: f64 = 0.0,
    uncounted: u64 = 0,
    initialized: bool = false,

    /// Create an EWMA with explicit smoothing factor `alpha` in `(0.0, 1.0]`
    /// and tick interval `interval_secs > 0.0`.
    pub fn init(alpha: f64, interval_secs: f64): Ewma ! HistogramError {
        if (alpha <= 0.0 || alpha > 1.0) {
            return HistogramError.InvalidPrecision;
        }
        if (interval_secs <= 0.0) {
            return HistogramError.InvalidWindow;
        }
        return Ewma{
            alpha: alpha,
            interval_secs: interval_secs,
            rate_val: 0.0,
            uncounted: 0,
            initialized: false,
        };
    }

    /// Standard 1-minute EWMA with 5-second tick interval (`alpha = 1 - exp(-5/60)`).
    pub fn initOneMinute(): Ewma {
        return Ewma{
            alpha: 1.0 - @exp(-5.0 / 60.0),
            interval_secs: 5.0,
            rate_val: 0.0,
            uncounted: 0,
            initialized: false,
        };
    }

    /// Standard 5-minute EWMA with 5-second tick interval (`alpha = 1 - exp(-5/300)`).
    pub fn initFiveMinutes(): Ewma {
        return Ewma{
            alpha: 1.0 - @exp(-5.0 / 300.0),
            interval_secs: 5.0,
            rate_val: 0.0,
            uncounted: 0,
            initialized: false,
        };
    }

    /// Standard 15-minute EWMA with 5-second tick interval (`alpha = 1 - exp(-5/900)`).
    pub fn initFifteenMinutes(): Ewma {
        return Ewma{
            alpha: 1.0 - @exp(-5.0 / 900.0),
            interval_secs: 5.0,
            rate_val: 0.0,
            uncounted: 0,
            initialized: false,
        };
    }

    /// Add `n` events to the current uncounted interval accumulator.
    pub fn update(*_, n: u64) {
        _.uncounted += n;
    }

    /// Advance one tick interval, folding uncounted events into the EWMA rate.
    pub fn tick(*_) {
        const count = _.uncounted;
        _.uncounted = 0;
        const instant_rate = (count as f64) / _.interval_secs;
        if (!_.initialized) {
            _.rate_val = instant_rate;
            _.initialized = true;
        } else {
            _.rate_val += _.alpha * (instant_rate - _.rate_val);
        }
    }

    /// Return current smoothed event rate per second.
    pub fn rate(): f64 {
        return _.rate_val;
    }
}

/// Fixed-ring-slot sliding time-window counter with single-shot initialization
/// and zero runtime heap allocations.
pub struct SlidingWindowCounter {
    alloc: std.mem.Allocator,
    buckets: []u64,
    bucket_count: usize,
    bucket_duration_ms: i64,
    window_ms: i64,
    last_tick_idx: i64 = 0,
    has_data: bool = false,

    /// Initialize a sliding window covering `window_ms` milliseconds split into `bucket_count` slots.
    pub fn init(
        alloc: std.mem.Allocator,
        window_ms: i64,
        bucket_count: usize,
    ): SlidingWindowCounter ! HistogramError {
        if (window_ms <= 0 || bucket_count == 0) {
            return HistogramError.InvalidWindow;
        }
        const bucket_dur = @divFloor(window_ms, @as(i64, @intCast(bucket_count)));
        if (bucket_dur <= 0) {
            return HistogramError.InvalidWindow;
        }
        const buckets = alloc.alloc(u64, bucket_count) catch return HistogramError.OutOfMemory;
        @memset(buckets, 0);
        return SlidingWindowCounter{
            alloc: alloc,
            buckets: buckets,
            bucket_count: bucket_count,
            bucket_duration_ms: bucket_dur,
            window_ms: bucket_dur * @as(i64, @intCast(bucket_count)),
            last_tick_idx: 0,
            has_data: false,
        };
    }

    /// Release ring buffer memory.
    pub fn deinit(*_) {
        if (_.buckets.len > 0) {
            _.alloc.free(_.buckets);
            _.buckets = [];
        }
    }

    /// Reset all ring slots to zero.
    pub fn reset(*_) {
        @memset(_.buckets, 0);
        _.last_tick_idx = 0;
        _.has_data = false;
    }

    fn advance(*_, now_ms: i64) {
        const cur_tick = @divFloor(now_ms, _.bucket_duration_ms);
        if (!_.has_data) {
            _.last_tick_idx = cur_tick;
            _.has_data = true;
            return;
        }
        if (cur_tick <= _.last_tick_idx) {
            return;
        }
        const diff = cur_tick - _.last_tick_idx;
        const n_i64: i64 = @as(i64, @intCast(_.bucket_count));
        if (diff >= n_i64) {
            @memset(_.buckets, 0);
        } else {
            let t = _.last_tick_idx + 1;
            while (t <= cur_tick) : (t += 1) {
                const slot: usize = @as(usize, @intCast(@mod(t, n_i64)));
                _.buckets[slot] = 0;
            }
        }
        _.last_tick_idx = cur_tick;
    }

    /// Record `count` events at timestamp `now_ms`.
    pub fn record(*_, now_ms: i64, count: u64) {
        _.advance(now_ms);
        const cur_tick = @divFloor(now_ms, _.bucket_duration_ms);
        const n_i64: i64 = @as(i64, @intCast(_.bucket_count));
        if (cur_tick + n_i64 > _.last_tick_idx) {
            const slot: usize = @as(usize, @intCast(@mod(cur_tick, n_i64)));
            _.buckets[slot] += count;
        }
    }

    /// Return total event count within the active window ending at `now_ms`.
    pub fn sum(*_, now_ms: i64): u64 {
        _.advance(now_ms);
        let total: u64 = 0;
        for (c of _.buckets) {
            total += c;
        }
        return total;
    }

    /// Return event rate per second over the configured window at `now_ms`.
    pub fn ratePerSec(*_, now_ms: i64): f64 {
        const total = _.sum(now_ms);
        return ((total as f64) * 1000.0) / (_.window_ms as f64);
    }
}
