// Source: cp/lib/metrics/histogram.cp (Cumulative Bucket Histogram & Quantile Estimator)
// Pure CP Prometheus cumulative bucket histogram with default HTTP latency buckets and quantile interpolation.

const std = @import("std");
import { MetricsError } from "./primitives.cp";

/// Default Prometheus HTTP request latency buckets in seconds.
pub const DEFAULT_HTTP_BUCKETS: [11]f64 = [
    0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0,
];

/// Internal storage for a cumulative bucket histogram.
pub struct HistogramState {
    alloc: std.mem.Allocator = undefined,
    bound_values: []f64 = [],
    cumulative_counts: []u64 = [],
    total_sum: f64 = 0.0,
    total_count: u64 = 0,

    /// Initializes a `HistogramState` with validated, strictly increasing finite upper bounds.
    /// When `bucket_bounds` is empty, `DEFAULT_HTTP_BUCKETS` is used automatically.
    pub static fn init(alloc: std.mem.Allocator, bucket_bounds: []const f64): HistogramState ! MetricsError {
        const src_bounds: []const f64 = if (bucket_bounds.len == 0) DEFAULT_HTTP_BUCKETS[0..] else bucket_bounds;
        return HistogramState.initExact(alloc, src_bounds);
    }

    /// Initializes a `HistogramState` with exact user-provided bounds (may be empty if no finite buckets).
    pub static fn initExact(alloc: std.mem.Allocator, bucket_bounds: []const f64): HistogramState ! MetricsError {
        let i: usize = 0;
        while (i < bucket_bounds.len) : (i += 1) {
            const b = bucket_bounds[i];
            if (std.math.isNan(b) || std.math.isInf(b)) {
                return MetricsError.InvalidBounds;
            }
            if (i > 0 && b <= bucket_bounds[i - 1]) {
                return MetricsError.InvalidBounds;
            }
        }

        const owned_bounds = alloc.alloc(f64, bucket_bounds.len) catch return MetricsError.OutOfMemory;
        errdefer alloc.free(owned_bounds);

        if (bucket_bounds.len > 0) {
            @memcpy(owned_bounds, bucket_bounds);
        }

        const counts = alloc.alloc(u64, bucket_bounds.len + 1) catch return MetricsError.OutOfMemory;
        @memset(counts, 0);

        return HistogramState{
            alloc: alloc,
            bound_values: owned_bounds,
            cumulative_counts: counts,
            total_sum: 0.0,
            total_count: 0,
        };
    }

    /// Releases allocated bucket arrays.
    pub fn deinit(*_) {
        if (_.bound_values.len > 0) {
            _.alloc.free(_.bound_values);
        }
        if (_.cumulative_counts.len > 0) {
            _.alloc.free(_.cumulative_counts);
        }
        _.bound_values = [];
        _.cumulative_counts = [];
        _.total_sum = 0.0;
        _.total_count = 0;
    }

    /// Records a single observation into cumulative buckets, sum, and total count.
    /// `NaN` observations are ignored to avoid poisoning `_sum`.
    pub fn observe(*_, value: f64) {
        if (std.math.isNan(value)) return;

        _.total_sum += value;
        _.total_count += 1;

        const n = _.bound_values.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            if (value <= _.bound_values[i]) {
                _.cumulative_counts[i] += 1;
            }
        }
        // Always increment the +Inf bucket (final slot)
        _.cumulative_counts[n] += 1;
    }

    /// Resets all bucket counts, sum, and total count to zero while preserving bucket bounds.
    pub fn reset(*_) {
        if (_.cumulative_counts.len > 0) {
            @memset(_.cumulative_counts, 0);
        }
        _.total_sum = 0.0;
        _.total_count = 0;
    }

    /// Returns cumulative bucket counts slice of length `bounds.len + 1` (last entry is `+Inf`).
    pub fn bucketCounts(): []const u64 {
        return _.cumulative_counts;
    }

    /// Returns configured finite upper bounds slice.
    pub fn bounds(): []const f64 {
        return _.bound_values;
    }

    /// Returns sum of all observed values.
    pub fn sum(): f64 {
        return _.total_sum;
    }

    /// Returns total count of observed values.
    pub fn count(): u64 {
        return _.total_count;
    }

    /// Estimates the `q`-th quantile (`0.0 <= q <= 1.0`, e.g. `0.5`, `0.95`, `0.99`)
    /// using Prometheus-style linear interpolation over cumulative histogram buckets.
    /// Returns `0.0` when `count() == 0` or no finite bounds exist; clamps `q` outside `[0, 1]`.
    pub fn estimateQuantile(q: f64): f64 {
        if (_.total_count == 0 || _.bound_values.len == 0) return 0.0;
        if (std.math.isNan(q)) return 0.0;

        const n = _.bound_values.len;
        if (q <= 0.0) {
            let idx: usize = 0;
            while (idx < n) : (idx += 1) {
                if (_.cumulative_counts[idx] > 0) {
                    if (idx == 0) {
                        return if (_.bound_values[0] > 0.0) 0.0 else _.bound_values[0];
                    }
                    return _.bound_values[idx - 1];
                }
            }
            return _.bound_values[n - 1];
        }
        if (q >= 1.0) {
            // If observations exceed the largest finite bound, Prometheus returns the largest finite bound.
            let idx: usize = 0;
            while (idx < n) : (idx += 1) {
                if (_.cumulative_counts[idx] >= _.total_count) {
                    return _.bound_values[idx];
                }
            }
            return _.bound_values[n - 1];
        }

        const rank = q * (_.total_count as f64);
        let b_idx: usize = 0;
        while (b_idx < n) : (b_idx += 1) {
            const cur_c = _.cumulative_counts[b_idx] as f64;
            if (cur_c >= rank) {
                const prev_c: f64 = if (b_idx == 0) 0.0 else (_.cumulative_counts[b_idx - 1] as f64);
                const bucket_end = _.bound_values[b_idx];
                const bucket_start: f64 = if (b_idx == 0) (if (bucket_end > 0.0) 0.0 else bucket_end) else _.bound_values[b_idx - 1];
                const span = cur_c - prev_c;
                if (span <= 0.0) return bucket_end;
                const frac = (rank - prev_c) / span;
                return bucket_start + (bucket_end - bucket_start) * frac;
            }
        }

        // Quantile falls in the (+Inf) bucket: return highest finite upper bound
        return _.bound_values[n - 1];
    }

    /// Strict quantile estimation returning `MetricsError.InvalidQuantile` when `q` is NaN or outside `[0.0, 1.0]`.
    pub fn estimateQuantileChecked(q: f64): f64 ! MetricsError {
        if (std.math.isNan(q) || q < 0.0 || q > 1.0) {
            return MetricsError.InvalidQuantile;
        }
        return _.estimateQuantile(q);
    }
}

