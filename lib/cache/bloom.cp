// Source: cp/lib/cache/bloom.cp (Space-Efficient Bloom Filter with Double Hashing)
// Space-efficient probabilistic membership filter using FNV-1a 64 + SplitMix64 double hashing
// for fast AI Agent deduplication and negative-lookup acceleration.

const std = @import("std");
import { CacheErr } from "./lru.cp";

const LN2: f64 = 0.6931471805599453;
const LN2_SQ: f64 = 0.4804530139182014;

zig {
    const FNV1A_64_OFFSET: u64 = 14695981039346656037;
    const FNV1A_64_PRIME: u64 = 1099511628211;
    const GOLDEN_64: u64 = 0x9E3779B97F4A7C15;
    const MIX_MUL1: u64 = 0xBF58476D1CE4E5B9;
    const MIX_MUL2: u64 = 0x94D049BB133111EB;
    const MIX_SEED: u64 = 0xD6E8FEB86659FD93;
}

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = 0;
    zig {
        h = FNV1A_64_OFFSET;
        for (data) |b| {
            h ^= @as(u64, b);
            h *%= FNV1A_64_PRIME;
        }
    }
    return h;
}

fn splitMix64(x_in: u64): u64 {
    let res: u64 = 0;
    zig {
        var z: u64 = x_in +% GOLDEN_64;
        z = (z ^ (z >> 30)) *% MIX_MUL1;
        z = (z ^ (z >> 27)) *% MIX_MUL2;
        res = z ^ (z >> 31);
    }
    return res;
}

fn secondaryHash64(data: []const u8, h1: u64): u64 {
    let res: u64 = 0;
    zig {
        var acc: u64 = h1 ^ (@as(u64, data.len) *% GOLDEN_64) ^ MIX_SEED;
        for (data) |b| {
            acc = (acc ^ @as(u64, b)) *% MIX_MUL1;
            acc = (acc << 13) | (acc >> 51);
        }
        res = splitMix64(acc) | 1;
    }
    return res;
}

/// Space-efficient Bloom filter backed by a contiguous 64-bit word array.
pub struct BloomFilter {
    allocator: std.mem.Allocator,
    words: []u64,
    bit_len: usize = 0,
    hash_count: usize = 0,
    inserted: u64 = 0,

    /// Initializes a Bloom filter sized optimally for `expected_items` and target `false_positive_rate`.
    /// Uses classic optimal sizing:
    /// `m = ceil(-n * ln(p) / (ln(2)^2))` and `k = round((m / n) * ln(2))`.
    pub static fn initOptimal(
        alloc: std.mem.Allocator,
        expected_items: usize,
        false_positive_rate: f64,
    ): BloomFilter! {
        if (expected_items == 0) return CacheErr.InvalidParameter;
        if (false_positive_rate <= 0.0 || false_positive_rate >= 1.0) {
            return CacheErr.InvalidParameter;
        }

        const n_f = expected_items as f64;
        let p = false_positive_rate;
        if (p < 0.000000000001) p = 0.000000000001;

        const m_raw = @ceil((-n_f * @log(p)) / LN2_SQ);
        let m_usize: usize = @intFromFloat(m_raw);
        if (m_usize < 64) m_usize = 64;

        let k_raw = @round(((m_usize as f64) / n_f) * LN2);
        if (k_raw < 1.0) k_raw = 1.0;
        if (k_raw > 32.0) k_raw = 32.0;
        const k_usize: usize = @intFromFloat(k_raw);

        return BloomFilter.initRaw(alloc, m_usize, k_usize);
    }

    /// Initializes a Bloom filter with explicit `bit_count > 0` and `num_hashes > 0`.
    pub static fn initRaw(
        alloc: std.mem.Allocator,
        bit_count: usize,
        num_hashes: usize,
    ): BloomFilter! {
        if (bit_count == 0 || num_hashes == 0) return CacheErr.InvalidParameter;

        const num_words = (bit_count + 63) / 64;
        let words = alloc.alloc(u64, num_words) catch return CacheErr.OutOfMemory;
        for (i of 0..num_words) {
            words[i] = 0;
        }

        return BloomFilter{
            allocator: alloc,
            words: words,
            bit_len: bit_count,
            hash_count: num_hashes,
            inserted: 0,
        };
    }

