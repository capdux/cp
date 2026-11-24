// Reference-hardened against pwoolcoc/hyperloglog (MIT) & Dunning t-digest (Apache-2.0)
// Source: cp/lib/ds/tdigest.cp (Pure CP Streaming T-Digest Quantile Estimator + 4-Lane f64 SIMD Kernels)

const std = @import("std");

type TdVec4f64 = @Vector(4, f64);

pub const TDIGEST_MIN_COMPRESSION: f64 = 10.0;
pub const TDIGEST_MAX_COMPRESSION: f64 = 10000.0;

pub err TDigestError {
    EmptyDigest,
    InvalidCompression,
    InvalidQuantile,
    InvalidValue,
    InvalidWeight,
    OutOfMemory,
}

pub struct Centroid {
    mean: f64,
    weight: f64,
}

pub struct TdMinMax {
    min_val: f64 = 0.0,
    max_val: f64 = 0.0,
    valid: bool = false,
}

// ============================================================================
// 4-Lane @Vector(4, f64) SIMD Kernels + Scalar Differential References
// ============================================================================

fn tdLoadVec4(data: []const f64, off: usize): TdVec4f64 {
    const v: TdVec4f64 = [data[off], data[off + 1], data[off + 2], data[off + 3]];
    return v;
}

/// 4-lane `@Vector(4, f64)` SIMD weight sum reduction.
pub fn simdTdSumWeightsF64(weights: []const f64): f64 {
    let acc: TdVec4f64 = @splat(0.0);
    let i: usize = 0;
    while (i + 4 <= weights.len) {
        acc += tdLoadVec4(weights, i);
        i += 4;
    }
    let total: f64 = @reduce(.Add, acc);
    while (i < weights.len) : (i += 1) {
        total += weights[i];
    }
    return total;
}

/// Scalar reference for `simdTdSumWeightsF64` (differential verification).
pub fn scalarTdSumWeightsF64(weights: []const f64): f64 {
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < weights.len) : (i += 1) {
        total += weights[i];
    }
    return total;
}

/// 4-lane `@Vector(4, f64)` SIMD min/max scanner over finite `f64` slices.
pub fn simdTdMinMaxF64(values: []const f64): TdMinMax {
    if (values.len == 0) {
        return TdMinMax{ min_val: 0.0, max_val: 0.0, valid: false };
    }
    let min_v: TdVec4f64 = @splat(values[0]);
    let max_v: TdVec4f64 = @splat(values[0]);
    let i: usize = 0;
    while (i + 4 <= values.len) {
        const v = tdLoadVec4(values, i);
        min_v = @min(min_v, v);
        max_v = @max(max_v, v);
        i += 4;
    }
    let lo: f64 = @reduce(.Min, min_v);
    let hi: f64 = @reduce(.Max, max_v);
    while (i < values.len) : (i += 1) {
        if (values[i] < lo) lo = values[i];
        if (values[i] > hi) hi = values[i];
    }
    return TdMinMax{ min_val: lo, max_val: hi, valid: true };
}

/// Scalar reference for `simdTdMinMaxF64` (differential verification).
pub fn scalarTdMinMaxF64(values: []const f64): TdMinMax {
    if (values.len == 0) {
        return TdMinMax{ min_val: 0.0, max_val: 0.0, valid: false };
    }
    let lo: f64 = values[0];
    let hi: f64 = values[0];
    let i: usize = 1;
    while (i < values.len) : (i += 1) {
        if (values[i] < lo) lo = values[i];
        if (values[i] > hi) hi = values[i];
    }
    return TdMinMax{ min_val: lo, max_val: hi, valid: true };
}

fn isFiniteF64(v: f64): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

fn centroidLess(_ctx: usize, a: Centroid, b: Centroid): bool {
    if (_ctx == 0) {}
    return a.mean < b.mean;
}

pub struct TDigest {
    compression: f64,
    centroids: std.ArrayList(Centroid),
    unmerged: std.ArrayList(Centroid),
    total_weight: f64,
    min_val: f64,
    max_val: f64,
    max_unmerged: usize,
    alloc: std.mem.Allocator,

    pub static fn init(alloc: std.mem.Allocator, compression: f64): TDigest ! TDigestError {
        if (!isFiniteF64(compression) || compression < TDIGEST_MIN_COMPRESSION || compression > TDIGEST_MAX_COMPRESSION) {
            return TDigestError.InvalidCompression;
        }
        const max_u: usize = @intFromFloat(@round(compression * 5.0));
        return TDigest{
            .compression = compression,
            .centroids = std.ArrayList(Centroid).empty,
            .unmerged = std.ArrayList(Centroid).empty,
            .total_weight = 0.0,
            .min_val = std.math.inf(f64),
            .max_val = -std.math.inf(f64),
            .max_unmerged = if (max_u < 64) 64 else max_u,
            .alloc = alloc,
        };
    }

    pub fn deinit(*_) {
        _.centroids.deinit(_.alloc);
        _.unmerged.deinit(_.alloc);
    }

    pub fn count(): f64 {
        return _.total_weight;
    }

    pub fn centroidCount(): usize {
        return _.centroids.items.len;
    }

    pub fn minValue(): f64? {
        if (_.total_weight <= 0.0) return null;
        return _.min_val;
    }

    pub fn maxValue(): f64? {
        if (_.total_weight <= 0.0) return null;
        return _.max_val;
    }

