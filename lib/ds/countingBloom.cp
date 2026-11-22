// Origin: Pure-CP Stdlib (Zero-Alloc 4-Bit Saturating Counting Bloom Filter with Deletion) | Created: 2026-11-23
// Pure CP zero-heap-allocation 4-bit saturating Counting Bloom Filter:
//   - Compact nibble-packed inline storage (2048 bytes holding up to 4096 4-bit counters in 0..15)
//   - Kirsch-Mitzenmacher double hashing (FNV-1a + SplitMix64) with configurable power-of-two `m`
//     (16..=4096) and hash count `k` (1..=8)
//   - Deletion with saturation protection (counters at 15 stay saturated on remove to guarantee zero
//     false negatives) and membership pre-check (never decrements buckets for absent keys)
//   - Saturating union (`mergeAdd`), cardinality estimation (`estimatedItems`), theoretical FPR
//     (`estimatedFpr`) & operational telemetry snapshot

const std = @import("std");

// ============================================================================
// 1. Constants & Data Models
// ============================================================================

/// Maximum number of 4-bit counters stored inline (zero dynamic heap).
pub const CBLOOM_MAX_BUCKETS: usize = 4096;

/// Minimum allowed number of buckets (power of two).
pub const CBLOOM_MIN_BUCKETS: usize = 16;

/// Inline nibble-packed backing storage size (`4096 / 2 = 2048` bytes).
pub const CBLOOM_STORAGE_BYTES: usize = 2048;

/// Maximum number of hash functions `k`.
pub const CBLOOM_MAX_HASHES: u8 = 8;

/// Maximum 4-bit saturating counter value (`0xF`).
pub const CBLOOM_MAX_COUNTER: u8 = 15;

/// Operational & probabilistic telemetry snapshot for `CountingBloomFilter`.
pub struct CountingBloomStats {
    num_buckets: usize = 0;
    num_hashes: u8 = 0;
    non_zero_buckets: usize = 0;
    saturated_buckets: usize = 0;
    total_inserts: u64 = 0;
    total_removes: u64 = 0;
    rejected_removes: u64 = 0;
    saturation_hits: u64 = 0;
    estimated_items: f64 = 0.0;
    estimated_fpr: f64 = 0.0;
}

// ============================================================================
// 2. Internal Hash & Power-of-Two Helpers
// ============================================================================

fn isPowerOfTwo(n: usize): bool {
    return n > 0 && (n & (n - 1)) == 0;
}

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h = h ^ (data[i] as u64);
        h = h *% 0x100000001b3;
    }
    return h;
}

fn splitMix64(x_in: u64): u64 {
    let z: u64 = x_in +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

fn u64ToLeBytes(val: u64): [8]u8 {
    let out: [8]u8 = [0; 8];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        out[i] = ((val >> ((i * 8) as u6)) & 0xFF) as u8;
    }
    return out;
}

// ============================================================================
// 3. Core Engine (`CountingBloomFilter`)
// ============================================================================

/// Zero-heap 4-bit saturating Counting Bloom Filter supporting safe deletion and saturating merge.
pub struct CountingBloomFilter {
    nibbles: [CBLOOM_STORAGE_BYTES]u8 = [0; CBLOOM_STORAGE_BYTES];
    num_buckets: usize = CBLOOM_MAX_BUCKETS;
    bucket_mask: usize = CBLOOM_MAX_BUCKETS - 1;
    num_hashes: u8 = 4;
    total_inserts: u64 = 0;
    total_removes: u64 = 0;
    rejected_removes: u64 = 0;
    saturation_hits: u64 = 0;

    /// Initializes a Counting Bloom Filter with `bucket_count` (power of 2 in `16..=4096`)
    /// and `hash_count` (`1..=8`). Returns `null` on invalid parameters.
    pub fn init(bucket_count: usize, hash_count: u8): CountingBloomFilter? {
        if (bucket_count < CBLOOM_MIN_BUCKETS || bucket_count > CBLOOM_MAX_BUCKETS || !isPowerOfTwo(bucket_count)) {
            return null;
        }
        if (hash_count == 0 || hash_count > CBLOOM_MAX_HASHES) {
            return null;
        }
        return CountingBloomFilter{
            num_buckets: bucket_count,
            bucket_mask: bucket_count - 1,
            num_hashes: hash_count,
        };
    }

