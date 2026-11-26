// @cap/hyperloglog — Zero-allocation 256-byte HyperLogLog cardinality estimator for CP
// Clean-room MIT implementation inspired by Rust `hyperloglog` and Redis PFADD/PFCOUNT/PFMERGE.
//
// Features:
// - `HyperLogLog256`: precision p = 8, m = 256 one-byte registers (256 bytes total stack footprint).
// - High-quality 64-bit Wyhash mixing (overflow-free, uniform avalanche).
// - Standard HyperLogLog harmonic mean with small-range LinearCounting bias correction.
// - Lossless distributed `merge` (per-register max) for multi-node UV aggregation.
// - Zero heap allocation.

const std = @import("std");

const HLL_M: usize = 256;

fn hashBytes64(data: string) -> u64 {
    const h: u64 = std.hash.Wyhash.hash(0, data);
    return h;
}

fn hashU64(val: u64) -> u64 {
    let buf: [8]u8 = undefined;
    buf[0] = (val & 0xFF) as u8;
    buf[1] = ((val >> 8) & 0xFF) as u8;
    buf[2] = ((val >> 16) & 0xFF) as u8;
    buf[3] = ((val >> 24) & 0xFF) as u8;
    buf[4] = ((val >> 32) & 0xFF) as u8;
    buf[5] = ((val >> 40) & 0xFF) as u8;
    buf[6] = ((val >> 48) & 0xFF) as u8;
    buf[7] = ((val >> 56) & 0xFF) as u8;
    const h: u64 = std.hash.Wyhash.hash(0, buf[0..8]);
    return h;
}

/// Counts leading zeros in the lower 56 bits of `w` (`0..=56`).
fn clz56(w: u64) -> u8 {
    if (w == 0) {
        return 56;
    }
    let zeros: u8 = 0;
    let mask: u64 = (1 as u64) << 55;
    while (mask != 0 && (w & mask) == 0) {
        zeros += 1;
        mask = mask >> 1;
    }
    return zeros;
}

/// 256-register (256-byte) HyperLogLog probabilistic cardinality estimator (p = 8, m = 256).
pub struct HyperLogLog256 {
    pub regs: [HLL_M]u8,

    pub static fn init() -> HyperLogLog256 {
        let arr: [HLL_M]u8 = undefined;
        let i: usize = 0;
        while (i < HLL_M) {
            arr[i] = 0;
            i += 1;
        }
        return HyperLogLog256{ regs: arr };
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < HLL_M) {
            _.regs[i] = 0;
            i += 1;
        }
    }

    pub fn registerCount() -> usize {
        return HLL_M;
    }

    fn insertHash(*_, h: u64) {
        const idx: usize = (h >> 56) as usize;
        const w: u64 = h & (0x00FFFFFFFFFFFFFF as u64);
        const rho: u8 = clz56(w) + 1;
        if (rho > _.regs[idx]) {
            _.regs[idx] = rho;
        }
    }

    /// Inserts a byte string element (`PFADD`).
    pub fn add(*_, item: string) {
        _.insertHash(hashBytes64(item));
    }

    /// Inserts a 64-bit unsigned integer element.
    pub fn addU64(*_, val: u64) {
        _.insertHash(hashU64(val));
    }

    /// Estimates distinct cardinality (`PFCOUNT`) using harmonic mean + LinearCounting small-range correction.
    pub fn count() -> u64 {
        let sum: f64 = 0.0;
        let zeros: usize = 0;
        let i: usize = 0;
        while (i < HLL_M) {
            const r = _.regs[i];
            if (r == 0) {
                zeros += 1;
                sum += 1.0;
            } else {
                const denom: u64 = (1 as u64) << (r as u6);
                sum += 1.0 / (denom as f64);
            }
            i += 1;
        }

        if (zeros == HLL_M) {
            return 0;
        }

        const m_f: f64 = 256.0;
        const alpha_m: f64 = 0.7213 / (1.0 + (1.079 / m_f));
        const raw: f64 = (alpha_m * m_f * m_f) / sum;

        // Small-range LinearCounting bias correction
        if (raw <= 2.5 * m_f && zeros > 0) {
            const ratio: f64 = m_f / (zeros as f64);
            const ln_val: f64 = std.math.log(f64, std.math.e, ratio);
            const est: f64 = m_f * ln_val;
            return (est + 0.5) as u64;
        }

        return (raw + 0.5) as u64;
    }

    /// Merges another `HyperLogLog256` into this instance (`PFMERGE`) by taking per-register max.
    pub fn merge(*_, other: HyperLogLog256) {
        let i: usize = 0;
        while (i < HLL_M) {
            if (other.regs[i] > _.regs[i]) {
                _.regs[i] = other.regs[i];
            }
            i += 1;
        }
    }

    /// Returns the theoretical relative standard error `1.04 / sqrt(256) = 0.065` (6.5%).
    pub fn relativeError() -> f64 {
        return 1.04 / 16.0;
    }
}