    /// Adds a weighted finite observation. Strictly rejects `NaN`, `±Inf`, and non-positive weights.
    pub fn add(*_, value: f64, weight: f64): void ! TDigestError {
        if (!isFiniteF64(value)) {
            return TDigestError.InvalidValue;
        }
        if (!isFiniteF64(weight) || weight <= 0.0) {
            return TDigestError.InvalidWeight;
        }
        if (value < _.min_val) { _.min_val = value; }
        if (value > _.max_val) { _.max_val = value; }
        _.unmerged.append(_.alloc, Centroid{ .mean = value, .weight = weight }) catch return TDigestError.OutOfMemory;
        _.total_weight += weight;
        if (_.unmerged.items.len >= _.max_unmerged) {
            try _.compress();
        }
    }

    /// Observes a single finite value with weight `1.0`.
    pub fn observe(*_, value: f64): void ! TDigestError {
        try _.add(value, 1.0);
    }

    /// Observes a batch of finite values (weight `1.0` each), pre-scanning min/max with 4-lane SIMD.
    pub fn addBatch(*_, values: []const f64): void ! TDigestError {
        if (values.len == 0) return;
        for (v of values) {
            if (!isFiniteF64(v)) return TDigestError.InvalidValue;
        }
        const mm = simdTdMinMaxF64(values);
        if (mm.valid) {
            if (mm.min_val < _.min_val) _.min_val = mm.min_val;
            if (mm.max_val > _.max_val) _.max_val = mm.max_val;
        }
        for (v of values) {
            _.unmerged.append(_.alloc, Centroid{ .mean = v, .weight = 1.0 }) catch return TDigestError.OutOfMemory;
            _.total_weight += 1.0;
            if (_.unmerged.items.len >= _.max_unmerged) {
                try _.compress();
            }
        }
    }

    pub fn compress(*_): void ! TDigestError {
        if (_.unmerged.items.len == 0 && _.centroids.items.len <= 1) { return; }
        for (u of _.unmerged.items) {
            _.centroids.append(_.alloc, u) catch return TDigestError.OutOfMemory;
        }
        _.unmerged.clearRetainingCapacity();
        if (_.centroids.items.len <= 1) { return; }

        const sort_ctx: usize = 0;
        std.mem.sort(Centroid, _.centroids.items, sort_ctx, centroidLess);

        let merged = std.ArrayList(Centroid).empty;
        errdefer merged.deinit(_.alloc);

        let cur = _.centroids.items[0];
        let weight_so_far: f64 = 0.0;
        const n = _.total_weight;

        let i: usize = 1;
        while (i < _.centroids.items.len) : (i += 1) {
            const nxt = _.centroids.items[i];
            const q = (weight_so_far + cur.weight + nxt.weight * 0.5) / n;
            const k_limit = (4.0 * n * q * (1.0 - q)) / _.compression;
            if (cur.weight + nxt.weight <= @max(1.0, k_limit)) {
                const new_w = cur.weight + nxt.weight;
                cur.mean = (cur.mean * cur.weight + nxt.mean * nxt.weight) / new_w;
                cur.weight = new_w;
            } else {
                merged.append(_.alloc, cur) catch return TDigestError.OutOfMemory;
                weight_so_far += cur.weight;
                cur = nxt;
            }
        }
        merged.append(_.alloc, cur) catch return TDigestError.OutOfMemory;

        _.centroids.deinit(_.alloc);
        _.centroids = merged;
    }

    pub fn merge(*_, other: *TDigest): void ! TDigestError {
        if (other.total_weight <= 0.0) return;
        try other.compress();
        if (other.min_val < _.min_val) _.min_val = other.min_val;
        if (other.max_val > _.max_val) _.max_val = other.max_val;
        for (c of other.centroids.items) {
            try _.add(c.mean, c.weight);
        }
        try _.compress();
    }

    pub fn quantile(*_, q: f64): f64 ! TDigestError {
        if (!isFiniteF64(q) || q < 0.0 || q > 1.0) { return TDigestError.InvalidQuantile; }
        if (_.total_weight <= 0.0) { return TDigestError.EmptyDigest; }
        try _.compress();

        const cs = _.centroids.items;
        if (cs.len == 0) { return TDigestError.EmptyDigest; }
        if (cs.len == 1) { return cs[0].mean; }
        if (q == 0.0) { return _.min_val; }
        if (q == 1.0) { return _.max_val; }

        const index = q * _.total_weight;
        if (index < cs[0].weight * 0.5) {
            return _.min_val + (cs[0].mean - _.min_val) * (index / (cs[0].weight * 0.5));
        }

        let weight_so_far: f64 = cs[0].weight * 0.5;
        let i: usize = 0;
        while (i + 1 < cs.len) : (i += 1) {
            const dw = (cs[i].weight + cs[i + 1].weight) * 0.5;
            if (weight_so_far + dw > index) {
                const frac = (index - weight_so_far) / dw;
                return cs[i].mean + (cs[i + 1].mean - cs[i].mean) * frac;
            }
            weight_so_far += dw;
        }

        const last = cs[cs.len - 1];
        const rem = _.total_weight - index;
        if (rem < last.weight * 0.5) {
            return _.max_val - (_.max_val - last.mean) * (rem / (last.weight * 0.5));
        }
        return last.mean;
    }
}