    /// Initializes a default 4096-bucket, 4-hash Counting Bloom Filter.
    pub fn initDefault(): CountingBloomFilter {
        return CountingBloomFilter{};
    }

    fn getBucket(idx: usize): u8 {
        const byte_idx = idx >> 1;
        const b = nibbles[byte_idx];
        if ((idx & 1) == 0) {
            return b & 0x0F;
        }
        return (b >> 4) & 0x0F;
    }

    fn setBucket(*_, idx: usize, val: u8) {
        const clamped: u8 = if (val > CBLOOM_MAX_COUNTER) CBLOOM_MAX_COUNTER else val;
        const byte_idx = idx >> 1;
        const cur = nibbles[byte_idx];
        if ((idx & 1) == 0) {
            nibbles[byte_idx] = (cur & 0xF0) | clamped;
        } else {
            nibbles[byte_idx] = (cur & 0x0F) | (clamped << 4);
        }
    }

    fn bucketIndex(h1: u64, h2: u64, step: u8): usize {
        const combined: u64 = h1 +% ((step as u64) *% h2);
        return (combined as usize) & bucket_mask;
    }

    /// Inserts `key` by saturating-incrementing its `k` 4-bit buckets (clamped at 15).
    /// Returns `true` if at least one bucket was previously 0 (definitely new), or `false` if all
    /// buckets were already non-zero.
    pub fn insert(*_, key: []const u8): bool {
        const h1 = fnv1a64(key);
        const h2 = splitMix64(h1) | 1;
        let saw_zero: bool = false;

        let i: u8 = 0;
        while (i < num_hashes) : (i += 1) {
            const idx = bucketIndex(h1, h2, i);
            const cur = getBucket(idx);
            if (cur == 0) {
                saw_zero = true;
            }
            if (cur < CBLOOM_MAX_COUNTER) {
                setBucket(idx, cur + 1);
            } else {
                saturation_hits += 1;
            }
        }
        total_inserts += 1;
        return saw_zero;
    }

    /// Convenience wrapper inserting a 64-bit integer key.
    pub fn insertU64(*_, key: u64): bool {
        const bytes = u64ToLeBytes(key);
        return insert(bytes[0..]);
    }

    /// Returns `true` if `key` may be in the set (all `k` buckets are non-zero).
    pub fn contains(key: []const u8): bool {
        const h1 = fnv1a64(key);
        const h2 = splitMix64(h1) | 1;

        let i: u8 = 0;
        while (i < num_hashes) : (i += 1) {
            const idx = bucketIndex(h1, h2, i);
            if (getBucket(idx) == 0) return false;
        }
        return true;
    }

    /// Convenience wrapper testing membership of a 64-bit integer key.
    pub fn containsU64(key: u64): bool {
        const bytes = u64ToLeBytes(key);
        return contains(bytes[0..]);
    }

    /// Returns the minimum counter value (`0..=15`) across all `k` buckets for `key`.
    pub fn minCount(key: []const u8): u8 {
        const h1 = fnv1a64(key);
        const h2 = splitMix64(h1) | 1;
        let min_v: u8 = CBLOOM_MAX_COUNTER;

        let i: u8 = 0;
        while (i < num_hashes) : (i += 1) {
            const idx = bucketIndex(h1, h2, i);
            const cur = getBucket(idx);
            if (cur < min_v) min_v = cur;
        }
        return min_v;
    }

    /// Safely removes `key`:
    ///   - If `key` is absent (`!contains(key)`), returns `false` without decrementing any bucket.
    ///   - Otherwise decrements every non-saturated bucket (`1..=14`), while leaving saturated
    ///     buckets (`15`) untouched to guarantee zero false negatives.
    pub fn remove(*_, key: []const u8): bool {
        if (!contains(key)) {
            rejected_removes += 1;
            return false;
        }

        const h1 = fnv1a64(key);
        const h2 = splitMix64(h1) | 1;

        let i: u8 = 0;
        while (i < num_hashes) : (i += 1) {
            const idx = bucketIndex(h1, h2, i);
            const cur = getBucket(idx);
            // Protect saturated counters (15) from decrement to prevent false negatives.
            if (cur > 0 && cur < CBLOOM_MAX_COUNTER) {
                setBucket(idx, cur - 1);
            }
        }
        total_removes += 1;
        return true;
    }

