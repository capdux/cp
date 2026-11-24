// Reference-hardened against fjall-rs/lsm-tree & rocksdb (MIT OR Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc 64-Byte Cache-Line Aligned Blocked Bloom Filter using @popCount) | Created: 2026-11-22
// Pure CP zero-heap-allocation Cache-Line Aligned Blocked Bloom Filter (RocksDB / Parquet Split-Block style)
// for SSTable / LSM-Tree storage engines:
//   - 4-lane @Vector(4, u64) SIMD (`BloomMvccVec4u64`) bitmap OR merge (`simdBloomOrBlocks`) &
//     hardware `@popCount` reduction (`simdBloomPopcountWords`) with 0-diff scalar references
//   - Fixed 512-bit (8 x u64 = 64 bytes = 1 CPU cache line) blocks: every probe touches exactly 1 cache line
//   - Double-hashing (FNV-1a + SplitMix64 avalanche) for block selection (h1) and intra-block k-bit probing (h2)
//   - Parameter planner (`recommendBloomParams`) computing optimal block count & probe count for target FPR
//   - Bitwise union (`unionInPlace`) with strict geometry checks & compact binary serialization (`CPB1`)

const std = @import("std");

/// Shared 4-lane u64 vector alias for Blocked Bloom Filter & MVCC ReadView hot paths.
pub type BloomMvccVec4u64 = @Vector(4, u64);

// ============================================================================
// Constants & 4-Lane SIMD Hot-Path Primitives (0-Diff with Scalar)
// ============================================================================

/// Magic bytes `"CPB1"` stored in little-endian (`0x43, 0x50, 0x42, 0x31`).
pub const BLOOM_BLOCK_MAGIC: u32 = 0x31425043;
pub const BLOOM_BLOCK_HEADER_SIZE: usize = 12;
pub const BLOOM_BLOCK_WORDS: usize = 8;
pub const BLOOM_BLOCK_BITS: usize = 512;
pub const BLOOM_BLOCK_BYTES: usize = 64;
pub const BLOOM_BLOCK_MAX_BLOCKS: usize = 256;
pub const BLOOM_BLOCK_MAX_PROBES: u32 = 16;

/// Scalar reference: bitwise OR merges `src` into `dst` word-by-word.
pub fn scalarBloomOrBlocks(dst: *[]u64, src: []const u64) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] |= src[i];
    }
}

/// 4-lane SIMD accelerated bitwise OR merge for 64-byte Bloom filter blocks.
pub fn simdBloomOrBlocks(dst: *[]u64, src: []const u64) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 4 <= n) {
        const vd: BloomMvccVec4u64 = dst[i .. i + 4][0..4].*;
        const vs: BloomMvccVec4u64 = src[i .. i + 4][0..4].*;
        dst[i .. i + 4][0..4].* = vd | vs;
        i += 4;
    }
    while (i < n) : (i += 1) {
        dst[i] |= src[i];
    }
}

/// Scalar reference: total set-bit count across `words` using `@popCount`.
pub fn scalarBloomPopcountWords(words: []const u64): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < words.len) : (i += 1) {
        total += @popCount(words[i]) as usize;
    }
    return total;
}

/// 4-lane SIMD accelerated total set-bit count across `words` using vector `@popCount`.
pub fn simdBloomPopcountWords(words: []const u64): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i + 4 <= words.len) {
        const v: BloomMvccVec4u64 = words[i .. i + 4][0..4].*;
        const pc = @popCount(v);
        total += (pc[0] as usize) + (pc[1] as usize) + (pc[2] as usize) + (pc[3] as usize);
        i += 4;
    }
    while (i < words.len) : (i += 1) {
        total += @popCount(words[i]) as usize;
    }
    return total;
}

fn bbIsFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

const BB_SPLITMIX_GAMMA: u64 = ((0x9E3779B9 as u64) << 32) | (0x7F4A7C15 as u64);
const BB_SPLITMIX_M1: u64 = ((0xBF58476D as u64) << 32) | (0x1CE4E5B9 as u64);
const BB_SPLITMIX_M2: u64 = ((0x94D049BB as u64) << 32) | (0x133111EB as u64);
const BB_FNV_OFFSET: u64 = ((0xCBF29CE4 as u64) << 32) | (0x84222325 as u64);
const BB_FNV_PRIME: u64 = ((0x00000100 as u64) << 32) | (0x000001B3 as u64);

