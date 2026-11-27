// Source: cp/lib/sketch/sketch.cp (HyperLogLog, QuantileDigest & ReservoirSampler)
// Pure CP zero-C-dependency streaming probabilistic sketch algorithms for
// cardinality estimation, bounded-memory quantile summarization, and uniform reservoir sampling.

const std = @import("std");

/// Error set for streaming sketch operations.
pub err SketchError {
    InvalidPrecision,
    InvalidQuantile,
    InvalidCapacity,
    IncompatibleMerge,
    InvalidValue,
    OutOfMemory,
}

@inline fn splitmix64(x: u64): u64 {
    let z = x +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

@inline fn hashBytes64(data: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    for (b of data) {
        h ^= b as u64;
        h = h *% 0x100000001b3;
    }
    return splitmix64(h ^ ((data.len as u64) *% 0x9e3779b97f4a7c15));
}

/// HyperLogLog cardinality estimator using 2^precision 1-byte registers.
/// Supports precision in [4, 16] (e.g., 12 -> 4 KiB, 14 -> 16 KiB),
/// small-range LinearCounting bias correction, and lossless distributed merge.
pub struct HyperLogLog {
    alloc: std.mem.Allocator,
    precision: u8,
    m: usize,
    registers: []u8,

    /// Initialize a HyperLogLog sketch with `precision` in [4, 16].
    pub static fn init(alloc: std.mem.Allocator, precision: u8): HyperLogLog ! SketchError {
        if (precision < 4 || precision > 16) {
            return SketchError.InvalidPrecision;
        }
        const m: usize = @as(usize, 1) << u6(precision);
        const regs = alloc.alloc(u8, m) catch return SketchError.OutOfMemory;
        @memset(regs, 0);
        return HyperLogLog{
            alloc: alloc,
            precision: precision,
            m: m,
            registers: regs,
        };
    }

    /// Release backing register storage.
    pub fn deinit(*_) {
        if (_.registers.len > 0) {
            _.alloc.free(_.registers);
            _.registers = [];
        }
    }

    /// Reset all registers to zero without reallocating.
    pub fn reset(*_) {
        @memset(_.registers, 0);
    }

    /// Return configured precision parameter `p`.
    pub fn getPrecision(): u8 {
        return _.precision;
    }

    /// Return number of register buckets `2^p`.
    pub fn bucketCount(): usize {
        return _.m;
    }

    fn addHash(*_, h: u64) {
        const rem_bits: u8 = @as(u8, 64) - _.precision;
        const shift_p = u6(rem_bits);
        const idx = (h >> shift_p) as usize;
        const w: u64 = h << u6(_.precision);
        let rho: u8 = rem_bits + 1;
        if (w != 0) {
            const lz = @clz(w) as u8;
            if (lz < rem_bits) {
                rho = lz + 1;
            }
        }
        if (rho > _.registers[idx]) {
            _.registers[idx] = rho;
        }
    }

    /// Observe an arbitrary byte slice / string item.
    pub fn add(*_, item: []const u8) {
        _.addHash(hashBytes64(item));
    }

    /// Observe a 64-bit unsigned integer item.
    pub fn addU64(*_, v: u64) {
        _.addHash(splitmix64(v));
    }

    /// Estimate distinct cardinality with LinearCounting correction for small cardinalities.
    pub fn count(): u64 {
        const mf = _.m as f64;
        let alpha: f64 = 0.7213 / (1.0 + 1.079 / mf);
        if (_.m == 16) {
            alpha = 0.673;
        } else if (_.m == 32) {
            alpha = 0.697;
        } else if (_.m == 64) {
            alpha = 0.709;
        }

        let inv_sum: f64 = 0.0;
        let zeros: usize = 0;
        for (reg of _.registers) {
            if (reg == 0) {
                zeros += 1;
                inv_sum += 1.0;
            } else {
                const denom = (@as(u64, 1) << u6(reg)) as f64;
                inv_sum += 1.0 / denom;
            }
        }

        let estimate = alpha * mf * mf / inv_sum;
        if (estimate <= 2.5 * mf && zeros > 0) {
            estimate = mf * @log(mf / (zeros as f64));
        }
        if (estimate < 0.0) return 0;
        return (estimate + 0.5) as u64;
    }

    /// Merge another HyperLogLog of identical precision into `self` in-place.
    pub fn merge(*_, other: HyperLogLog): void ! SketchError {
        if (_.precision != other.precision || _.m != other.m) {
            return SketchError.IncompatibleMerge;
        }
        let i: usize = 0;
        while (i < _.m) : (i += 1) {
            if (other.registers[i] > _.registers[i]) {
                _.registers[i] = other.registers[i];
            }
        }
    }
}

/// Weighted centroid for T-Digest style quantile summarization.
pub struct Centroid {
    mean: f64,
    weight: f64,
}

fn siftDownCentroids(arr: []Centroid, start: usize, end: usize) {
    let root = start;
    while (root * 2 + 1 < end) {
        let child = root * 2 + 1;
        if (child + 1 < end && arr[child].mean < arr[child + 1].mean) {
            child += 1;
        }
        if (arr[root].mean < arr[child].mean) {
            const tmp = arr[root];
            arr[root] = arr[child];
            arr[child] = tmp;
            root = child;
        } else {
            return;
        }
    }
}

fn sortCentroids(arr: []Centroid) {
    if (arr.len < 2) return;
    let start = arr.len / 2;
    while (start > 0) {
        start -= 1;
        siftDownCentroids(arr, start, arr.len);
    }
    let end = arr.len;
    while (end > 1) {
        end -= 1;
        const tmp = arr[0];
        arr[0] = arr[end];
        arr[end] = tmp;
        siftDownCentroids(arr, 0, end);
    }
}

/// Streaming bounded-memory quantile estimator using T-Digest style centroid merging.
/// Guarantees bounded centroid count (`<= max_centroids`) and high tail accuracy (P95/P99/P99.9).
pub struct QuantileDigest {
    alloc: std.mem.Allocator,
    max_centroids: usize,
    centroids: []Centroid,
    merged_count: usize = 0,
    unmerged_count: usize = 0,
    total_weight: f64 = 0.0,
    min_val: f64 = 0.0,
    max_val: f64 = 0.0,
    has_data: bool = false,

    /// Initialize a QuantileDigest with at most `max_centroids` compressed centroids (`>= 2`).
    pub static fn init(alloc: std.mem.Allocator, max_centroids: usize): QuantileDigest ! SketchError {
        if (max_centroids < 2) {
            return SketchError.InvalidCapacity;
        }
        const buf_cap = max_centroids * 4;
        const buf = alloc.alloc(Centroid, buf_cap) catch return SketchError.OutOfMemory;
        return QuantileDigest{
            alloc: alloc,
            max_centroids: max_centroids,
            centroids: buf,
        };
    }

    /// Release backing centroid buffer.
    pub fn deinit(*_) {
        if (_.centroids.len > 0) {
            _.alloc.free(_.centroids);
            _.centroids = [];
        }
    }

    /// Reset all accumulated observations.
    pub fn reset(*_) {
        _.merged_count = 0;
        _.unmerged_count = 0;
        _.total_weight = 0.0;
        _.min_val = 0.0;
        _.max_val = 0.0;
        _.has_data = false;
    }

    /// Total number of observed values.
    pub fn count(): u64 {
        return (_.total_weight + 0.5) as u64;
    }

    /// Minimum observed value, or null if empty.
    pub fn min(): f64? {
        if (!_.has_data) return null;
        return _.min_val;
    }

    /// Maximum observed value, or null if empty.
    pub fn max(): f64? {
        if (!_.has_data) return null;
        return _.max_val;
    }

    fn compressPass(*_, total_n: usize, delta_eff: f64): usize {
        let write_idx: usize = 0;
        let cur = _.centroids[0];
        let weight_so_far: f64 = 0.0;
        const n = _.total_weight;

        let i: usize = 1;
        while (i < total_n) : (i += 1) {
            const nxt = _.centroids[i];
            const proposed = cur.weight + nxt.weight;
            const q = (weight_so_far + proposed * 0.5) / n;
            const max_w = 4.0 * n * q * (1.0 - q) / delta_eff;
            if (proposed <= max_w) {
                cur.mean = (cur.mean * cur.weight + nxt.mean * nxt.weight) / proposed;
                cur.weight = proposed;
            } else {
                _.centroids[write_idx] = cur;
                write_idx += 1;
                weight_so_far += cur.weight;
                cur = nxt;
            }
        }
        _.centroids[write_idx] = cur;
        return write_idx + 1;
    }

    fn compress(*_) {
        const total_n = _.merged_count + _.unmerged_count;
        if (total_n == 0) return;
        if (_.unmerged_count == 0 && _.merged_count <= _.max_centroids) return;

        sortCentroids(_.centroids[0..total_n]);

        let delta_eff = (_.max_centroids as f64) * 0.6;
        if (delta_eff < 1.0) delta_eff = 1.0;

        let new_len = _.compressPass(total_n, delta_eff);
        let attempts: usize = 0;
        while (new_len > _.max_centroids && attempts < 8) : (attempts += 1) {
            delta_eff *= 0.5;
            if (delta_eff < 0.25) delta_eff = 0.25;
            new_len = _.compressPass(new_len, delta_eff);
        }

        // Hard fallback if tail singletons still exceed max_centroids
        while (new_len > _.max_centroids) {
            let w_idx: usize = 0;
            let i: usize = 0;
            while (i < new_len) {
                if (i + 1 < new_len && (new_len - (i / 2)) > _.max_centroids) {
                    const c0 = _.centroids[i];
                    const c1 = _.centroids[i + 1];
                    const pw = c0.weight + c1.weight;
                    _.centroids[w_idx] = Centroid{
                        mean: (c0.mean * c0.weight + c1.mean * c1.weight) / pw,
                        weight: pw,
                    };
                    w_idx += 1;
                    i += 2;
                } else {
                    _.centroids[w_idx] = _.centroids[i];
                    w_idx += 1;
                    i += 1;
                }
            }
            new_len = w_idx;
        }

        _.merged_count = new_len;
        _.unmerged_count = 0;
    }

    fn addCentroid(*_, c: Centroid) {
        if (!_.has_data) {
            _.min_val = c.mean;
            _.max_val = c.mean;
            _.has_data = true;
        } else {
            if (c.mean < _.min_val) _.min_val = c.mean;
            if (c.mean > _.max_val) _.max_val = c.mean;
        }
        _.total_weight += c.weight;
        _.centroids[_.merged_count + _.unmerged_count] = c;
        _.unmerged_count += 1;

        if (_.merged_count + _.unmerged_count >= _.centroids.len) {
            _.compress();
        }
    }

    /// Observe a finite floating-point value. Rejects NaN and +/-Inf with `InvalidValue`.
    pub fn add(*_, val: f64): void ! SketchError {
        if (std.math.isNan(val) || std.math.isInf(val)) {
            return SketchError.InvalidValue;
        }
        _.addCentroid(Centroid{ mean: val, weight: 1.0 });
    }

    /// Estimate the quantile `q` in `[0.0, 1.0]`. Returns null if no observations exist.
    pub fn quantile(*_, q: f64): f64? ! SketchError {
        if (std.math.isNan(q) || q < 0.0 || q > 1.0) {
            return SketchError.InvalidQuantile;
        }
        if (!_.has_data) return null;

        _.compress();

        if (q == 0.0) return _.min_val;
        if (q == 1.0) return _.max_val;
        if (_.merged_count == 1) return _.centroids[0].mean;

        const n = _.total_weight;
        const index = q * n;

        const first = _.centroids[0];
        if (index < first.weight * 0.5) {
            const frac = index / (first.weight * 0.5);
            return _.min_val + frac * (first.mean - _.min_val);
        }

        const last = _.centroids[_.merged_count - 1];
        const tail_start = n - last.weight * 0.5;
        if (index > tail_start) {
            const frac = (index - tail_start) / (last.weight * 0.5);
            return last.mean + frac * (_.max_val - last.mean);
        }

        let weight_so_far = first.weight * 0.5;
        let i: usize = 0;
        while (i + 1 < _.merged_count) : (i += 1) {
            const c0 = _.centroids[i];
            const c1 = _.centroids[i + 1];
            const dw = (c0.weight + c1.weight) * 0.5;
            if (weight_so_far + dw >= index) {
                const frac = if (dw > 0.0) (index - weight_so_far) / dw else 0.5;
                return c0.mean + frac * (c1.mean - c0.mean);
            }
            weight_so_far += dw;
        }

        return last.mean;
    }

    /// Merge another QuantileDigest into `self` in-place.
    pub fn merge(*_, other: QuantileDigest) {
        if (!other.has_data) return;
        const other_total = other.merged_count + other.unmerged_count;
        let i: usize = 0;
        while (i < other_total) : (i += 1) {
            _.addCentroid(other.centroids[i]);
        }
        if (other.min_val < _.min_val) _.min_val = other.min_val;
        if (other.max_val > _.max_val) _.max_val = other.max_val;
        _.compress();
    }
}

/// Algorithm R uniform reservoir sampler over an unknown-length stream.
/// Guarantees every observed item has exact probability `capacity / seenCount` of being retained.
pub fn ReservoirSampler(comptime T: type): type {
    return struct {
        const Self = @This();

        alloc: std.mem.Allocator,
        capacity: usize,
        items: []T,
        filled: usize = 0,
        seen: u64 = 0,
        rng_state: u64,

        /// Initialize a ReservoirSampler with fixed capacity `capacity >= 1` and PRNG `seed`.
        pub static fn init(alloc: std.mem.Allocator, capacity: usize, seed: u64): Self ! SketchError {
            if (capacity == 0) {
                return SketchError.InvalidCapacity;
            }
            const buf = alloc.alloc(T, capacity) catch return SketchError.OutOfMemory;
            return Self{
                alloc: alloc,
                capacity: capacity,
                items: buf,
                rng_state: seed,
            };
        }

        /// Release backing reservoir slice.
        pub fn deinit(*_) {
            if (_.items.len > 0) {
                _.alloc.free(_.items);
                _.items = [];
            }
        }

        fn nextU64(*_): u64 {
            _.rng_state +%= 0x9e3779b97f4a7c15;
            return splitmix64(_.rng_state);
        }

        fn nextBounded(*_, bound: u64): u64 {
            const r = _.nextU64();
            const prod = (r as u128) * (bound as u128);
            return (prod >> 64) as u64;
        }

        /// Observe one item from the stream.
        pub fn observe(*_, item: T) {
            _.seen += 1;
            if (_.filled < _.capacity) {
                _.items[_.filled] = item;
                _.filled += 1;
                return;
            }
            const j = _.nextBounded(_.seen);
            if (j < (_.capacity as u64)) {
                _.items[j as usize] = item;
            }
        }

        /// Return currently retained sample slice (length `min(seenCount, capacity)`).
        pub fn samples(): []const T {
            return _.items[0.._.filled];
        }

        /// Return total number of items observed so far.
        pub fn seenCount(): u64 {
            return _.seen;
        }

        /// Reset sampler state and re-seed the PRNG without reallocating.
        pub fn reset(*_, seed: u64) {
            _.filled = 0;
            _.seen = 0;
            _.rng_state = seed;
        }
    };
}
