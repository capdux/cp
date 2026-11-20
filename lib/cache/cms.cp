// Source: cp/lib/cache/cms.cp (Count-Min Sketch Frequency Estimator)
// Sub-linear space streaming frequency estimator with zero underestimation guarantee
// and algebraic merge support for distributed AI Agent telemetry & hot-key tracking.

const std = @import("std");
import { CacheErr } from "./lru.cp";

const EULER_E: f64 = 2.718281828459045;

zig {
    const CMS_BASE_SEED: u64 = 0x517CC1B727220A95;
    const CMS_FNV_OFFSET: u64 = 14695981039346656037;
    const CMS_FNV_PRIME: u64 = 1099511628211;
    const CMS_GOLDEN_64: u64 = 0x9E3779B97F4A7C15;
    const CMS_MIX_MUL1: u64 = 0xBF58476D1CE4E5B9;
    const CMS_MIX_MUL2: u64 = 0x94D049BB133111EB;
}

fn splitMix64Cms(x_in: u64): u64 {
    let res: u64 = 0;
    zig {
        var z: u64 = x_in +% CMS_GOLDEN_64;
        z = (z ^ (z >> 30)) *% CMS_MIX_MUL1;
        z = (z ^ (z >> 27)) *% CMS_MIX_MUL2;
        res = z ^ (z >> 31);
    }
    return res;
}

fn hashRowItem(item: []const u8, row: usize): u64 {
    let res: u64 = 0;
    zig {
        const r64 = @as(u64, row);
        var h: u64 = CMS_FNV_OFFSET ^ splitMix64Cms(CMS_BASE_SEED +% (r64 *% CMS_GOLDEN_64));
        for (item) |b| {
            h ^= @as(u64, b);
            h *%= CMS_FNV_PRIME;
        }
        res = splitMix64Cms(h ^ (r64 << 32));
    }
    return res;
}

/// Count-Min Sketch probabilistic frequency table (`width * depth` 64-bit counters).
/// Guarantees `estimate(x) >= true_frequency(x)` for all non-negative increments.
pub struct CountMinSketch {
    allocator: std.mem.Allocator,
    counters: []u64,
    cols: usize = 0,
    rows: usize = 0,
    total_events: u64 = 0,

    /// Initializes a Count-Min Sketch with explicit `num_width > 0` columns and `num_depth > 0` rows.
    pub static fn init(alloc: std.mem.Allocator, num_width: usize, num_depth: usize): CountMinSketch! {
        if (num_width == 0 || num_depth == 0) return CacheErr.InvalidParameter;

        const total_cells = std.math.mul(usize, num_width, num_depth) catch return CacheErr.InvalidParameter;
        let counters = alloc.alloc(u64, total_cells) catch return CacheErr.OutOfMemory;
        for (i of 0..total_cells) {
            counters[i] = 0;
        }

        return CountMinSketch{
            allocator: alloc,
            counters: counters,
            cols: num_width,
            rows: num_depth,
            total_events: 0,
        };
    }

    /// Initializes a Count-Min Sketch from additive error tolerance `epsilon` and failure probability `delta`.
    /// Uses `width = ceil(e / epsilon)` and `depth = ceil(ln(1 / delta))`.
    pub static fn initWithError(
        alloc: std.mem.Allocator,
        epsilon: f64,
        delta: f64,
    ): CountMinSketch! {
        if (epsilon <= 0.0 || epsilon >= 1.0) return CacheErr.InvalidParameter;
        if (delta <= 0.0 || delta >= 1.0) return CacheErr.InvalidParameter;

        let w_f = @ceil(EULER_E / epsilon);
        if (w_f < 1.0) w_f = 1.0;
        let d_f = @ceil(@log(1.0 / delta));
        if (d_f < 1.0) d_f = 1.0;
        const w_usize: usize = @intFromFloat(w_f);
        const d_usize: usize = @intFromFloat(d_f);

        return CountMinSketch.init(alloc, w_usize, d_usize);
    }

    /// Releases the underlying counter matrix.
    pub fn deinit(*_) {
        if (_.counters.len > 0) {
            _.allocator.free(_.counters);
        }
        _.counters = [];
        _.cols = 0;
        _.rows = 0;
        _.total_events = 0;
    }

    /// Returns the number of buckets per hash row (`width`).
    pub fn width(): usize {
        return _.cols;
    }

    /// Returns the number of independent hash rows (`depth`).
    pub fn depth(): usize {
        return _.rows;
    }

    /// Returns the cumulative sum of all `increment` counts added to the sketch.
    pub fn totalCount(): u64 {
        return _.total_events;
    }

    /// Increments the frequency estimate of `item` by `count` across all `depth` rows.
    pub fn increment(*_, item: []const u8, count: u64) {
        if (_.cols == 0 || _.rows == 0) return;
        const w_u64 = _.cols as u64;
        let r: usize = 0;
        while (r < _.rows) : (r += 1) {
            const col = (hashRowItem(item, r) % w_u64) as usize;
            const idx = (r * _.cols) + col;
            _.counters[idx] = _.counters[idx] +| count;
        }
        _.total_events = _.total_events +| count;
    }

    /// Returns the minimum counter value across all hash rows for `item`.
    /// Never underestimates the true count (`estimate >= true_count`).
    pub fn estimate(item: []const u8): u64 {
        if (_.cols == 0 || _.rows == 0) return 0;
        const w_u64 = _.cols as u64;
        let min_val: u64 = std.math.maxInt(u64);
        let r: usize = 0;
        while (r < _.rows) : (r += 1) {
            const col = (hashRowItem(item, r) % w_u64) as usize;
            const idx = (r * _.cols) + col;
            const v = _.counters[idx];
            if (v < min_val) {
                min_val = v;
            }
        }
        return min_val;
    }

    /// Merges `other` into `this` by elementwise saturating addition.
    /// Requires matching `width()` and `depth()`.
    pub fn merge(*_, other: CountMinSketch): ! {
        if (_.cols != other.cols || _.rows != other.rows) {
            return CacheErr.SizeMismatch;
        }
        for (i of 0.._.counters.len) {
            _.counters[i] = _.counters[i] +| other.counters[i];
        }
        _.total_events = _.total_events +| other.total_events;
    }

    /// Resets all counters and `totalCount()` to zero while retaining allocated capacity.
    pub fn clear(*_) {
        for (i of 0.._.counters.len) {
            _.counters[i] = 0;
        }
        _.total_events = 0;
    }
}