/// SplitMix64 avalanche mixer for high-quality independent hash derivation.
pub fn bloomSplitMix64(x: u64): u64 {
    let z: u64 = x +% BB_SPLITMIX_GAMMA;
    z = (z ^ (z >> 30)) *% BB_SPLITMIX_M1;
    z = (z ^ (z >> 27)) *% BB_SPLITMIX_M2;
    return z ^ (z >> 31);
}

/// Deterministic 64-bit byte-slice hash combining FNV-1a with SplitMix64 finalization.
pub fn bloomHashBytes(data: []const u8): u64 {
    let h: u64 = BB_FNV_OFFSET;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h ^= data[i] as u64;
        h = h *% BB_FNV_PRIME;
    }
    return bloomSplitMix64(h ^ ((data.len as u64) *% BB_SPLITMIX_GAMMA));
}

fn bbWriteU32Le(dst: *[]u8, offset: usize, val: u32) {
    dst[offset + 0] = (val & 0xFF) as u8;
    dst[offset + 1] = ((val >> 8) & 0xFF) as u8;
    dst[offset + 2] = ((val >> 16) & 0xFF) as u8;
    dst[offset + 3] = ((val >> 24) & 0xFF) as u8;
}

fn bbReadU32Le(src: []const u8, offset: usize): u32 {
    return (src[offset + 0] as u32)
        | ((src[offset + 1] as u32) << 8)
        | ((src[offset + 2] as u32) << 16)
        | ((src[offset + 3] as u32) << 24);
}

fn bbWriteU64Le(dst: *[]u8, offset: usize, val: u64) {
    let b: usize = 0;
    while (b < 8) : (b += 1) {
        const shift: u6 = (b * 8) as u6;
        dst[offset + b] = ((val >> shift) & 0xFF) as u8;
    }
}

fn bbReadU64Le(src: []const u8, offset: usize): u64 {
    let out: u64 = 0;
    let b: usize = 0;
    while (b < 8) : (b += 1) {
        const shift: u6 = (b * 8) as u6;
        out |= (src[offset + b] as u64) << shift;
    }
    return out;
}

// ============================================================================
// 1. Parameter Planner, Block, and Stats Models
// ============================================================================

/// Recommended Blocked Bloom Filter geometry computed by `recommendBloomParams`.
pub struct BloomBlockParams {
    num_blocks: usize = 1;
    num_probes: u32 = 7;
    bits_per_key: f64 = 10.0;
    estimated_fpr: f64 = 0.01;
}

/// Live occupancy and false-positive rate diagnostics for `BlockedBloomFilter`.
pub struct BloomBlockStats {
    num_blocks: usize = 0;
    num_probes: u32 = 0;
    total_bits: usize = 0;
    set_bits: usize = 0;
    fill_ratio: f64 = 0.0;
    estimated_fpr: f64 = 0.0;
    serialized_bytes: usize = 0;
    inserted_count: u64 = 0;
}

