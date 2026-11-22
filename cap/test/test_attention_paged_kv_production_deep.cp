// Origin: Pure-CP Stdlib Test Suite (lib/attention/paged_kv.cp) | Created: 2026-11-22
// Production Deep Verification Suite for vLLM-Style PagedAttention KV-Cache & CoW Manager:
//   1. PagedKvBlockPool alloc/retain/release, high-watermark, utilization & exhaustion guards
//   2. SeqBlockTable cross-block append & zero-leak releaseAll
//   3. Zero-copy forkSequence + Copy-on-Write isolation (parent never polluted, cow_copies tracked)
//   4. SIMD @dot/@fma pagedAttentionDecodeF32 exact equivalence vs naive contiguous attention over fragmented blocks
//   5. Divergent Beam Search decode (shared prefix + CoW diverged tail -> distinct exact outputs)
//   6. Pool exhaustion, invalid init, NaN query & out-of-bounds defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PAGED_KV_INVALID_BLOCK,
    PagedKvPoolStats,
    PagedKvBlockPool,
    PagedTokenSlot,
    SeqBlockTable,
    pagedAttentionDecodeF32,
} from "cap:attention";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

/// Reference naive contiguous single-head attention for exact numerical equivalence testing.
fn naiveContiguousAttentionF32(
    query: []const f32,
    k_flat: []const f32,
    v_flat: []const f32,
    seq_len: usize,
    head_dim: usize,
    scale: f32,
    out_vec: *[]f32,
) {
    const eff_scale: f32 = if (scale > 0.0) scale else (1.0 / @sqrt(head_dim as f32));
    let scores: [16]f32 = [0.0; 16];
    let max_s: f32 = 0.0 - std.math.inf(f32);

    let t: usize = 0;
    while (t < seq_len) : (t += 1) {
        let dot: f32 = 0.0;
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            dot += query[d] * k_flat[t * head_dim + d];
        }
        scores[t] = dot * eff_scale;
        if (scores[t] > max_s) max_s = scores[t];
    }

    let sum_exp: f32 = 0.0;
    t = 0;
    while (t < seq_len) : (t += 1) {
        scores[t] = std.math.exp(scores[t] - max_s);
        sum_exp += scores[t];
    }

    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        let acc: f32 = 0.0;
        t = 0;
        while (t < seq_len) : (t += 1) {
            acc += (scores[t] / sum_exp) * v_flat[t * head_dim + d];
        }
        out_vec[d] = acc;
    }
}

// ============================================================================
// Test 1: PagedKvBlockPool Refcounting, High-Watermark & Exhaustion
// ============================================================================

fn testBlockPoolRefcountingAndStats()! {
    let refs: [3]u32 = [0; 3];
    let refs_s: []u32 = refs[0..];
    let pool = cap.attention.PagedKvBlockPool.init(refs_s) ?? return error.InitFailed;

    expect_eq(pool.freeBlocks(), 3);
    expect_eq(pool.allocatedBlocks(), 0);

    const b0 = pool.allocBlock() ?? return error.AllocFailed;
    const b1 = pool.allocBlock() ?? return error.AllocFailed;
    const b2 = pool.allocBlock() ?? return error.AllocFailed;
    expect_eq(pool.allocBlock() == null, true); // Pool exhausted

    expect_eq(pool.refCount(b0), 1);
    expect_eq(pool.retainBlock(b0), true);
    expect_eq(pool.refCount(b0), 2);

    // Releasing b0 once drops refcount to 1 (still allocated!)
    expect_eq(pool.releaseBlock(b0), true);
    expect_eq(pool.refCount(b0), 1);
    expect_eq(pool.allocatedBlocks(), 3);

    // Releasing b0 second time frees it
    expect_eq(pool.releaseBlock(b0), true);
    expect_eq(pool.refCount(b0), 0);
    expect_eq(pool.allocatedBlocks(), 2);
    expect_eq(pool.freeBlocks(), 1);

    // Double release & retaining free block rejected
    expect_eq(pool.releaseBlock(b0), false);
    expect_eq(pool.retainBlock(b0), false);
    expect_eq(pool.releaseBlock(99), false);

    const st = pool.stats();
    expect_eq(st.total_blocks, 3);
    expect_eq(st.allocated_blocks, 2);
    expect_eq(st.free_blocks, 1);
    expect_eq(st.high_watermark, 3);
    expect_eq(absF32(st.utilization - (2.0 as f32) / 3.0) < 0.001, true);

    * = pool.releaseBlock(b1);
    * = pool.releaseBlock(b2);
    expect_eq(pool.freeBlocks(), 3);
}

