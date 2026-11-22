// Origin: Pure-CP Stdlib Test Suite (lib/db/bloomBlock.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Cache-Line Aligned Blocked Bloom Filter:
//   1. recommendBloomParams planning & invalid input guards (0 keys, out-of-range/NaN/Inf FPR)
//   2. 1000-key zero-false-negative invariant & 2000-probe empirical FPR verification (~1%)
//   3. Single cache-line (64B / 512-bit block) locality proof via per-block @popCount
//   4. Live @popCount setBitCount / fillRatio / estimateFalsePositiveRate / stats & clear()
//   5. Isomorphic unionInPlace bitwise merge & non-isomorphic atomic rejection guards
//   6. Compact binary serialization (CPB1 magic) & lossless deserialization round-trip
//   7. Malformed stream & geometry boundary guards (truncated, bad magic, oversized, invalid bounds)
//   8. Empty key "", minimal (1 block, 1 probe), & max-scale (256 blocks = 16KB) stress test

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BLOOM_BLOCK_MAGIC,
    BLOOM_BLOCK_HEADER_SIZE,
    BLOOM_BLOCK_BITS,
    BLOOM_BLOCK_BYTES,
    BLOOM_BLOCK_MAX_BLOCKS,
    BLOOM_BLOCK_MAX_PROBES,
    bloomSplitMix64,
    bloomHashBytes,
    BloomBlockParams,
    BloomBlockStats,
    recommendBloomParams,
    BloomBlock,
    BlockedBloomFilter,
} from "cap:db";

test("bloomBlock: recommendBloomParams accuracy & invalid input guards", () => {
    // Invalid expected_keys and target_fpr bounds
    expect_eq(recommendBloomParams(0, 0.01) == null, true);
    expect_eq(recommendBloomParams(100, 0.0) == null, true);
    expect_eq(recommendBloomParams(100, -0.05) == null, true);
    expect_eq(recommendBloomParams(100, 1.0) == null, true);
    expect_eq(recommendBloomParams(100, 1.5) == null, true);
    expect_eq(recommendBloomParams(100, std.math.nan(f64)) == null, true);
    expect_eq(recommendBloomParams(100, std.math.inf(f64)) == null, true);

    // 1000 keys @ 1% FPR -> ~9585 bits -> 19 blocks (9728 bits), k ~= 7
    const p1 = recommendBloomParams(1000, 0.01) ?? unreachable;
    expect_eq(p1.num_blocks >= 18 && p1.num_blocks <= 20, true);
    expect_eq(p1.num_probes >= 6 && p1.num_probes <= 8, true);
    expect_eq(p1.estimated_fpr > 0.0 && p1.estimated_fpr < 0.02, true);

    // Huge expected_keys caps cleanly at BLOOM_BLOCK_MAX_BLOCKS
    const p_big = recommendBloomParams(1000000, 0.001) ?? unreachable;
    expect_eq(p_big.num_blocks, BLOOM_BLOCK_MAX_BLOCKS);
    expect_eq(p_big.num_probes >= 1 && p_big.num_probes <= BLOOM_BLOCK_MAX_PROBES, true);

    // Tiny key count uses at least 1 block and valid k <= 16
    const p_tiny = recommendBloomParams(1, 0.01) ?? unreachable;
    expect_eq(p_tiny.num_blocks, 1 as usize);
    expect_eq(p_tiny.num_probes >= 1 && p_tiny.num_probes <= BLOOM_BLOCK_MAX_PROBES, true);
});

test("bloomBlock: zero false negatives & ~1% empirical FPR over 1000 inserted / 2000 absent keys", () => {
    const params = recommendBloomParams(1000, 0.01) ?? unreachable;
    let bf = BlockedBloomFilter.fromParams(params) ?? unreachable;

    // Insert 1000 distinct 64-bit hashes
    let i: u64 = 0;
    while (i < 1000) : (i += 1) {
        bf.insertHash(i *% 2654435761 +% 17);
    }
    expect_eq(bf.stats().inserted_count, 1000 as u64);

    // Zero false negatives on all 1000 inserted keys
    i = 0;
    while (i < 1000) : (i += 1) {
        expect_eq(bf.mayContainHash(i *% 2654435761 +% 17), true);
    }

    // Measure empirical false positive rate over 2000 disjoint keys
    let fp: usize = 0;
    let j: u64 = 100000;
    while (j < 102000) : (j += 1) {
        if (bf.mayContainHash(j *% 2654435761 +% 17)) {
            fp += 1;
        }
    }
    // Blocked Bloom Filter at ~10 bits/key has empirical FPR ~1.1%-1.6%; allow generous 3.5% bound
    expect_eq(fp < 70, true);
});