/// Computes recommended `num_blocks` (within `[1, BLOOM_BLOCK_MAX_BLOCKS]`) and `num_probes`
/// (within `[1, BLOOM_BLOCK_MAX_PROBES]`) for `expected_keys` and `target_fpr ∈ (0.0, 1.0)`.
/// Returns `null` if `expected_keys == 0` or `target_fpr` is non-finite / outside `(0.0, 1.0)`.
pub fn recommendBloomParams(expected_keys: usize, target_fpr: f64): BloomBlockParams? {
    if (expected_keys == 0) {
        return null;
    }
    if (!bbIsFiniteF64(target_fpr) || target_fpr <= 0.0 || target_fpr >= 1.0) {
        return null;
    }

    const ln2: f64 = 0.6931471805599453;
    const ln2_sq: f64 = ln2 * ln2;
    const n_f: f64 = expected_keys as f64;
    const raw_bits: f64 = -(n_f * @log(target_fpr)) / ln2_sq;

    let blocks_f: f64 = @ceil(raw_bits / (BLOOM_BLOCK_BITS as f64));
    if (blocks_f < 1.0) {
        blocks_f = 1.0;
    }
    let num_blocks: usize = if (blocks_f >= (BLOOM_BLOCK_MAX_BLOCKS as f64)) {
        BLOOM_BLOCK_MAX_BLOCKS
    } else {
        blocks_f as usize
    };
    if (num_blocks == 0) {
        num_blocks = 1;
    }

    const total_bits_f: f64 = (num_blocks * BLOOM_BLOCK_BITS) as f64;
    const actual_bpk: f64 = total_bits_f / n_f;
    const raw_k: f64 = @round(actual_bpk * ln2);

    let k: u32 = 1;
    if (raw_k >= (BLOOM_BLOCK_MAX_PROBES as f64)) {
        k = BLOOM_BLOCK_MAX_PROBES;
    } else if (raw_k >= 1.0) {
        k = raw_k as u32;
    }

    const k_f: f64 = k as f64;
    const est_fill: f64 = 1.0 - @exp(-k_f / actual_bpk);
    const est_fpr: f64 = std.math.pow(f64, est_fill, k_f);

    return BloomBlockParams {
        num_blocks: num_blocks,
        num_probes: k,
        bits_per_key: actual_bpk,
        estimated_fpr: est_fpr,
    };
}

/// Single 64-byte (512-bit = 8 x u64) cache-line aligned Bloom Filter block.
pub struct BloomBlock {
    words: [BLOOM_BLOCK_WORDS]u64 = [0; BLOOM_BLOCK_WORDS];

    /// Sets bit at `bit_idx & 511` within this 512-bit block.
    fn setBit(*_, bit_idx: usize) {
        const idx: usize = bit_idx & 511;
        const word_i: usize = idx >> 6;
        const bit_shift: u6 = (idx & 63) as u6;
        _.words[word_i] |= (1 as u64) << bit_shift;
    }

    /// Tests bit at `bit_idx & 511` within this 512-bit block.
    fn testBit(bit_idx: usize): bool {
        const idx: usize = bit_idx & 511;
        const word_i: usize = idx >> 6;
        const bit_shift: u6 = (idx & 63) as u6;
        return (_.words[word_i] & ((1 as u64) << bit_shift)) != 0;
    }

    /// Exact number of set bits in this 512-bit block using 4-lane SIMD `@popCount`.
    fn popCount(): usize {
        return simdBloomPopcountWords(_.words[0..]);
    }

    /// Resets all 512 bits to zero.
    fn clear(*_) {
        let i: usize = 0;
        while (i < BLOOM_BLOCK_WORDS) : (i += 1) {
            _.words[i] = 0;
        }
    }
}

// ============================================================================
// 2. Zero-Alloc Cache-Line Aligned Blocked Bloom Filter
// ============================================================================

/// Zero-heap-allocation Cache-Line Aligned Blocked Bloom Filter backed by up to
/// `BLOOM_BLOCK_MAX_BLOCKS` (256) 64-byte blocks (131,072 bits max).
pub struct BlockedBloomFilter {
    blocks: [BLOOM_BLOCK_MAX_BLOCKS]BloomBlock = [BloomBlock {}; BLOOM_BLOCK_MAX_BLOCKS];
    num_blocks: usize = 0;
    num_probes: u32 = 0;
    inserted_count: u64 = 0;

    /// Creates a Blocked Bloom Filter with `num_blocks ∈ [1, 256]` and `num_probes ∈ [1, 16]`.
    /// Returns `null` on invalid geometry.
    static fn init(num_blocks: usize, num_probes: u32): BlockedBloomFilter? {
        if (num_blocks == 0 || num_blocks > BLOOM_BLOCK_MAX_BLOCKS) {
            return null;
        }
        if (num_probes == 0 || num_probes > BLOOM_BLOCK_MAX_PROBES) {
            return null;
        }
        return BlockedBloomFilter {
            num_blocks: num_blocks,
            num_probes: num_probes,
        };
    }

    /// Creates a Blocked Bloom Filter directly from `recommendBloomParams` output.
    static fn fromParams(params: BloomBlockParams): BlockedBloomFilter? {
        return BlockedBloomFilter.init(params.num_blocks, params.num_probes);
    }