// ============================================================================
// Test 2: SeqBlockTable Cross-Block Append & Zero-Leak Release
// ============================================================================

fn testSeqBlockTableCrossBlockAndZeroLeak()! {
    let refs: [4]u32 = [0; 4];
    let k_cache: [32]f32 = [0.0; 32]; // 4 blocks * block_size 2 * head_dim 4
    let v_cache: [32]f32 = [0.0; 32];
    let pool = PagedKvBlockPool.init(refs[0..]) ?? return error.InitFailed;

    let blks: [4]u32 = [0; 4];
    let seq = SeqBlockTable.init(blks[0..], 2) ?? return error.InitFailed;

    // Append 5 tokens across 3 blocks: (b0,s0), (b0,s1), (b1,s0), (b1,s1), (b2,s0)
    let t: usize = 0;
    while (t < 5) : (t += 1) {
        const slot = seq.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;
        expect_eq(slot.logical_block, t / 2);
        expect_eq(slot.slot_in_block, t % 2);
        expect_eq(slot.did_cow, false);
    }

    expect_eq(seq.seq_len, 5);
    expect_eq(seq.num_blocks, 3);
    expect_eq(pool.allocatedBlocks(), 3);

    // Release all -> 0 leaked blocks
    seq.releaseAll(&pool);
    expect_eq(seq.seq_len, 0);
    expect_eq(seq.num_blocks, 0);
    expect_eq(pool.allocatedBlocks(), 0);
    expect_eq(pool.freeBlocks(), 4);
}

// ============================================================================
// Test 3: Zero-Copy Fork & Copy-on-Write Isolation
// ============================================================================