    /// Convenience wrapper removing a 64-bit integer key.
    pub fn removeU64(*_, key: u64): bool {
        const bytes = u64ToLeBytes(key);
        return remove(bytes[0..]);
    }

    /// Performs a saturating element-wise union (`bucket = min(15, a + b)`) with `other`.
    /// Returns `false` if `num_buckets` or `num_hashes` do not match.
    pub fn mergeAdd(*_, other: CountingBloomFilter): bool {
        if (num_buckets != other.num_buckets || num_hashes != other.num_hashes) {
            return false;
        }
        let idx: usize = 0;
        while (idx < num_buckets) : (idx += 1) {
            const sum: u16 = (getBucket(idx) as u16) + (other.getBucket(idx) as u16);
            if (sum >= (CBLOOM_MAX_COUNTER as u16)) {
                setBucket(idx, CBLOOM_MAX_COUNTER);
                if (sum > (CBLOOM_MAX_COUNTER as u16)) {
                    saturation_hits += 1;
                }
            } else {
                setBucket(idx, sum as u8);
            }
        }
        return true;
    }

    /// Resets all 4-bit counters and telemetry statistics to zero.
    pub fn clear(*_) {
        const byte_len = num_buckets >> 1;
        let i: usize = 0;
        while (i < byte_len) : (i += 1) {
            nibbles[i] = 0;
        }
        total_inserts = 0;
        total_removes = 0;
        rejected_removes = 0;
        saturation_hits = 0;
    }

    /// Counts how many 4-bit buckets are currently non-zero.
    pub fn nonZeroBuckets(): usize {
        let count: usize = 0;
        let idx: usize = 0;
        while (idx < num_buckets) : (idx += 1) {
            if (getBucket(idx) > 0) count += 1;
        }
        return count;
    }

    /// Counts how many 4-bit buckets are currently saturated at `15`.
    pub fn saturatedBuckets(): usize {
        let count: usize = 0;
        let idx: usize = 0;
        while (idx < num_buckets) : (idx += 1) {
            if (getBucket(idx) == CBLOOM_MAX_COUNTER) count += 1;
        }
        return count;
    }

    /// Estimates the number of distinct elements using Swamidass & Baldi's formula:
    /// `-(m / k) * ln(1 - X / m)`.
    pub fn estimatedItems(): f64 {
        const x = nonZeroBuckets();
        if (x == 0) return 0.0;
        const m_f: f64 = num_buckets as f64;
        const k_f: f64 = num_hashes as f64;
        let fill: f64 = (x as f64) / m_f;
        if (fill >= 0.999999) {
            fill = 0.999999;
        }
        return -(m_f / k_f) * @log(1.0 - fill);
    }

    /// Estimates the current false positive probability `(X / m)^k`.
    pub fn estimatedFpr(): f64 {
        const x = nonZeroBuckets();
        if (x == 0) return 0.0;
        const fill: f64 = (x as f64) / (num_buckets as f64);
        let fpr: f64 = 1.0;
        let i: u8 = 0;
        while (i < num_hashes) : (i += 1) {
            fpr = fpr * fill;
        }
        return fpr;
    }

    /// Returns a point-in-time operational & probabilistic telemetry snapshot.
    pub fn stats(): CountingBloomStats {
        const nz = nonZeroBuckets();
        const sat = saturatedBuckets();
        const m_f: f64 = num_buckets as f64;
        const k_f: f64 = num_hashes as f64;

        let est_items: f64 = 0.0;
        let est_fpr: f64 = 0.0;
        if (nz > 0) {
            const fill: f64 = (nz as f64) / m_f;
            const safe_fill: f64 = if (fill >= 0.999999) 0.999999 else fill;
            est_items = -(m_f / k_f) * @log(1.0 - safe_fill);
            est_fpr = 1.0;
            let i: u8 = 0;
            while (i < num_hashes) : (i += 1) {
                est_fpr = est_fpr * fill;
            }
        }

        return CountingBloomStats{
            num_buckets: num_buckets,
            num_hashes: num_hashes,
            non_zero_buckets: nz,
            saturated_buckets: sat,
            total_inserts: total_inserts,
            total_removes: total_removes,
            rejected_removes: rejected_removes,
            saturation_hits: saturation_hits,
            estimated_items: est_items,
            estimated_fpr: est_fpr,
        };
    }
}
