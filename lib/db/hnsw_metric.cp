// Source: cp/lib/hnsw/metric.cp (Pure CP HNSW distance metrics, errors, and configuration)
const std = @import("std");

pub const HnswError = error{
    DimensionMismatch,
    EmptyVector,
    InvalidConfig,
    DuplicateId,
    IdNotFound,
    CorruptSnapshot,
};

pub enum DistanceMetric {
    L2Squared,
    Cosine,
    InnerProduct,
}

pub struct HnswConfig {
    m: usize = 16,
    m0: usize = 32,
    ef_construction: usize = 100,
    seed: u64 = 0x5EEDCAFE12345678,

    pub fn validate(): HnswError!void {
        if (_.m < 2 or _.ef_construction < 1) {
            return HnswError.InvalidConfig;
        }
        if (_.m0 < _.m) {
            return HnswError.InvalidConfig;
        }
    }
}

pub struct SearchHit {
    id: u64,
    distance: f32,
}

pub fn computeDistance(metric: DistanceMetric, a: []const f32, b: []const f32): HnswError!f32 {
    if (a.len == 0 or b.len == 0) {
        return HnswError.EmptyVector;
    }
    if (a.len != b.len) {
        return HnswError.DimensionMismatch;
    }

    switch (metric) {
        .L2Squared => {
            let sum: f64 = 0.0;
            let i: usize = 0;
            while (i < a.len) {
                const diff: f64 = @as(f64, a[i]) - @as(f64, b[i]);
                sum += diff * diff;
                i += 1;
            }
            return @floatCast(sum);
        },
        .Cosine => {
            let dot: f64 = 0.0;
            let norm_a: f64 = 0.0;
            let norm_b: f64 = 0.0;
            let i: usize = 0;
            while (i < a.len) {
                const va: f64 = @as(f64, a[i]);
                const vb: f64 = @as(f64, b[i]);
                dot += va * vb;
                norm_a += va * va;
                norm_b += vb * vb;
                i += 1;
            }
            const denom: f64 = std.math.sqrt(norm_a) * std.math.sqrt(norm_b);
            if (denom == 0.0) {
                return 1.0;
            }
            let cos_sim: f64 = dot / denom;
            if (cos_sim > 1.0) {
                cos_sim = 1.0;
            }
            if (cos_sim < -1.0) {
                cos_sim = -1.0;
            }
            return @floatCast(1.0 - cos_sim);
        },
        .InnerProduct => {
            let dot: f64 = 0.0;
            let i: usize = 0;
            while (i < a.len) {
                dot += @as(f64, a[i]) * @as(f64, b[i]);
                i += 1;
            }
            return @floatCast(-dot);
        },
    }
}