fn testZeroCopyForkAndCowIsolation()! {
    // 6 physical blocks, block_size = 2, head_dim = 4
    let refs: [6]u32 = [0; 6];
    let k_cache: [48]f32 = [0.0; 48];
    let v_cache: [48]f32 = [0.0; 48];
    let pool = PagedKvBlockPool.init(refs[0..]) ?? return error.InitFailed;

    let p_blks: [4]u32 = [0; 4];
    let parent = SeqBlockTable.init(p_blks[0..], 2) ?? return error.InitFailed;

    // Write 3 prefix tokens into parent (logical block 0 full, logical block 1 half-full)
    const k_tok: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    const v_p0: [4]f32 = [10.0, 10.0, 10.0, 10.0];
    const v_p1: [4]f32 = [20.0, 20.0, 20.0, 20.0];
    const v_p2: [4]f32 = [30.0, 30.0, 30.0, 30.0];

    const s0 = parent.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;
    expect_eq(parent.writeKvToken(s0, k_tok[0..], v_p0[0..], k_cache[0..], v_cache[0..], 4), true);
    const s1 = parent.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;
    expect_eq(parent.writeKvToken(s1, k_tok[0..], v_p1[0..], k_cache[0..], v_cache[0..], 4), true);
    const s2 = parent.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;
    expect_eq(parent.writeKvToken(s2, k_tok[0..], v_p2[0..], k_cache[0..], v_cache[0..], 4), true);

    const shared_blk0 = parent.physicalBlockAt(0) ?? return error.MissingBlock;
    const shared_blk1 = parent.physicalBlockAt(1) ?? return error.MissingBlock;
    expect_eq(pool.allocatedBlocks(), 2);

    // Zero-copy fork: no new physical blocks allocated, refcounts become 2!
    let c_blks: [4]u32 = [0; 4];
    let child = parent.forkSequence(&pool, c_blks[0..]) ?? return error.ForkFailed;
    expect_eq(pool.allocatedBlocks(), 2);
    expect_eq(pool.refCount(shared_blk0), 2);
    expect_eq(pool.refCount(shared_blk1), 2);
    expect_eq(child.seq_len, 3);

    // Child appends token 3 into shared tail block 1 -> MUST trigger Copy-on-Write!
    const child_s3 = child.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;
    expect_eq(child_s3.did_cow, true);
    expect_eq(child_s3.logical_block, 1);
    expect_eq(child_s3.slot_in_block, 1);
    expect_eq(child_s3.physical_block != shared_blk1, true);
    expect_eq(pool.stats().cow_copies, 1);
    // Block 0 is STILL shared (refcount 2), block 1 is now exclusively parent's (refcount 1)!
    expect_eq(pool.refCount(shared_blk0), 2);
    expect_eq(pool.refCount(shared_blk1), 1);
    expect_eq(pool.refCount(child_s3.physical_block), 1);

    // Write distinct values to child's slot 3 and parent's slot 3
    const v_child3: [4]f32 = [99.0, 99.0, 99.0, 99.0];
    expect_eq(child.writeKvToken(child_s3, k_tok[0..], v_child3[0..], k_cache[0..], v_cache[0..], 4), true);

    const parent_s3 = parent.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;
    // Parent's tail block now has refcount 1 -> NO second CoW needed!
    expect_eq(parent_s3.did_cow, false);
    expect_eq(parent_s3.physical_block, shared_blk1);
    const v_parent3: [4]f32 = [40.0, 40.0, 40.0, 40.0];
    expect_eq(parent.writeKvToken(parent_s3, k_tok[0..], v_parent3[0..], k_cache[0..], v_cache[0..], 4), true);

    // Verify child's CoW block preserved prefix token 2 (30.0) AND has child's token 3 (99.0),
    // while parent's block still has parent's token 3 (40.0)!
    const child_blk1_base: usize = (child_s3.physical_block as usize) * 8;
    const parent_blk1_base: usize = (shared_blk1 as usize) * 8;
    expect_eq(absF32(v_cache[child_blk1_base + 0] - 30.0) < 0.0001, true);
    expect_eq(absF32(v_cache[child_blk1_base + 4] - 99.0) < 0.0001, true);
    expect_eq(absF32(v_cache[parent_blk1_base + 4] - 40.0) < 0.0001, true);

    // Release both sequences -> zero leak
    child.releaseAll(&pool);
    expect_eq(pool.refCount(shared_blk0), 1);
    parent.releaseAll(&pool);
    expect_eq(pool.allocatedBlocks(), 0);
    expect_eq(pool.freeBlocks(), 6);
}

// ============================================================================
// Test 4: SIMD PagedAttention Exact Equivalence Over Fragmented Physical Blocks
// ============================================================================