test("bloomBlock: strict single 64-byte cache-line locality per key", () => {
    let bf = BlockedBloomFilter.init(32, 7) ?? unreachable;
    bf.insertBytes("sstable:row:alpha:0001");

    // Exactly 1 out of 32 blocks must have non-zero bits; all other 31 blocks must remain untouched
    let touched_blocks: usize = 0;
    let total_set: usize = 0;
    let b: usize = 0;
    while (b < 32) : (b += 1) {
        const pc: usize = bf.blocks[b].popCount();
        if (pc > 0) {
            touched_blocks += 1;
            total_set += pc;
        }
    }
    expect_eq(touched_blocks, 1 as usize);
    expect_eq(total_set >= 1 && total_set <= 7, true);
    expect_eq(bf.setBitCount(), total_set);
    expect_eq(bf.mayContainBytes("sstable:row:alpha:0001"), true);
});

test("bloomBlock: @popCount setBitCount, fillRatio, live FPR estimation & clear()", () => {
    let bf = BlockedBloomFilter.init(4, 6) ?? unreachable;

    // Empty filter invariants
    expect_eq(bf.totalBits(), 4 * BLOOM_BLOCK_BITS);
    expect_eq(bf.setBitCount(), 0 as usize);
    expect_eq(bf.fillRatio() == 0.0, true);
    expect_eq(bf.estimateFalsePositiveRate() == 0.0, true);

    // Insert 150 keys
    let i: u64 = 0;
    while (i < 150) : (i += 1) {
        bf.insertHash(bloomSplitMix64(i + 42));
    }

    const st = bf.stats();
    expect_eq(st.num_blocks, 4 as usize);
    expect_eq(st.num_probes, 6 as u32);
    expect_eq(st.total_bits, 2048 as usize);
    expect_eq(st.set_bits > 400 && st.set_bits < 1400, true);
    expect_eq(st.fill_ratio > 0.2 && st.fill_ratio < 0.7, true);
    expect_eq(st.estimated_fpr > 0.0 && st.estimated_fpr < 0.15, true);
    expect_eq(st.serialized_bytes, BLOOM_BLOCK_HEADER_SIZE + 4 * BLOOM_BLOCK_BYTES);

    // Clear resets all bits and counters while preserving geometry
    bf.clear();
    expect_eq(bf.setBitCount(), 0 as usize);
    expect_eq(bf.fillRatio() == 0.0, true);
    expect_eq(bf.estimateFalsePositiveRate() == 0.0, true);
    expect_eq(bf.stats().inserted_count, 0 as u64);
    expect_eq(bf.mayContainHash(bloomSplitMix64(42)), false);
});

test("bloomBlock: isomorphic unionInPlace & non-isomorphic atomic rejection", () => {
    let a = BlockedBloomFilter.init(8, 5) ?? unreachable;
    let b = BlockedBloomFilter.init(8, 5) ?? unreachable;

    a.insertBytes("tenant:a:key1");
    a.insertBytes("tenant:a:key2");
    b.insertBytes("tenant:b:key1");
    b.insertBytes("tenant:b:key2");

    const bits_a: usize = a.setBitCount();
    const bits_b: usize = b.setBitCount();

    expect_eq(a.unionInPlace(b), true);
    expect_eq(a.mayContainBytes("tenant:a:key1"), true);
    expect_eq(a.mayContainBytes("tenant:a:key2"), true);
    expect_eq(a.mayContainBytes("tenant:b:key1"), true);
    expect_eq(a.mayContainBytes("tenant:b:key2"), true);
    expect_eq(a.setBitCount() >= bits_a, true);
    expect_eq(a.setBitCount() <= bits_a + bits_b, true);
    expect_eq(a.stats().inserted_count, 4 as u64);

    // Non-isomorphic filters (different num_blocks or num_probes) must be rejected without mutation
    const diff_blocks = BlockedBloomFilter.init(16, 5) ?? unreachable;
    const diff_probes = BlockedBloomFilter.init(8, 7) ?? unreachable;
    const before_bits: usize = a.setBitCount();
    expect_eq(a.unionInPlace(diff_blocks), false);
    expect_eq(a.unionInPlace(diff_probes), false);
    expect_eq(a.setBitCount(), before_bits);
});

test("bloomBlock: CPB1 compact binary serialization & lossless deserialization round-trip", () => {
    let bf = BlockedBloomFilter.init(6, 7) ?? unreachable;
    bf.insertBytes("doc:alpha");
    bf.insertBytes("doc:beta");
    bf.insertBytes("doc:gamma");
    bf.insertHash(0xDEADBEEFCAFEBABE);

    const expected_size: usize = BLOOM_BLOCK_HEADER_SIZE + 6 * BLOOM_BLOCK_BYTES;
    expect_eq(bf.serializedSize(), expected_size);

    let buf: [512]u8 = [0; 512];
    let s: []u8 = buf[0..];
    const written = bf.serialize(*s) ?? unreachable;
    expect_eq(written, expected_size);

    // Check ASCII magic "CPB1" in header bytes 0..4
    expect_eq(buf[0], 0x43 as u8); // 'C'
    expect_eq(buf[1], 0x50 as u8); // 'P'
    expect_eq(buf[2], 0x42 as u8); // 'B'
    expect_eq(buf[3], 0x31 as u8); // '1'

    expect_eq(BlockedBloomFilter.verifySerializedBytes(buf[0..written]), true);

    const restored = BlockedBloomFilter.deserialize(buf[0..written]) ?? unreachable;
    expect_eq(restored.num_blocks, 6 as usize);
    expect_eq(restored.num_probes, 7 as u32);
    expect_eq(restored.setBitCount(), bf.setBitCount());
    expect_eq(restored.mayContainBytes("doc:alpha"), true);
    expect_eq(restored.mayContainBytes("doc:beta"), true);
    expect_eq(restored.mayContainBytes("doc:gamma"), true);
    expect_eq(restored.mayContainHash(0xDEADBEEFCAFEBABE), true);
    expect_eq(restored.mayContainBytes("doc:never-inserted-xyz-999"), false);
});