    /// Inserts a pre-hashed 64-bit key into a single cache-line block.
    fn insertHash(*_, hash: u64) {
        if (_.num_blocks == 0 || _.num_probes == 0) {
            return;
        }
        const h1: u64 = bloomSplitMix64(hash);
        const h2: u64 = bloomSplitMix64(h1 ^ BB_SPLITMIX_GAMMA) | 1;
        const block_idx: usize = (h1 as usize) % _.num_blocks;

        let cur: u64 = h1;
        let k: u32 = 0;
        while (k < _.num_probes) : (k += 1) {
            const bit_pos: usize = ((cur >> 23) as usize) & 511;
            _.blocks[block_idx].setBit(bit_pos);
            cur = cur +% h2;
        }
        _.inserted_count += 1;
    }

    /// Hashes `key` via FNV-1a + SplitMix64 and inserts it into a single cache-line block.
    fn insertBytes(*_, key: []const u8) {
        _.insertHash(bloomHashBytes(key));
    }

    /// Probes a pre-hashed 64-bit key within a single 64-byte cache line.
    /// Zero false negatives for all previously inserted keys.
    fn mayContainHash(hash: u64): bool {
        if (_.num_blocks == 0 || _.num_probes == 0) {
            return false;
        }
        const h1: u64 = bloomSplitMix64(hash);
        const h2: u64 = bloomSplitMix64(h1 ^ BB_SPLITMIX_GAMMA) | 1;
        const block_idx: usize = (h1 as usize) % _.num_blocks;

        let cur: u64 = h1;
        let k: u32 = 0;
        while (k < _.num_probes) : (k += 1) {
            const bit_pos: usize = ((cur >> 23) as usize) & 511;
            if (!_.blocks[block_idx].testBit(bit_pos)) {
                return false;
            }
            cur = cur +% h2;
        }
        return true;
    }

    /// Hashes `key` and probes the filter within a single 64-byte cache line.
    fn mayContainBytes(key: []const u8): bool {
        return _.mayContainHash(bloomHashBytes(key));
    }

    /// Bitwise-OR merges another isomorphic `BlockedBloomFilter` into this filter in-place
    /// using 4-lane SIMD (`simdBloomOrBlocks`).
    /// Atomically returns `false` without mutation if `num_blocks` or `num_probes` differ.
    fn unionInPlace(*_, other: BlockedBloomFilter): bool {
        if (_.num_blocks == 0 || _.num_blocks != other.num_blocks || _.num_probes != other.num_probes) {
            return false;
        }
        let b: usize = 0;
        while (b < _.num_blocks) : (b += 1) {
            simdBloomOrBlocks(_.blocks[b].words[0..], other.blocks[b].words[0..]);
        }
        _.inserted_count += other.inserted_count;
        return true;
    }

    /// Total bit capacity (`num_blocks * 512`).
    fn totalBits(): usize {
        return _.num_blocks * BLOOM_BLOCK_BITS;
    }

    /// Exact count of set bits across all active blocks via 4-lane SIMD `@popCount`.
    fn setBitCount(): usize {
        let total: usize = 0;
        let b: usize = 0;
        while (b < _.num_blocks) : (b += 1) {
            total += _.blocks[b].popCount();
        }
        return total;
    }

    /// Observed bit fill density (`set_bits / total_bits ∈ [0.0, 1.0]`).
    fn fillRatio(): f64 {
        const total: usize = _.totalBits();
        if (total == 0) {
            return 0.0;
        }
        return (_.setBitCount() as f64) / (total as f64);
    }

    /// Live false-positive rate estimate derived from exact `@popCount` fill density: `fill_ratio ^ num_probes`.
    fn estimateFalsePositiveRate(): f64 {
        if (_.num_blocks == 0 || _.num_probes == 0) {
            return 0.0;
        }
        const fill: f64 = _.fillRatio();
        if (fill <= 0.0) {
            return 0.0;
        }
        if (fill >= 1.0) {
            return 1.0;
        }
        return std.math.pow(f64, fill, _.num_probes as f64);
    }

    /// Exact byte length required to serialize this filter (`12 + num_blocks * 64`).
    fn serializedSize(): usize {
        return BLOOM_BLOCK_HEADER_SIZE + _.num_blocks * BLOOM_BLOCK_BYTES;
    }

