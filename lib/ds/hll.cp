// Reference-hardened against pwoolcoc/hyperloglog (MIT) & Dunning t-digest (Apache-2.0)
// Source: cp/lib/ds/hll.cp (HyperLogLog Cardinality Estimation, 16-Lane SIMD Merge & Serialization)
// Pure CP zero-C-dependency HyperLogLog probabilistic cardinality estimator with
// SplitMix64 mixing, LinearCounting small-range correction, 64-bit large-range correction,
// 16-lane @Vector(16, u8) SIMD register merge & zero-count, and versioned binary serialization ("HLL1").

const std = @import("std");

type HllVec16u8 = @Vector(16, u8);

pub const HLL_MIN_PRECISION: u8 = 4;
pub const HLL_MAX_PRECISION: u8 = 16;
pub const HLL_SERIAL_HEADER_LEN: usize = 5;

/// Error set for HyperLogLog cardinality estimation and serialization operations.
pub err HllError {
    InvalidPrecision,
    PrecisionMismatch,
    InvalidSerializedData,
    OutOfMemory,
}

fn hllLoadVec16(data: []const u8, off: usize): HllVec16u8 {
    const v: HllVec16u8 = [
        data[off], data[off + 1], data[off + 2], data[off + 3],
        data[off + 4], data[off + 5], data[off + 6], data[off + 7],
        data[off + 8], data[off + 9], data[off + 10], data[off + 11],
        data[off + 12], data[off + 13], data[off + 14], data[off + 15],
    ];
    return v;
}

/// 16-lane `@Vector(16, u8)` SIMD element-wise register max merge (`dst[i] = max(dst[i], src[i])`).
pub fn simdHllMergeRegisters(dst: []u8, src: []const u8): usize {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const va = hllLoadVec16(dst, i);
        const vb = hllLoadVec16(src, i);
        const vm = @max(va, vb);
        dst[i] = vm[0];
        dst[i + 1] = vm[1];
        dst[i + 2] = vm[2];
        dst[i + 3] = vm[3];
        dst[i + 4] = vm[4];
        dst[i + 5] = vm[5];
        dst[i + 6] = vm[6];
        dst[i + 7] = vm[7];
        dst[i + 8] = vm[8];
        dst[i + 9] = vm[9];
        dst[i + 10] = vm[10];
        dst[i + 11] = vm[11];
        dst[i + 12] = vm[12];
        dst[i + 13] = vm[13];
        dst[i + 14] = vm[14];
        dst[i + 15] = vm[15];
        i += 16;
    }
    while (i < n) : (i += 1) {
        if (src[i] > dst[i]) {
            dst[i] = src[i];
        }
    }
    return n;
}

/// Scalar reference for `simdHllMergeRegisters` (differential verification).
pub fn scalarHllMergeRegisters(dst: []u8, src: []const u8): usize {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (src[i] > dst[i]) {
            dst[i] = src[i];
        }
    }
    return n;
}

/// 16-lane `@Vector(16, u8)` SIMD counter for zero registers (used in empty check & LinearCounting).
pub fn simdHllCountZeros(regs: []const u8): usize {
    const zero_v: HllVec16u8 = @splat(0);
    let zeros: usize = 0;
    let i: usize = 0;
    while (i + 16 <= regs.len) {
        const v = hllLoadVec16(regs, i);
        if (@reduce(.Or, v == zero_v)) {
            let j: usize = 0;
            while (j < 16) : (j += 1) {
                if (regs[i + j] == 0) {
                    zeros += 1;
                }
            }
        }
        i += 16;
    }
    while (i < regs.len) : (i += 1) {
        if (regs[i] == 0) {
            zeros += 1;
        }
    }
    return zeros;
}

/// Scalar reference for `simdHllCountZeros` (differential verification).
pub fn scalarHllCountZeros(regs: []const u8): usize {
    let zeros: usize = 0;
    let i: usize = 0;
    while (i < regs.len) : (i += 1) {
        if (regs[i] == 0) {
            zeros += 1;
        }
    }
    return zeros;
}

/// Zero-allocation strict validator for serialized `"HLL1"` binary payloads.
pub fn validateHllSerialized(bytes: []const u8): HllError!void {
    if (bytes.len < HLL_SERIAL_HEADER_LEN) {
        return HllError.InvalidSerializedData;
    }
    if (bytes[0] != 'H' || bytes[1] != 'L' || bytes[2] != 'L' || bytes[3] != '1') {
        return HllError.InvalidSerializedData;
    }
    const p_val = bytes[4];
    if (p_val < HLL_MIN_PRECISION || p_val > HLL_MAX_PRECISION) {
        return HllError.InvalidSerializedData;
    }
    const m: usize = @as(usize, 1) << @as(u6, @intCast(p_val));
    if (bytes.len != HLL_SERIAL_HEADER_LEN + m) {
        return HllError.InvalidSerializedData;
    }
    const max_rho: u8 = (@as(u8, 64) - p_val) + 1;
    let i: usize = HLL_SERIAL_HEADER_LEN;
    while (i < bytes.len) : (i += 1) {
        if (bytes[i] > max_rho) {
            return HllError.InvalidSerializedData;
        }
    }
}

