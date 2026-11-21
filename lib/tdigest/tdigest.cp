// Source: cp/lib/tdigest/tdigest.cp (Pure CP Streaming T-Digest Quantile Estimator)
const std = @import("std");

pub const TDigestError = error{
    EmptyDigest,
    InvalidCompression,
    InvalidQuantile,
};

pub const Centroid = struct {
    mean: f64,
    weight: f64,
};

fn centroidLess(_ctx: void, a: Centroid, b: Centroid): bool {
    * = _ctx;
    return a.mean < b.mean;
}

pub const TDigest = struct {
    compression: f64,
    centroids: std.ArrayList(Centroid),
    unmerged: std.ArrayList(Centroid),
    total_weight: f64,
    min_val: f64,
    max_val: f64,
    max_unmerged: usize,
    alloc: std.mem.Allocator,

    pub fn init(alloc: std.mem.Allocator, compression: f64): TDigestError!TDigest {
        if (compression < 10.0) { return TDigestError.InvalidCompression; }
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

    pub fn deinit(self: *TDigest) {
        self.centroids.deinit(self.alloc);
        self.unmerged.deinit(self.alloc);
    }

    pub fn count(self: TDigest): f64 {
        return self.total_weight;
    }

    pub fn add(self: *TDigest, value: f64, weight: f64): !void {
        if (weight <= 0.0 or std.math.isNan(value)) { return; }
        if (value < self.min_val) { self.min_val = value; }
        if (value > self.max_val) { self.max_val = value; }
        try self.unmerged.append(self.alloc, Centroid{ .mean = value, .weight = weight });
        self.total_weight += weight;
        if (self.unmerged.items.len >= self.max_unmerged) {
            try self.compress();
        }
    }

    pub fn observe(self: *TDigest, value: f64): !void {
        try self.add(value, 1.0);
    }

    pub fn compress(self: *TDigest): !void {
        if (self.unmerged.items.len == 0 and self.centroids.items.len <= 1) { return; }
        for (u of self.unmerged.items) {
            try self.centroids.append(self.alloc, u);
        }
        self.unmerged.clearRetainingCapacity();
        if (self.centroids.items.len <= 1) { return; }

        std.mem.sort(Centroid, self.centroids.items, {}, centroidLess);

        let merged = std.ArrayList(Centroid).empty;
        errdefer merged.deinit(self.alloc);

        let cur = self.centroids.items[0];
        let weight_so_far: f64 = 0.0;
        const n = self.total_weight;

        let i: usize = 1;
        while (i < self.centroids.items.len) : (i += 1) {
            const nxt = self.centroids.items[i];
            const q = (weight_so_far + cur.weight + nxt.weight * 0.5) / n;
            const k_limit = (4.0 * n * q * (1.0 - q)) / self.compression;
            if (cur.weight + nxt.weight <= @max(1.0, k_limit)) {
                const new_w = cur.weight + nxt.weight;
                cur.mean = (cur.mean * cur.weight + nxt.mean * nxt.weight) / new_w;
                cur.weight = new_w;
            } else {
                try merged.append(self.alloc, cur);
                weight_so_far += cur.weight;
                cur = nxt;
            }
        }
        try merged.append(self.alloc, cur);

        self.centroids.deinit(self.alloc);
        self.centroids = merged;
    }

    pub fn merge(self: *TDigest, other: *TDigest): !void {
        try other.compress();
        for (c of other.centroids.items) {
            try self.add(c.mean, c.weight);
        }
        try self.compress();
    }

    pub fn quantile(self: *TDigest, q: f64): !f64 {
        if (q < 0.0 or q > 1.0) { return TDigestError.InvalidQuantile; }
        if (self.total_weight <= 0.0) { return TDigestError.EmptyDigest; }
        try self.compress();

        const cs = self.centroids.items;
        if (cs.len == 1) { return cs[0].mean; }
        if (q == 0.0) { return self.min_val; }
        if (q == 1.0) { return self.max_val; }

        const index = q * self.total_weight;
        if (index < cs[0].weight * 0.5) {
            return self.min_val + (cs[0].mean - self.min_val) * (index / (cs[0].weight * 0.5));
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
        const rem = self.total_weight - index;
        if (rem < last.weight * 0.5) {
            return self.max_val - (self.max_val - last.mean) * (rem / (last.weight * 0.5));
        }
        return last.mean;
    }
};