test("bloomBlock: geometry bounds & malformed serialized stream guards", () => {
    // Invalid init geometry
    expect_eq(BlockedBloomFilter.init(0, 7) == null, true);
    expect_eq(BlockedBloomFilter.init(BLOOM_BLOCK_MAX_BLOCKS + 1, 7) == null, true);
    expect_eq(BlockedBloomFilter.init(4, 0) == null, true);
    expect_eq(BlockedBloomFilter.init(4, BLOOM_BLOCK_MAX_PROBES + 1) == null, true);

    // Undersized serialize buffer returns null
    const bf = BlockedBloomFilter.init(2, 5) ?? unreachable;
    let tiny: [32]u8 = [0; 32];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(bf.serialize(*tiny_s) == null, true);

    // Valid serialized payload for mutation testing
    let valid_buf: [256]u8 = [0; 256];
    let valid_s: []u8 = valid_buf[0..];
    const n = bf.serialize(*valid_s) ?? unreachable;

    // 1. Truncated header (< 12B)
    expect_eq(BlockedBloomFilter.verifySerializedBytes(valid_buf[0..8]), false);
    expect_eq(BlockedBloomFilter.deserialize(valid_buf[0..8]) == null, true);

    // 2. Truncated payload
    expect_eq(BlockedBloomFilter.verifySerializedBytes(valid_buf[0..(n - 1)]), false);
    expect_eq(BlockedBloomFilter.deserialize(valid_buf[0..(n - 1)]) == null, true);

    // 3. Trailing garbage bytes
    expect_eq(BlockedBloomFilter.verifySerializedBytes(valid_buf[0..(n + 1)]), false);
    expect_eq(BlockedBloomFilter.deserialize(valid_buf[0..(n + 1)]) == null, true);

    // 4. Corrupted magic
    let bad_magic: [256]u8 = valid_buf;
    bad_magic[0] = 0x58; // 'X' instead of 'C'
    expect_eq(BlockedBloomFilter.verifySerializedBytes(bad_magic[0..n]), false);
    expect_eq(BlockedBloomFilter.deserialize(bad_magic[0..n]) == null, true);

    // 5. Out-of-range num_probes in header (17 > 16)
    let bad_probes: [256]u8 = valid_buf;
    bad_probes[8] = 17;
    expect_eq(BlockedBloomFilter.verifySerializedBytes(bad_probes[0..n]), false);
    expect_eq(BlockedBloomFilter.deserialize(bad_probes[0..n]) == null, true);

    // 6. Zero num_blocks in header
    let bad_blocks: [256]u8 = valid_buf;
    bad_blocks[4] = 0;
    expect_eq(BlockedBloomFilter.verifySerializedBytes(bad_blocks[0..n]), false);
    expect_eq(BlockedBloomFilter.deserialize(bad_blocks[0..n]) == null, true);
});

test("bloomBlock: empty key, minimal geometry (1x1) & max-scale (256 blocks) stress", () => {
    // Minimal 1 block, 1 probe + empty key ""
    let mini = BlockedBloomFilter.init(1, 1) ?? unreachable;
    expect_eq(mini.mayContainBytes(""), false);
    mini.insertBytes("");
    expect_eq(mini.mayContainBytes(""), true);
    expect_eq(mini.setBitCount(), 1 as usize);

    // Max-scale 256 blocks (131,072 bits = 16 KB) with 5000 keys
    let big = BlockedBloomFilter.init(BLOOM_BLOCK_MAX_BLOCKS, 8) ?? unreachable;
    let i: u64 = 0;
    while (i < 5000) : (i += 1) {
        big.insertHash(bloomSplitMix64(i + 10000));
    }
    expect_eq(big.totalBits(), 131072 as usize);

    // All 5000 keys must be found (zero false negatives)
    i = 0;
    while (i < 5000) : (i += 1) {
        expect_eq(big.mayContainHash(bloomSplitMix64(i + 10000)), true);
    }
    // Live FPR at 5000 keys / 131072 bits (~26.2 bits/key, k=8) should be very low (< 0.1%)
    expect_eq(big.estimateFalsePositiveRate() < 0.001, true);
});
