// Origin: Pure-CP Stdlib Test Suite (lib/attention/tokenMerge.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Bipartite Soft Matching Token Merging (ToMe):
//   1. Bipartite Soft Matching with CLS prefix protection, Top-r merging, size conservation & telemetry
//   2. Non-uniform prior weights & many-to-one weighted average fusion exact numerical verification
//   3. `min_similarity` threshold guard preventing dissimilar semantic tokens from forced merging
//   4. Proportional Attention `computeProportionalLogBias` (`log(size)`) accuracy & invalid weight guard
//   5. Zero-alloc `unmergeTokensF32` inverse projection & out-of-bounds merge_map rejection
//   6. Boundary guards (r=0 identity, NaN/Inf/dim/buffer_too_small) & `cap.attention` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    TOME_MAX_TOKENS,
    TOME_MAX_DIM,
    TOME_PERMILLE_SCALE,
    TokenMergeStatus,
    TokenMergeConfig,
    TokenMergeReport,
    mergeTokensF32,
    computeProportionalLogBias,
    unmergeTokensF32,
} from "cap:attention";

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

test("TokenMerge: bipartite soft matching, prefix protection & size conservation") {
    // 5 tokens, dim=4:
    //   0: CLS [0, 0, 0, 1] (protected)
    //   1 (A0): [1, 0, 0, 0]   2 (B0): [0.99, 0.14, 0, 0]  (near duplicate)
    //   3 (A1): [0, 1, 0, 0]   4 (B1): [0.10, 0.995, 0, 0] (near duplicate)
    const toks: [20]f32 = [
        0.0, 0.0, 0.0, 1.0,
        1.0, 0.0, 0.0, 0.0,
        0.99, 0.14, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.10, 0.995, 0.0, 0.0,
    ];
    const empty_sizes: [0]f32 = [];
    let out_toks: [20]f32 = [0.0; 20];
    let out_sizes: [5]f32 = [0.0; 5];
    let mmap: [5]usize = [0; 5];
    let ot_s: []f32 = out_toks[0..];
    let os_s: []f32 = out_sizes[0..];
    let mm_s: []usize = mmap[0..];

    const rep: TokenMergeReport = mergeTokensF32(
        toks[0..],
        empty_sizes[0..],
        5,
        4,
        TokenMergeConfig.make(2, 1, 0.90),
        *ot_s,
        *os_s,
        *mm_s,
    );

    expect_eq(rep.status, TokenMergeStatus.ok);
    expect_eq(rep.input_tokens, @as(usize, 5));
    expect_eq(rep.output_tokens, @as(usize, 3)); // 1 CLS + 2 merged B tokens
    expect_eq(rep.merged_pairs, @as(usize, 2));
    expect_eq(rep.protected_tokens, @as(usize, 1));
    expect_eq(rep.compression_permille, @as(u32, 400)); // (5-3)*1000/5 = 400
    expect_true(rep.avg_merge_sim > 0.98);
    expect_true(rep.min_merge_sim > 0.98);

    // Protected CLS token at output index 0 is untouched
    expect_eq(mmap[0], @as(usize, 0));
    expect_true(absF32(out_toks[3] - 1.0) < 0.00001);
    expect_true(absF32(out_sizes[0] - 1.0) < 0.00001);

    // Merged pairs share the same output slot and conserve total size = 5.0
    expect_eq(mmap[1], mmap[2]);
    expect_eq(mmap[3], mmap[4]);
    const total_size = out_sizes[0] + out_sizes[1] + out_sizes[2];
    expect_true(absF32(total_size - 5.0) < 0.0001);
}