fn testPagedAttentionExactEquivalence()! {
    // 5 physical blocks, block_size = 2, head_dim = 8 (exercises 8-lane SIMD @dot & @fma)
    const block_size: usize = 2;
    const head_dim: usize = 8;
    let refs: [5]u32 = [0; 5];
    let k_cache: [80]f32 = [0.0; 80]; // 5 * 2 * 8
    let v_cache: [80]f32 = [0.0; 80];
    let pool = PagedKvBlockPool.init(refs[0..]) ?? return error.InitFailed;

    // Deliberately allocate and free dummy blocks so sequence gets fragmented physical blocks [1, 3, 0]
    const dummy0 = pool.allocBlock() ?? return error.AllocFailed; // 0
    const seq_b0_pre = pool.allocBlock() ?? return error.AllocFailed; // 1
    const dummy2 = pool.allocBlock() ?? return error.AllocFailed; // 2
    * = pool.releaseBlock(seq_b0_pre); // free 1

    let blks: [4]u32 = [0; 4];
    let seq = SeqBlockTable.init(blks[0..], block_size) ?? return error.InitFailed;

    const k_contig: [40]f32 = [
        0.9,  0.1,  0.0,  0.2, -0.1,  0.3,  0.0,  0.4, // tok 0
        0.1,  0.8, -0.3,  0.1,  0.5,  0.0,  0.2, -0.2, // tok 1
       -0.4,  0.2,  0.9,  0.1,  0.0, -0.3,  0.4,  0.1, // tok 2
        0.5, -0.5,  0.2,  0.7, -0.2,  0.4,  0.1,  0.3, // tok 3
        0.2,  0.3, -0.1,  0.4,  0.8, -0.2,  0.5, -0.1, // tok 4
    ];
    const v_contig: [40]f32 = [
        1.0,  2.0, -1.0,  0.5,  3.0, -2.0,  0.25, 1.5,
        0.5, -1.5,  2.0,  1.0, -0.5,  2.5,  1.0, -1.0,
       -2.0,  0.5,  1.5, -0.5,  1.0,  0.0, -1.5,  2.0,
        2.5,  1.0, -0.5,  2.0, -1.0,  1.5,  0.5,  0.0,
        0.0,  3.0,  0.5, -1.5,  2.0, -0.5,  1.0,  1.0,
    ];

    let t: usize = 0;
    while (t < 5) : (t += 1) {
        if (t == 4) {
            // Free dummy0 (block 0) right before 3rd logical block so block 2 maps to physical block 0!
            * = pool.releaseBlock(dummy0);
        }
        const slot = seq.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], head_dim) ?? return error.AppendFailed;
        const k_slice = k_contig[t * head_dim .. (t + 1) * head_dim];
        const v_slice = v_contig[t * head_dim .. (t + 1) * head_dim];
        expect_eq(seq.writeKvToken(slot, k_slice, v_slice, k_cache[0..], v_cache[0..], head_dim), true);
    }
    * = pool.releaseBlock(dummy2);

    // Confirm physical blocks are genuinely non-contiguous: [1, 3, 0]
    expect_eq(seq.physicalBlockAt(0) ?? 99, 1);
    expect_eq(seq.physicalBlockAt(1) ?? 99, 3);
    expect_eq(seq.physicalBlockAt(2) ?? 99, 0);

    const query: [8]f32 = [0.7, -0.2, 0.4, 0.5, -0.3, 0.6, 0.1, 0.2];
    let paged_out: [8]f32 = [0.0; 8];
    let naive_out: [8]f32 = [0.0; 8];

    expect_eq(
        pagedAttentionDecodeF32(query[0..], seq, k_cache[0..], v_cache[0..], head_dim, 0.0, paged_out[0..]),
        true,
    );
    naiveContiguousAttentionF32(query[0..], k_contig[0..], v_contig[0..], 5, head_dim, 0.0, naive_out[0..]);

    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        expect_eq(absF32(paged_out[d] - naive_out[d]) < 0.00002, true);
    }

    seq.releaseAll(&pool);
    expect_eq(pool.freeBlocks(), 5);
}

// ============================================================================
// Test 5: Divergent Beam Search Decode (Shared Prefix + Diverged CoW Tail)
// ============================================================================

fn testBeamSearchDivergentDecode()! {
    let refs: [4]u32 = [0; 4];
    let k_cache: [32]f32 = [0.0; 32]; // 4 blocks * block_size 2 * head_dim 4
    let v_cache: [32]f32 = [0.0; 32];
    let pool = PagedKvBlockPool.init(refs[0..]) ?? return error.InitFailed;

    let b1_ids: [2]u32 = [0; 2];
    let beam1 = SeqBlockTable.init(b1_ids[0..], 2) ?? return error.InitFailed;
    const k_prefix: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v_prefix: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const s0 = beam1.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;
    expect_eq(beam1.writeKvToken(s0, k_prefix[0..], v_prefix[0..], k_cache[0..], v_cache[0..], 4), true);

    // Fork beam2 from beam1
    let b2_ids: [2]u32 = [0; 2];
    let beam2 = beam1.forkSequence(&pool, b2_ids[0..]) ?? return error.ForkFailed;

    // Both beams attend strongly to their own diverged 2nd token
    const k_tail: [4]f32 = [0.0, 10.0, 0.0, 0.0];
    const v_b1: [4]f32 = [0.0, 100.0, 0.0, 0.0];
    const v_b2: [4]f32 = [0.0, -100.0, 0.0, 0.0];

    const s1_b2 = beam2.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;
    expect_eq(s1_b2.did_cow, true);
    expect_eq(beam2.writeKvToken(s1_b2, k_tail[0..], v_b2[0..], k_cache[0..], v_cache[0..], 4), true);

    const s1_b1 = beam1.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;
    expect_eq(s1_b1.did_cow, false);
    expect_eq(beam1.writeKvToken(s1_b1, k_tail[0..], v_b1[0..], k_cache[0..], v_cache[0..], 4), true);

    const q_tail: [4]f32 = [0.0, 5.0, 0.0, 0.0];
    let out1: [4]f32 = [0.0; 4];
    let out2: [4]f32 = [0.0; 4];
    expect_eq(pagedAttentionDecodeF32(q_tail[0..], beam1, k_cache[0..], v_cache[0..], 4, 1.0, out1[0..]), true);
    expect_eq(pagedAttentionDecodeF32(q_tail[0..], beam2, k_cache[0..], v_cache[0..], 4, 1.0, out2[0..]), true);

    expect_eq(out1[1] > 99.0, true);
    expect_eq(out2[1] < -99.0, true);

    beam1.releaseAll(&pool);
    beam2.releaseAll(&pool);
    expect_eq(pool.freeBlocks(), 4);
}