    /// Serializes header (`"CPB1"` + `num_blocks` + `num_probes`) and raw little-endian block
    /// bitmaps into `out`. Returns total bytes written, or `null` if `out` is too small.
    fn serialize(out: *[]u8): usize? {
        if (_.num_blocks == 0 || _.num_probes == 0) {
            return null;
        }
        const needed: usize = _.serializedSize();
        if (out.len < needed) {
            return null;
        }

        bbWriteU32Le(out, 0, BLOOM_BLOCK_MAGIC);
        bbWriteU32Le(out, 4, _.num_blocks as u32);
        bbWriteU32Le(out, 8, _.num_probes);

        let off: usize = BLOOM_BLOCK_HEADER_SIZE;
        let b: usize = 0;
        while (b < _.num_blocks) : (b += 1) {
            let w: usize = 0;
            while (w < BLOOM_BLOCK_WORDS) : (w += 1) {
                bbWriteU64Le(out, off, _.blocks[b].words[w]);
                off += 8;
            }
        }
        return needed;
    }

    /// Zero-copy validates whether `data` is a well-formed serialized `BlockedBloomFilter`
    /// SSTable filter block (valid `"CPB1"` magic, legal bounds, exact payload byte length).
    static fn verifySerializedBytes(data: []const u8): bool {
        if (data.len < BLOOM_BLOCK_HEADER_SIZE) {
            return false;
        }
        const magic: u32 = bbReadU32Le(data, 0);
        if (magic != BLOOM_BLOCK_MAGIC) {
            return false;
        }
        const nb: usize = bbReadU32Le(data, 4) as usize;
        const np: u32 = bbReadU32Le(data, 8);
        if (nb == 0 || nb > BLOOM_BLOCK_MAX_BLOCKS) {
            return false;
        }
        if (np == 0 || np > BLOOM_BLOCK_MAX_PROBES) {
            return false;
        }
        const expected_len: usize = BLOOM_BLOCK_HEADER_SIZE + nb * BLOOM_BLOCK_BYTES;
        return data.len == expected_len;
    }

    /// Deserializes a `BlockedBloomFilter` from a compact binary SSTable filter block.
    /// Returns `null` on invalid magic, out-of-range header params, or truncated/oversized payload.
    static fn deserialize(data: []const u8): BlockedBloomFilter? {
        if (!BlockedBloomFilter.verifySerializedBytes(data)) {
            return null;
        }
        const nb: usize = bbReadU32Le(data, 4) as usize;
        const np: u32 = bbReadU32Le(data, 8);

        let filter = BlockedBloomFilter {
            num_blocks: nb,
            num_probes: np,
        };
        let off: usize = BLOOM_BLOCK_HEADER_SIZE;
        let b: usize = 0;
        while (b < nb) : (b += 1) {
            let w: usize = 0;
            while (w < BLOOM_BLOCK_WORDS) : (w += 1) {
                filter.blocks[b].words[w] = bbReadU64Le(data, off);
                off += 8;
            }
        }
        return filter;
    }

    /// Returns a comprehensive live occupancy & FPR diagnostic snapshot.
    fn stats(): BloomBlockStats {
        const total_b: usize = _.totalBits();
        const set_b: usize = _.setBitCount();
        const fill: f64 = if (total_b == 0) 0.0 else ((set_b as f64) / (total_b as f64));
        const fpr: f64 = if (fill <= 0.0 || _.num_probes == 0) {
            0.0
        } else if (fill >= 1.0) {
            1.0
        } else {
            std.math.pow(f64, fill, _.num_probes as f64)
        };
        return BloomBlockStats {
            num_blocks: _.num_blocks,
            num_probes: _.num_probes,
            total_bits: total_b,
            set_bits: set_b,
            fill_ratio: fill,
            estimated_fpr: fpr,
            serialized_bytes: _.serializedSize(),
            inserted_count: _.inserted_count,
        };
    }

    /// Clears all bits and resets `inserted_count` while preserving geometry.
    fn clear(*_) {
        let b: usize = 0;
        while (b < _.num_blocks) : (b += 1) {
            _.blocks[b].clear();
        }
        _.inserted_count = 0;
    }
}
