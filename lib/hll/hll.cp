// Source: cp/lib/hll/hll.cp (HyperLogLog Cardinality Estimation & Mergeable Sketch)
// Pure CP zero-C-dependency HyperLogLog probabilistic cardinality estimator with
// SplitMix64 mixing, LinearCounting small-range correction, 64-bit large-range correction,
// lossless distributed merge, and versioned binary serialization ("HLL1").

const std = @import("std");

/// Error set for HyperLogLog cardinality estimation and serialization operations.
pub err HllError {
    InvalidPrecision,
    PrecisionMismatch,
    InvalidSerializedData,
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
    return h ^ ((data.len as u64) *% 0x9e3779b97f4a7c15);
}

/// HyperLogLog cardinality estimator using `m = 2^p` 1-byte registers (`p` in `4..16`).
/// Provides ~`1.04 / sqrt(m)` standard error, LinearCounting small-range bias correction,
/// lossless element-wise max merge, and compact binary serialization.
pub struct HyperLogLog {
    alloc: std.mem.Allocator,
    p: u8,
    m: usize,
    regs: []u8,

    /// Initialize a HyperLogLog sketch with precision `p` in `[4, 16]` (`m = 1 << p` registers).
    pub fn init(allocator: std.mem.Allocator, precision_val: u8): HyperLogLog ! HllError {
        if (precision_val < 4 || precision_val > 16) {
            return HllError.InvalidPrecision;
        }
        const m: usize = @as(usize, 1) << u6(precision_val);
        const regs = allocator.alloc(u8, m) catch return HllError.OutOfMemory;
        @memset(regs, 0);
        return HyperLogLog{
            alloc: allocator,
            p: precision_val,
            m: m,
            regs: regs,
        };
    }

    /// Release backing register memory.
    pub fn deinit(*_) {
        if (_.regs.len > 0) {
            _.alloc.free(_.regs);
            _.regs = [];
        }
    }

    /// Reset all registers to zero without reallocating.
    pub fn clear(*_) {
        @memset(_.regs, 0);
    }

    /// Return configured precision parameter `p` (`4..16`).
    pub fn precision(): u8 {
        return _.p;
    }

    /// Return number of register buckets `m = 2^p`.
    pub fn registerCount(): usize {
        return _.m;
    }

    /// Return theoretical standard relative error bound `1.04 / sqrt(m)`.
    pub fn relativeErrorBound(): f64 {
        return 1.04 / @sqrt(_.m as f64);
    }

    /// Observe a 64-bit hash/integer value, applying SplitMix64 finalizer mixing.
    /// Top `p` bits select the register bucket; remaining `64 - p` bits yield leading-zero rank + 1.
    pub fn addHash(*_, hash64: u64) {
        const h = splitmix64(hash64);
        const rem_bits: u8 = @as(u8, 64) - _.p;
        const idx = (h >> u6(rem_bits)) as usize;
        const w: u64 = h << u6(_.p);
        let rho: u8 = rem_bits + 1;
        if (w != 0) {
            const lz = @clz(w) as u8;
            if (lz < rem_bits) {
                rho = lz + 1;
            }
        }
        if (rho > _.regs[idx]) {
            _.regs[idx] = rho;
        }
    }

    /// Observe an arbitrary byte slice / string item.
    pub fn add(*_, item: []const u8) {
        _.addHash(hashBytes64(item));
    }

    /// Estimate distinct cardinality using harmonic mean with LinearCounting small-range
    /// correction and 64-bit large-range correction.
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
        for (reg of _.regs) {
            if (reg == 0) {
                zeros += 1;
                inv_sum += 1.0;
            } else {
                const denom = (@as(u64, 1) << u6(reg)) as f64;
                inv_sum += 1.0 / denom;
            }
        }

        let estimate = alpha * mf * mf / inv_sum;
        if (estimate <= 2.5 * mf) {
            if (zeros > 0) {
                estimate = mf * @log(mf / (zeros as f64));
            }
        } else {
            const two64: f64 = 18446744073709551616.0;
            if (estimate > two64 / 30.0) {
                const ratio = estimate / two64;
                if (ratio < 1.0) {
                    estimate = -two64 * @log(1.0 - ratio);
                }
            }
        }
        if (estimate < 0.0) return 0;
        return (estimate + 0.5) as u64;
    }

    /// Merge another HyperLogLog of identical precision into `self` in-place via element-wise max.
    pub fn merge(*_, other: HyperLogLog): void ! HllError {
        if (_.p != other.p || _.m != other.m) {
            return HllError.PrecisionMismatch;
        }
        let i: usize = 0;
        while (i < _.m) : (i += 1) {
            if (other.regs[i] > _.regs[i]) {
                _.regs[i] = other.regs[i];
            }
        }
    }

    /// Serialize sketch into compact binary format: `"HLL1"` (4B) + `precision` (1B) + `regs` (`m` B).
    pub fn serialize(allocator: std.mem.Allocator): []u8 ! HllError {
        let out = allocator.alloc(u8, 5 + _.m) catch return HllError.OutOfMemory;
        out[0] = 'H';
        out[1] = 'L';
        out[2] = 'L';
        out[3] = '1';
        out[4] = _.p;
        @memcpy(out[5..], _.regs);
        return out;
    }

    /// Deserialize sketch from `"HLL1"` binary payload with header & length validation.
    pub fn deserialize(allocator: std.mem.Allocator, bytes: []const u8): HyperLogLog ! HllError {
        if (bytes.len < 5) {
            return HllError.InvalidSerializedData;
        }
        if (bytes[0] != 'H' || bytes[1] != 'L' || bytes[2] != 'L' || bytes[3] != '1') {
            return HllError.InvalidSerializedData;
        }
        const p_val = bytes[4];
        if (p_val < 4 || p_val > 16) {
            return HllError.InvalidSerializedData;
        }
        const m: usize = @as(usize, 1) << u6(p_val);
        if (bytes.len != 5 + m) {
            return HllError.InvalidSerializedData;
        }
        const regs = allocator.alloc(u8, m) catch return HllError.OutOfMemory;
        @memcpy(regs, bytes[5..]);
        return HyperLogLog{
            alloc: allocator,
            p: p_val,
            m: m,
            regs: regs,
        };
    }
}