test("TokenMerge: weighted average & many-to-one fusion accuracy") {
    // 4 tokens, dim=2, protect_prefix=0:
    //   A0 (idx 0, w=3.0): [1.0, 0.0]
    //   B0 (idx 1, w=1.0): [1.0, 0.2]
    //   A1 (idx 2, w=2.0): [1.0, 0.0]
    //   B1 (idx 3, w=1.0): [0.0, 1.0] (orthogonal, neither A matches B1)
    // Both A0 and A1 match B0 -> many-to-one merge into B0 with total weight 3+1+2 = 6.0!
    const toks: [8]f32 = [
        1.0, 0.0,
        1.0, 0.2,
        1.0, 0.0,
        0.0, 1.0,
    ];
    const sizes: [4]f32 = [3.0, 1.0, 2.0, 1.0];
    let out_toks: [8]f32 = [0.0; 8];
    let out_sizes: [4]f32 = [0.0; 4];
    let mmap: [4]usize = [0; 4];
    let ot_s: []f32 = out_toks[0..];
    let os_s: []f32 = out_sizes[0..];
    let mm_s: []usize = mmap[0..];

    const rep = mergeTokensF32(
        toks[0..],
        sizes[0..],
        4,
        2,
        TokenMergeConfig.make(2, 0, 0.90),
        *ot_s,
        *os_s,
        *mm_s,
    );

    expect_eq(rep.status, TokenMergeStatus.ok);
    expect_eq(rep.output_tokens, @as(usize, 2)); // B0 (merged) + B1 (unmerged)
    expect_eq(rep.merged_pairs, @as(usize, 2));

    const b0_slot = mmap[1];
    expect_eq(mmap[0], b0_slot);
    expect_eq(mmap[2], b0_slot);
    expect_true(absF32(out_sizes[b0_slot] - 6.0) < 0.0001);
    // Weighted average y = (3*0 + 1*0.2 + 2*0) / 6 = 0.2 / 6 = 0.033333...
    expect_true(absF32(out_toks[b0_slot * 2] - 1.0) < 0.0001);
    expect_true(absF32(out_toks[b0_slot * 2 + 1] - (0.2 / 6.0)) < 0.0001);
}

test("TokenMerge: min_similarity threshold blocks dissimilar merges") {
    // 4 orthogonal tokens -> max cosine similarity between A and B is 0.0
    const toks: [16]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];
    const empty_sizes: [0]f32 = [];
    let out_toks: [16]f32 = [0.0; 16];
    let out_sizes: [4]f32 = [0.0; 4];
    let mmap: [4]usize = [0; 4];
    let ot_s: []f32 = out_toks[0..];
    let os_s: []f32 = out_sizes[0..];
    let mm_s: []usize = mmap[0..];

    const rep = mergeTokensF32(
        toks[0..],
        empty_sizes[0..],
        4,
        4,
        TokenMergeConfig.make(2, 0, 0.85),
        *ot_s,
        *os_s,
        *mm_s,
    );
    expect_eq(rep.status, TokenMergeStatus.ok);
    expect_eq(rep.merged_pairs, @as(usize, 0));
    expect_eq(rep.output_tokens, @as(usize, 4));
    expect_eq(rep.compression_permille, @as(u32, 0));
}

test("TokenMerge: proportional attention log(size) bias & invalid size rejection") {
    const sizes: [3]f32 = [1.0, 2.0, 3.0];
    let bias: [3]f32 = [0.0; 3];
    let b_s: []f32 = bias[0..];
    expect_true(computeProportionalLogBias(sizes[0..], *b_s));
    expect_true(absF32(bias[0]) < 0.00001);               // ln(1) = 0
    expect_true(absF32(bias[1] - 0.693147) < 0.0005);     // ln(2)
    expect_true(absF32(bias[2] - 1.098612) < 0.0005);     // ln(3)

    const bad_sizes: [2]f32 = [1.0, 0.0];
    expect_true(!computeProportionalLogBias(bad_sizes[0..], *b_s));
    const empty: [0]f32 = [];
    expect_true(!computeProportionalLogBias(empty[0..], *b_s));
}