    /// Releases the underlying bitset buffer.
    pub fn deinit(*_) {
        if (_.words.len > 0) {
            _.allocator.free(_.words);
        }
        _.words = [];
        _.bit_len = 0;
        _.hash_count = 0;
        _.inserted = 0;
    }

    /// Returns the total bit capacity `m`.
    pub fn bitCount(): usize {
        return _.bit_len;
    }

    /// Returns the number of hash probes `k` per item.
    pub fn numHashes(): usize {
        return _.hash_count;
    }

    /// Returns the number of `insert` calls recorded since creation or last `clear`.
    pub fn insertedCount(): u64 {
        return _.inserted;
    }

    /// Inserts `item` into the Bloom filter using Kirsch-Mitzenmacher double hashing.
    pub fn insert(*_, item: []const u8) {
        const h1 = fnv1a64(item);
        const h2 = secondaryHash64(item, h1);
        const m_u64 = _.bit_len as u64;

        for (i of 0.._.hash_count) {
            const combined = h1 +% ((i as u64) *% h2);
            const bit_idx = (combined % m_u64) as usize;
            const word_idx = bit_idx >> 6;
            const bit_off: u6 = @truncate(bit_idx);
            _.words[word_idx] |= (1 as u64) << bit_off;
        }
        _.inserted += 1;
    }

    /// Returns `true` if `item` might be present in the set, or `false` if it is definitely absent.
    pub fn mightContain(item: []const u8): bool {
        if (_.bit_len == 0) return false;
        const h1 = fnv1a64(item);
        const h2 = secondaryHash64(item, h1);
        const m_u64 = _.bit_len as u64;

        for (i of 0.._.hash_count) {
            const combined = h1 +% ((i as u64) *% h2);
            const bit_idx = (combined % m_u64) as usize;
            const word_idx = bit_idx >> 6;
            const bit_off: u6 = @truncate(bit_idx);
            if ((_.words[word_idx] & ((1 as u64) << bit_off)) == 0) {
                return false;
            }
        }
        return true;
    }

    /// Merges `other` into `this` via bitwise OR. Requires identical `bitCount()` and `numHashes()`.
    pub fn unionWith(*_, other: BloomFilter): ! {
        if (_.bit_len != other.bit_len || _.hash_count != other.hash_count) {
            return CacheErr.SizeMismatch;
        }
        for (i of 0.._.words.len) {
            _.words[i] |= other.words[i];
        }
        _.inserted = _.inserted +| other.inserted;
    }

    /// Resets all filter bits and the insertion counter to zero while preserving capacity.
    pub fn clear(*_) {
        for (i of 0.._.words.len) {
            _.words[i] = 0;
        }
        _.inserted = 0;
    }

    fn countSetBits(): usize {
        let total: usize = 0;
        for (w of _.words) {
            total += @popCount(w) as usize;
        }
        if (total > _.bit_len) return _.bit_len;
        return total;
    }

    /// Returns the fraction of bits currently set in `[0.0, 1.0]`.
    pub fn fillRatio(): f64 {
        if (_.bit_len == 0) return 0.0;
        return (_.countSetBits() as f64) / (_.bit_len as f64);
    }

    /// Estimates the number of distinct elements inserted using `-(m / k) * ln(1 - X / m)`.
    pub fn estimatedCount(): f64 {
        if (_.bit_len == 0 || _.hash_count == 0) return 0.0;
        const set_bits = _.countSetBits();
        if (set_bits == 0) return 0.0;
        const m_f = _.bit_len as f64;
        const k_f = _.hash_count as f64;
        const x_f = set_bits as f64;
        if (set_bits >= _.bit_len) {
            return -(m_f / k_f) * @log(1.0 / m_f);
        }
        return -(m_f / k_f) * @log(1.0 - (x_f / m_f));
    }
}