@inline fn splitmix64(x: u64): u64 {
    const c1: u64 = (@as(u64, 0x9e3779b9) << 32) | @as(u64, 0x7f4a7c15);
    const c2: u64 = (@as(u64, 0xbf58476d) << 32) | @as(u64, 0x1ce4e5b9);
    const c3: u64 = (@as(u64, 0x94d049bb) << 32) | @as(u64, 0x133111eb);
    let z = x +% c1;
    z = (z ^ (z >> 30)) *% c2;
    z = (z ^ (z >> 27)) *% c3;
    return z ^ (z >> 31);
}

@inline fn hashBytes64(data: []const u8): u64 {
    const offset_basis: u64 = (@as(u64, 0xcbf29ce4) << 32) | @as(u64, 0x84222325);
    const prime: u64 = (@as(u64, 0x00000100) << 32) | @as(u64, 0x000001b3);
    const mix: u64 = (@as(u64, 0x9e3779b9) << 32) | @as(u64, 0x7f4a7c15);
    let h: u64 = offset_basis;
    for (b of data) {
        h ^= b as u64;
        h = h *% prime;
    }
    return h ^ ((data.len as u64) *% mix);
}

/// HyperLogLog cardinality estimator using `m = 2^p` 1-byte registers (`p` in `4..16`).
/// Provides ~`1.04 / sqrt(m)` standard error, LinearCounting small-range bias correction,
/// 16-lane SIMD lossless element-wise max merge, and compact binary serialization.
pub struct HyperLogLog {
    alloc: std.mem.Allocator,
    p: u8,
    m: usize,
    regs: []u8,

    /// Initialize a HyperLogLog sketch with precision `p` in `[4, 16]` (`m = 1 << p` registers).
    pub static fn init(allocator: std.mem.Allocator, precision_val: u8): HyperLogLog ! HllError {
        if (precision_val < HLL_MIN_PRECISION || precision_val > HLL_MAX_PRECISION) {
            return HllError.InvalidPrecision;
        }
        const m: usize = @as(usize, 1) << @as(u6, @intCast(precision_val));
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
        const idx = (h >> @as(u6, @intCast(rem_bits))) as usize;
        const w: u64 = h << @as(u6, @intCast(_.p));
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

    /// Estimate distinct cardinality using harmonic mean with 16-lane SIMD zero counting,
    /// LinearCounting small-range correction, and 64-bit large-range correction.
    pub fn count(): u64 {
        const zeros = simdHllCountZeros(_.regs);
        if (zeros == _.m) {
            return 0;
        }

        const mf = _.m as f64;
        let alpha: f64 = 0.7213 / (1.0 + 1.079 / mf);
        if (_.m == 16) {
            alpha = 0.673;
        } else if (_.m == 32) {
            alpha = 0.697;
        } else if (_.m == 64) {
            alpha = 0.709;
        }

        let inv_sum: f64 = zeros as f64;
        for (reg of _.regs) {
            if (reg > 0) {
                const denom = (@as(u64, 1) << @as(u6, @intCast(reg))) as f64;
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

    /// Merge another HyperLogLog of identical precision into `self` in-place via 16-lane SIMD max.
    pub fn merge(*_, other: HyperLogLog): void ! HllError {
        if (_.p != other.p || _.m != other.m) {
            return HllError.PrecisionMismatch;
        }
        * = simdHllMergeRegisters(_.regs, other.regs);
    }

    /// Serialize sketch into compact binary format: `"HLL1"` (4B) + `precision` (1B) + `regs` (`m` B).
    pub fn serialize(allocator: std.mem.Allocator): []u8 ! HllError {
        let out = allocator.alloc(u8, HLL_SERIAL_HEADER_LEN + _.m) catch return HllError.OutOfMemory;
        out[0] = 'H';
        out[1] = 'L';
        out[2] = 'L';
        out[3] = '1';
        out[4] = _.p;
        @memcpy(out[HLL_SERIAL_HEADER_LEN..], _.regs);
        return out;
    }

    /// Deserialize sketch from `"HLL1"` binary payload with strict header, length & register-bound validation.
    pub static fn deserialize(allocator: std.mem.Allocator, bytes: []const u8): HyperLogLog ! HllError {
        try validateHllSerialized(bytes);
        const p_val = bytes[4];
        const m: usize = @as(usize, 1) << @as(u6, @intCast(p_val));
        const regs = allocator.alloc(u8, m) catch return HllError.OutOfMemory;
        @memcpy(regs, bytes[HLL_SERIAL_HEADER_LEN..]);
        return HyperLogLog{
            alloc: allocator,
            p: p_val,
            m: m,
            regs: regs,
        };
    }
}