/// Cumulative bucket histogram supporting default HTTP latency buckets, custom ordered bounds,
/// and Prometheus linear-interpolation quantile estimation.
/// Works both standalone and as a handle bound to a `MetricsRegistry` entry.
pub struct MetricsHistogram {
    local_state: HistogramState = HistogramState{},
    shared_ptr: (*HistogramState!)? = null,

    /// Initializes a standalone histogram. When `bucket_bounds` is empty, uses `DEFAULT_HTTP_BUCKETS`.
    pub static fn init(alloc: std.mem.Allocator, bucket_bounds: []const f64): MetricsHistogram ! MetricsError {
        const st = HistogramState.init(alloc, bucket_bounds)?;
        return MetricsHistogram{
            local_state: st,
            shared_ptr: null,
        };
    }

    /// Initializes a standalone histogram with `DEFAULT_HTTP_BUCKETS`.
    pub static fn initDefault(alloc: std.mem.Allocator): MetricsHistogram ! MetricsError {
        return MetricsHistogram.init(alloc, DEFAULT_HTTP_BUCKETS[0..]);
    }

    /// Creates a registry-backed histogram view pointing to a stable `HistogramState`.
    pub static fn fromStatePtr(ptr: *HistogramState!): MetricsHistogram {
        return MetricsHistogram{
            local_state: HistogramState{},
            shared_ptr: ptr,
        };
    }

    /// Frees memory owned by a standalone histogram (no-op for registry-backed views).
    pub fn deinit(*_) {
        if (_.shared_ptr == null) {
            _.local_state.deinit();
        }
    }

    /// Records a sample observation into the histogram.
    pub fn observe(*_, value: f64) {
        if (ptr := _.shared_ptr) {
            unsafe {
                ptr.observe(value);
            }
        } else {
            _.local_state.observe(value);
        }
    }

    /// Resets all bucket counts, sum, and total count to zero.
    pub fn reset(*_) {
        if (ptr := _.shared_ptr) {
            unsafe {
                ptr.reset();
            }
        } else {
            _.local_state.reset();
        }
    }

    /// Returns cumulative bucket counts slice of length `bounds.len + 1` (final slot is `+Inf`).
    pub fn bucketCounts(): []const u64 {
        if (ptr := _.shared_ptr) {
            unsafe {
                return ptr.bucketCounts();
            }
        }
        return _.local_state.bucketCounts();
    }

    /// Returns configured finite upper bounds slice.
    pub fn bounds(): []const f64 {
        if (ptr := _.shared_ptr) {
            unsafe {
                return ptr.bounds();
            }
        }
        return _.local_state.bounds();
    }

    /// Returns total sum of observed samples.
    pub fn sum(): f64 {
        if (ptr := _.shared_ptr) {
            unsafe {
                return ptr.sum();
            }
        }
        return _.local_state.sum();
    }

    /// Returns total count of observed samples.
    pub fn count(): u64 {
        if (ptr := _.shared_ptr) {
            unsafe {
                return ptr.count();
            }
        }
        return _.local_state.count();
    }

    /// Estimates the `q`-th quantile (`0.0 <= q <= 1.0`, e.g. `0.5`, `0.95`, `0.99`).
    pub fn estimateQuantile(q: f64): f64 {
        if (ptr := _.shared_ptr) {
            unsafe {
                return ptr.estimateQuantile(q);
            }
        }
        return _.local_state.estimateQuantile(q);
    }

    /// Strict quantile estimation returning `MetricsError.InvalidQuantile` when `q` is invalid.
    pub fn estimateQuantileChecked(q: f64): f64 ! MetricsError {
        if (ptr := _.shared_ptr) {
            unsafe {
                return ptr.estimateQuantileChecked(q);
            }
        }
        return _.local_state.estimateQuantileChecked(q);
    }
}