// ============================================================================
// Test 6: Defensive Guards (Invalid Init, Pool Exhaustion on CoW, NaN Query)
// ============================================================================

fn testDefensiveGuards()! {
    let refs: [1]u32 = [0];
    let empty_u32: []u32 = refs[0..0];
    expect_eq(PagedKvBlockPool.init(empty_u32) == null, true);
    expect_eq(SeqBlockTable.init(empty_u32, 4) == null, true);
    expect_eq(SeqBlockTable.init(refs[0..], 0) == null, true);

    // 1-block pool: fork succeeds, then CoW append must fail cleanly without corrupting refcount!
    let pool = PagedKvBlockPool.init(refs[0..]) ?? return error.InitFailed;
    let k_cache: [8]f32 = [0.0; 8];
    let v_cache: [8]f32 = [0.0; 8];
    let p_blks: [2]u32 = [0; 2];
    let c_blks: [2]u32 = [0; 2];
    let parent = SeqBlockTable.init(p_blks[0..], 2) ?? return error.InitFailed;
    * = parent.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) ?? return error.AppendFailed;

    let child = parent.forkSequence(&pool, c_blks[0..]) ?? return error.ForkFailed;
    expect_eq(pool.refCount(0), 2);
    // Pool has 0 free blocks -> CoW must return null and preserve refCount == 2 & seq_len == 1!
    expect_eq(child.appendTokenSlot(&pool, k_cache[0..], v_cache[0..], 4) == null, true);
    expect_eq(pool.refCount(0), 2);
    expect_eq(child.seq_len, 1);

    // Undersized child_block_ids -> fork returns null
    expect_eq(parent.forkSequence(&pool, empty_u32) == null, true);

    // pagedAttentionDecodeF32 guards: NaN query, dim mismatch, empty sequence
    const nan_f32: f32 = std.math.nan(f32);
    const nan_q: [4]f32 = [nan_f32, 0.0, 0.0, 0.0];
    let out: [4]f32 = [0.0; 4];
    expect_eq(pagedAttentionDecodeF32(nan_q[0..], parent, k_cache[0..], v_cache[0..], 4, 0.0, out[0..]), false);
    expect_eq(pagedAttentionDecodeF32(nan_q[0..2], parent, k_cache[0..], v_cache[0..], 4, 0.0, out[0..]), false);

    child.releaseAll(&pool);
    parent.releaseAll(&pool);
    expect_eq(pagedAttentionDecodeF32(out[0..], parent, k_cache[0..], v_cache[0..], 4, 0.0, out[0..]), false);
    expect_eq(pool.freeBlocks(), 1);
}

fn main() {
    test("1. attention.paged_kv: block pool refcounting, watermark & exhaustion", () => {
        testBlockPoolRefcountingAndStats()?;
    });
    test("2. attention.paged_kv: cross-block append & zero-leak releaseAll", () => {
        testSeqBlockTableCrossBlockAndZeroLeak()?;
    });
    test("3. attention.paged_kv: zero-copy forkSequence & CoW isolation", () => {
        testZeroCopyForkAndCowIsolation()?;
    });
    test("4. attention.paged_kv: SIMD PagedAttention exact equivalence over fragmented blocks", () => {
        testPagedAttentionExactEquivalence()?;
    });
    test("5. attention.paged_kv: divergent Beam Search decode with CoW tail", () => {
        testBeamSearchDivergentDecode()?;
    });
    test("6. attention.paged_kv: defensive guards (CoW exhaustion, NaN, empty)", () => {
        testDefensiveGuards()?;
    });
}