test("TokenMerge: unmergeTokensF32 inverse projection & bounds validation") {
    const toks: [8]f32 = [
        0.0, 1.0, // CLS
        1.0, 0.0, // A0
        1.0, 0.2, // B0 (merges with A0 -> [1.0, 0.1])
        0.0, -1.0, // A1 (unmerged)
    ];
    const empty_sizes: [0]f32 = [];
    let merged: [8]f32 = [0.0; 8];
    let out_sizes: [4]f32 = [0.0; 4];
    let mmap: [4]usize = [0; 4];
    let m_s: []f32 = merged[0..];
    let os_s: []f32 = out_sizes[0..];
    let mm_s: []usize = mmap[0..];

    const rep = mergeTokensF32(
        toks[0..],
        empty_sizes[0..],
        4,
        2,
        TokenMergeConfig.make(1, 1, 0.8),
        *m_s,
        *os_s,
        *mm_s,
    );
    expect_eq(rep.output_tokens, @as(usize, 3));

    let restored: [8]f32 = [0.0; 8];
    let r_s: []f32 = restored[0..];
    expect_eq(
        unmergeTokensF32(merged[0 .. rep.output_tokens * 2], mmap[0..], 4, rep.output_tokens, 2, *r_s),
        TokenMergeStatus.ok,
    );

    // CLS restored exactly; A0 & B0 both receive fused [1.0, 0.1]; A1 restored exactly
    expect_true(absF32(restored[1] - 1.0) < 0.0001);
    expect_true(absF32(restored[2] - 1.0) < 0.0001);
    expect_true(absF32(restored[3] - 0.1) < 0.0001);
    expect_true(absF32(restored[4] - 1.0) < 0.0001);
    expect_true(absF32(restored[5] - 0.1) < 0.0001);
    expect_true(absF32(restored[7] + 1.0) < 0.0001);

    // Out-of-bounds merge_map index rejected cleanly
    let bad_map: [4]usize = [0, 1, 99, 2];
    expect_eq(
        unmergeTokensF32(merged[0 .. rep.output_tokens * 2], bad_map[0..], 4, rep.output_tokens, 2, *r_s),
        TokenMergeStatus.invalid_input,
    );
}

test("TokenMerge: boundary guards (r=0, NaN, buffer_too_small) & cap.attention facade parity") {
    const zero: f32 = 0.0;
    const nan_val: f32 = zero / zero;

    const toks: [4]f32 = [1.0, 0.0, 0.0, 1.0];
    const nan_toks: [4]f32 = [1.0, nan_val, 0.0, 1.0];
    const empty_sizes: [0]f32 = [];
    let out_toks: [4]f32 = [0.0; 4];
    let out_sizes: [2]f32 = [0.0; 2];
    let mmap: [2]usize = [0; 2];
    let ot_s: []f32 = out_toks[0..];
    let os_s: []f32 = out_sizes[0..];
    let mm_s: []usize = mmap[0..];

    // r=0 identity pass via cap.attention facade
    const id_rep = cap.attention.mergeTokensF32(
        toks[0..],
        empty_sizes[0..],
        2,
        2,
        cap.attention.TokenMergeConfig.make(0, 0, -1.0),
        *ot_s,
        *os_s,
        *mm_s,
    );
    expect_eq(id_rep.status, TokenMergeStatus.ok);
    expect_eq(id_rep.output_tokens, @as(usize, 2));
    expect_eq(mmap[0], @as(usize, 0));
    expect_eq(mmap[1], @as(usize, 1));

    // NaN input rejected
    expect_eq(
        mergeTokensF32(nan_toks[0..], empty_sizes[0..], 2, 2, TokenMergeConfig.make(1, 0, -1.0), *ot_s, *os_s, *mm_s).status,
        TokenMergeStatus.invalid_input,
    );

    // Output buffer too small rejected
    let tiny_t: [1]f32 = [0.0];
    let tiny_s: []f32 = tiny_t[0..];
    expect_eq(
        mergeTokensF32(toks[0..], empty_sizes[0..], 2, 2, TokenMergeConfig.make(0, 0, -1.0), *tiny_s, *os_s, *mm_s).status,
        TokenMergeStatus.buffer_too_small,
    );

    expect_eq(TOME_MAX_TOKENS, @as(usize, 256));
    expect_eq(TOME_MAX_DIM, @as(usize, 64));
    expect_eq(TOME_PERMILLE_SCALE, @as(u32, 1000));
}
