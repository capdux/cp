// Origin: Pure-CP Stdlib Test Suite (lib/attention/flashDecode.cp) | Created: 2026-11-22
// Production Deep Verification Suite for FlashDecoding Split-K KV-Partition Attention & LSE Merge:
//   1. Single-partition online Softmax & exact LSE scalar (max_logit + ln(sum_exp)) parity
//   2. Split-K (1, 2, 4, 7, 16 splits) exact mathematical equivalence vs full single-pass SDPA (< 1e-5)
//   3. Arbitrary non-uniform split boundaries & partition permutation invariance
//   4. Empty partition skipping (merge stats) & oversplit (num_splits > num_kv) resilience
//   5. Extreme large (+600) and small (-600) logits overflow/underflow safety (zero NaN/Inf)
//   6. Multi-head Grouped-Query Attention (GQA) batch Split-K reduction parity
//   7. Full boundary/malformed input guards & cap.attention facade verification

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    FLASH_DECODE_MAX_HEAD_DIM,
    FLASH_DECODE_MAX_SPLITS,
    FlashDecodePartial,
    flashDecodePartition,
    mergeFlashDecodePartialsWithStats,
    mergeFlashDecodePartials,
    flashDecodeSplitK,
    flashDecodeSplitKGqa,
} from "cap:attention";

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// High-precision f64 reference two-pass SDPA for single query + exact global LSE.
fn referenceSdpaF64(
    q: []const f32,
    k: []const f32,
    v: []const f32,
    num_kv: usize,
    head_dim: usize,
    scale: f32,
    out_ref: *[]f32,
): f64 {
    const eff_scale: f64 = if (scale > 0.0) (scale as f64) else (1.0 / std.math.sqrt(head_dim as f64));
    let scores: [128]f64 = [0.0; 128];
    let max_s: f64 = 0.0 - std.math.inf(f64);

    let j: usize = 0;
    while (j < num_kv) : (j += 1) {
        let dot: f64 = 0.0;
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            dot += (q[d] as f64) * (k[j * head_dim + d] as f64);
        }
        const s: f64 = dot * eff_scale;
        scores[j] = s;
        if (j == 0 || s > max_s) {
            max_s = s;
        }
    }

    let sum_e: f64 = 0.0;
    let j2: usize = 0;
    while (j2 < num_kv) : (j2 += 1) {
        sum_e += std.math.exp(scores[j2] - max_s);
    }

    let d0: usize = 0;
    while (d0 < head_dim) : (d0 += 1) {
        let acc: f64 = 0.0;
        let j3: usize = 0;
        while (j3 < num_kv) : (j3 += 1) {
            const w: f64 = std.math.exp(scores[j3] - max_s) / sum_e;
            acc += w * (v[j3 * head_dim + d0] as f64);
        }
        out_ref[d0] = acc as f32;
    }

    return max_s + @log(sum_e);
}

// ============================================================================
// Test 1: Single-Partition Online Softmax & Exact LSE Scalar
// ============================================================================
test("flashDecode: single partition matches f64 reference SDPA & exact LSE", () => {
    const head_dim: usize = 8;
    const num_kv: usize = 6;
    const q: [8]f32 = [0.5, -0.3, 0.8, 0.1, -0.6, 0.4, 0.2, -0.7];
    let k: [48]f32 = [0.0; 48];
    let v: [48]f32 = [0.0; 48];

    let j: usize = 0;
    while (j < num_kv) : (j += 1) {
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            const idx: usize = j * head_dim + d;
            k[idx] = std.math.sin(((idx + 1) as f64) * 0.37) as f32;
            v[idx] = std.math.cos(((idx + 3) as f64) * 0.29) as f32;
        }
    }

    let part = FlashDecodePartial.empty();
    expect_true(!part.isValid());
    expect_true(flashDecodePartition(q[0..], k[0..], v[0..], num_kv, head_dim, 0.0, *part));
    expect_true(part.isValid());
    expect_eq(part.head_dim, head_dim);
    expect_eq(part.num_kv, num_kv);

    let ref_out: [8]f32 = [0.0; 8];
    let ref_s: []f32 = ref_out[0..];
    const ref_lse: f64 = referenceSdpaF64(q[0..], k[0..], v[0..], num_kv, head_dim, 0.0, *ref_s);

    expect_true(absF64(part.lse - ref_lse) < 1.0e-5);
    const out_view = part.outputSlice();
    expect_eq(out_view.len, head_dim);

    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        expect_true(absF32(out_view[d] - ref_out[d]) < 1.0e-5);
    }
});

// ============================================================================
// Test 2: Split-K Equivalence Across Multiple Split Counts (1, 2, 4, 7, 16)
// ============================================================================
test("flashDecode: Split-K (1/2/4/7/16 splits) matches full single-pass SDPA within 1e-5", () => {
    const head_dim: usize = 16;
    const num_kv: usize = 48;
    let q: [16]f32 = [0.0; 16];
    let k: [768]f32 = [0.0; 768];
    let v: [768]f32 = [0.0; 768];

    let d0: usize = 0;
    while (d0 < head_dim) : (d0 += 1) {
        q[d0] = (std.math.sin(((d0 + 2) as f64) * 0.53) * 1.2) as f32;
    }
    let idx: usize = 0;
    while (idx < num_kv * head_dim) : (idx += 1) {
        k[idx] = (std.math.cos(((idx + 5) as f64) * 0.19) * 1.1) as f32;
        v[idx] = (std.math.sin(((idx + 11) as f64) * 0.13) * 2.0) as f32;
    }

    let ref_out: [16]f32 = [0.0; 16];
    let ref_s: []f32 = ref_out[0..];
    * = referenceSdpaF64(q[0..], k[0..], v[0..], num_kv, head_dim, 0.0, *ref_s);

    const split_counts: [5]usize = [1, 2, 4, 7, 16];
    let si: usize = 0;
    while (si < 5) : (si += 1) {
        let out_vec: [16]f32 = [0.0; 16];
        let out_s: []f32 = out_vec[0..];
        expect_true(flashDecodeSplitK(q[0..], k[0..], v[0..], num_kv, head_dim, split_counts[si], 0.0, *out_s));

        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            expect_true(absF32(out_vec[d] - ref_out[d]) < 1.0e-5);
        }
    }
});

// ============================================================================
// Test 3: Non-Uniform Split Boundaries & Partition Permutation Invariance
// ============================================================================
test("flashDecode: non-uniform split boundaries & arbitrary partition order invariance", () => {
    const head_dim: usize = 12;
    const num_kv: usize = 29;
    let q: [12]f32 = [0.0; 12];
    let k: [348]f32 = [0.0; 348];
    let v: [348]f32 = [0.0; 348];

    let d0: usize = 0;
    while (d0 < head_dim) : (d0 += 1) {
        q[d0] = ((d0 as f64) * 0.15 - 0.8) as f32;
    }
    let idx: usize = 0;
    while (idx < num_kv * head_dim) : (idx += 1) {
        k[idx] = std.math.sin(((idx + 7) as f64) * 0.41) as f32;
        v[idx] = std.math.cos(((idx + 2) as f64) * 0.23) as f32;
    }

    let ref_out: [12]f32 = [0.0; 12];
    let ref_s: []f32 = ref_out[0..];
    const ref_lse: f64 = referenceSdpaF64(q[0..], k[0..], v[0..], num_kv, head_dim, 0.25, *ref_s);

    // Non-uniform split lengths: [3, 11, 1, 14] (sum = 29)
    const lens: [4]usize = [3, 11, 1, 14];
    let parts: [4]FlashDecodePartial = [FlashDecodePartial.empty(); 4];
    let off: usize = 0;
    let p: usize = 0;
    while (p < 4) : (p += 1) {
        const s_e: usize = off * head_dim;
        const e_e: usize = (off + lens[p]) * head_dim;
        expect_true(flashDecodePartition(q[0..], k[s_e..e_e], v[s_e..e_e], lens[p], head_dim, 0.25, *parts[p]));
        off += lens[p];
    }

    // Forward merge
    let out_fwd: [12]f32 = [0.0; 12];
    let out_fwd_s: []f32 = out_fwd[0..];
    if (stats_fwd := mergeFlashDecodePartialsWithStats(parts[0..], head_dim, *out_fwd_s)) {
        expect_eq(stats_fwd.active_partitions, 4);
        expect_eq(stats_fwd.skipped_empty, 0);
        expect_eq(stats_fwd.total_kv, 29);
        expect_true(absF64(stats_fwd.global_lse - ref_lse) < 1.0e-5);
    } else {
        expect_true(false);
    }

    // Permuted merge order: [2, 0, 3, 1]
    const permuted: [4]FlashDecodePartial = [parts[2], parts[0], parts[3], parts[1]];
    let out_perm: [12]f32 = [0.0; 12];
    let out_perm_s: []f32 = out_perm[0..];
    expect_true(mergeFlashDecodePartials(permuted[0..], head_dim, *out_perm_s));

    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        expect_true(absF32(out_fwd[d] - ref_out[d]) < 1.0e-5);
        expect_true(absF32(out_perm[d] - ref_out[d]) < 1.0e-5);
    }
});

// ============================================================================
// Test 4: Empty Partition Skipping & Oversplit (num_splits > num_kv)
// ============================================================================
test("flashDecode: empty partitions skipped cleanly & num_splits > num_kv works", () => {
    const head_dim: usize = 4;
    const num_kv: usize = 5;
    const q: [4]f32 = [1.0, -0.5, 0.25, 0.75];
    const k: [20]f32 = [
        0.5, 0.2, -0.1, 0.4,
        -0.3, 0.8, 0.5, -0.2,
        0.9, -0.4, 0.3, 0.1,
        -0.6, -0.1, 0.7, 0.8,
        0.2, 0.6, -0.5, 0.3,
    ];
    const v: [20]f32 = [
        1.0, 2.0, 3.0, 4.0,
        -1.0, 0.5, 1.5, -2.0,
        0.25, -0.75, 2.5, 1.25,
        3.0, 1.0, -1.0, 0.0,
        -0.5, 1.5, 0.5, 2.5,
    ];

    let ref_out: [4]f32 = [0.0; 4];
    let ref_s: []f32 = ref_out[0..];
    * = referenceSdpaF64(q[0..], k[0..], v[0..], num_kv, head_dim, 0.0, *ref_s);

    // Interleave empty partitions at indices 0, 2, 4
    let parts: [5]FlashDecodePartial = [FlashDecodePartial.empty(); 5];
    expect_true(flashDecodePartition(q[0..], k[0..0], v[0..0], 0, head_dim, 0.0, *parts[0]));
    expect_true(!parts[0].isValid());
    expect_true(flashDecodePartition(q[0..], k[0..8], v[0..8], 2, head_dim, 0.0, *parts[1]));
    expect_true(flashDecodePartition(q[0..], k[8..8], v[8..8], 0, head_dim, 0.0, *parts[2]));
    expect_true(flashDecodePartition(q[0..], k[8..20], v[8..20], 3, head_dim, 0.0, *parts[3]));
    expect_true(flashDecodePartition(q[0..], k[20..20], v[20..20], 0, head_dim, 0.0, *parts[4]));

    let out_merged: [4]f32 = [0.0; 4];
    let out_merged_s: []f32 = out_merged[0..];
    if (stats := mergeFlashDecodePartialsWithStats(parts[0..], head_dim, *out_merged_s)) {
        expect_eq(stats.active_partitions, 2);
        expect_eq(stats.skipped_empty, 3);
        expect_eq(stats.total_kv, 5);
    } else {
        expect_true(false);
    }

    // End-to-end oversplit: num_splits = 12 > num_kv = 5
    let out_over: [4]f32 = [0.0; 4];
    let out_over_s: []f32 = out_over[0..];
    expect_true(flashDecodeSplitK(q[0..], k[0..], v[0..], num_kv, head_dim, 12, 0.0, *out_over_s));

    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        expect_true(absF32(out_merged[d] - ref_out[d]) < 1.0e-5);
        expect_true(absF32(out_over[d] - ref_out[d]) < 1.0e-5);
    }
});

// ============================================================================
// Test 5: Extreme Large (+600) & Extreme Small (-600) Logits Overflow/NaN Guard
// ============================================================================
test("flashDecode: extreme +600 and -600 logits remain finite and accurate without NaN", () => {
    const head_dim: usize = 4;
    const num_kv: usize = 4;
    const q: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    // Large logits: with scale=1.0, dot products are [600.0, 601.0, 600.0, 601.0]
    // Naive exp(600) overflows f32/f64; softmax weights must be exact [1, e, 1, e] / (2 + 2e)
    const k_hi: [16]f32 = [
        600.0, 0.0, 0.0, 0.0,
        601.0, 0.0, 0.0, 0.0,
        600.0, 0.0, 0.0, 0.0,
        601.0, 0.0, 0.0, 0.0,
    ];
    const v_all: [16]f32 = [
        10.0, 0.0, -5.0, 2.0,
        20.0, 4.0, 5.0, -2.0,
        10.0, 0.0, -5.0, 2.0,
        20.0, 4.0, 5.0, -2.0,
    ];

    let out_hi: [4]f32 = [0.0; 4];
    let out_hi_s: []f32 = out_hi[0..];
    expect_true(flashDecodeSplitK(q[0..], k_hi[0..], v_all[0..], num_kv, head_dim, 2, 1.0, *out_hi_s));

    // Small logits: [-601.0, -600.0, -601.0, -600.0] -> exact same relative weights!
    const k_lo: [16]f32 = [
        -601.0, 0.0, 0.0, 0.0,
        -600.0, 0.0, 0.0, 0.0,
        -601.0, 0.0, 0.0, 0.0,
        -600.0, 0.0, 0.0, 0.0,
    ];
    let out_lo: [4]f32 = [0.0; 4];
    let out_lo_s: []f32 = out_lo[0..];
    expect_true(flashDecodeSplitK(q[0..], k_lo[0..], v_all[0..], num_kv, head_dim, 3, 1.0, *out_lo_s));

    const w_small: f64 = 1.0 / (1.0 + std.math.e);
    const w_big: f64 = std.math.e / (1.0 + std.math.e);
    const exp0: f32 = (w_small * 10.0 + w_big * 20.0) as f32;
    const exp1: f32 = (w_small * 0.0 + w_big * 4.0) as f32;
    const exp2: f32 = (w_small * (-5.0) + w_big * 5.0) as f32;
    const exp3: f32 = (w_small * 2.0 + w_big * (-2.0)) as f32;
    const expected: [4]f32 = [exp0, exp1, exp2, exp3];

    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        expect_true(isFiniteF32(out_hi[d]));
        expect_true(isFiniteF32(out_lo[d]));
        expect_true(absF32(out_hi[d] - expected[d]) < 1.0e-4);
        expect_true(absF32(out_lo[d] - expected[d]) < 1.0e-4);
    }
});

// ============================================================================
// Test 6: Multi-Head Grouped-Query Attention (GQA) Batch Split-K Reduction
// ============================================================================
test("flashDecode: flashDecodeSplitKGqa matches per-head reference SDPA for 6Q/2KV heads", () => {
    const num_q_heads: usize = 6;
    const num_kv_heads: usize = 2;
    const num_kv: usize = 24;
    const head_dim: usize = 8;
    const num_splits: usize = 5;

    let q_heads: [48]f32 = [0.0; 48];       // 6 * 8
    let k_heads: [384]f32 = [0.0; 384];     // 2 * 24 * 8
    let v_heads: [384]f32 = [0.0; 384];     // 2 * 24 * 8

    let qi: usize = 0;
    while (qi < 48) : (qi += 1) {
        q_heads[qi] = std.math.sin(((qi + 1) as f64) * 0.47) as f32;
    }
    let ki: usize = 0;
    while (ki < 384) : (ki += 1) {
        k_heads[ki] = std.math.cos(((ki + 3) as f64) * 0.21) as f32;
        v_heads[ki] = std.math.sin(((ki + 9) as f64) * 0.17) as f32;
    }

    let out_heads: [48]f32 = [0.0; 48];
    let out_s: []f32 = out_heads[0..];
    expect_true(flashDecodeSplitKGqa(
        q_heads[0..],
        k_heads[0..],
        v_heads[0..],
        num_q_heads,
        num_kv_heads,
        num_kv,
        head_dim,
        num_splits,
        0.0,
        *out_s,
    ));

    // Verify each query head against its mapped KV head (group_size = 3)
    const kv_stride: usize = num_kv * head_dim;
    let qh: usize = 0;
    while (qh < num_q_heads) : (qh += 1) {
        const kvh: usize = qh / 3;
        let ref_h: [8]f32 = [0.0; 8];
        let ref_hs: []f32 = ref_h[0..];
        * = referenceSdpaF64(
            q_heads[qh * head_dim .. (qh + 1) * head_dim],
            k_heads[kvh * kv_stride .. (kvh + 1) * kv_stride],
            v_heads[kvh * kv_stride .. (kvh + 1) * kv_stride],
            num_kv,
            head_dim,
            0.0,
            *ref_hs,
        );

        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            expect_true(absF32(out_heads[qh * head_dim + d] - ref_h[d]) < 1.0e-5);
        }
    }
});

// ============================================================================
// Test 7: Full Boundary Guards, Malformed Inputs & cap.attention Facade
// ============================================================================
test("flashDecode: boundary guards reject invalid dims, splits, buffers, NaN/Inf & facade works", () => {
    const q: [4]f32 = [0.5, 0.5, 0.5, 0.5];
    const k: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0];
    const v: [8]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];
    let out: [4]f32 = [0.0; 4];
    let out_s: []f32 = out[0..];
    let part = FlashDecodePartial.empty();

    // head_dim == 0 & head_dim > FLASH_DECODE_MAX_HEAD_DIM
    expect_true(!flashDecodePartition(q[0..], k[0..], v[0..], 2, 0, 0.0, *part));
    expect_true(!flashDecodePartition(q[0..], k[0..], v[0..], 2, FLASH_DECODE_MAX_HEAD_DIM + 1, 0.0, *part));
    expect_true(!flashDecodeSplitK(q[0..], k[0..], v[0..], 2, 0, 2, 0.0, *out_s));
    expect_true(!flashDecodeSplitK(q[0..], k[0..], v[0..], 2, FLASH_DECODE_MAX_HEAD_DIM + 1, 2, 0.0, *out_s));

    // num_kv == 0, num_splits == 0, num_splits > FLASH_DECODE_MAX_SPLITS
    expect_true(!flashDecodeSplitK(q[0..], k[0..], v[0..], 0, 4, 2, 0.0, *out_s));
    expect_true(!flashDecodeSplitK(q[0..], k[0..], v[0..], 2, 4, 0, 0.0, *out_s));
    expect_true(!flashDecodeSplitK(q[0..], k[0..], v[0..], 2, 4, FLASH_DECODE_MAX_SPLITS + 1, 0.0, *out_s));

    // Undersized buffers
    let short_out: [2]f32 = [0.0; 2];
    let short_out_s: []f32 = short_out[0..];
    expect_true(!flashDecodeSplitK(q[0..2], k[0..], v[0..], 2, 4, 2, 0.0, *out_s));
    expect_true(!flashDecodeSplitK(q[0..], k[0..4], v[0..], 2, 4, 2, 0.0, *out_s));
    expect_true(!flashDecodeSplitK(q[0..], k[0..], v[0..4], 2, 4, 2, 0.0, *out_s));
    expect_true(!flashDecodeSplitK(q[0..], k[0..], v[0..], 2, 4, 2, 0.0, *short_out_s));

    // NaN / Inf inputs
    const nan_val: f32 = std.math.nan(f32);
    const inf_val: f32 = std.math.inf(f32);
    const q_nan: [4]f32 = [nan_val, 0.0, 0.0, 0.0];
    const k_inf: [8]f32 = [inf_val, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0];
    expect_true(!flashDecodeSplitK(q_nan[0..], k[0..], v[0..], 2, 4, 2, 0.0, *out_s));
    expect_true(!flashDecodeSplitK(q[0..], k_inf[0..], v[0..], 2, 4, 2, 0.0, *out_s));
    expect_true(!flashDecodeSplitK(q[0..], k[0..], v[0..], 2, 4, 2, nan_val, *out_s));

    // All-empty partials merge fails; mismatched head_dim merge fails
    const all_empty: [2]FlashDecodePartial = [FlashDecodePartial.empty(), FlashDecodePartial.empty()];
    expect_true(!mergeFlashDecodePartials(all_empty[0..], 4, *out_s));
    expect_true(flashDecodePartition(q[0..], k[0..], v[0..], 2, 4, 0.0, *part));
    const single_arr: [1]FlashDecodePartial = [part];
    expect_true(!mergeFlashDecodePartials(single_arr[0..], 2, *out_s));

    // GQA indivisible heads rejected
    expect_true(!flashDecodeSplitKGqa(q[0..], k[0..], v[0..], 5, 2, 2, 4, 2, 0.0, *out_s));

    // cap.attention facade sanity check
    expect_true(cap.attention.flashDecodeSplitK(q[0..], k[0..], v[0..], 2, 4, 2, 0.0, *out_s));
    expect_true(absF32(out[0] - 3.0) < 1.0e-5);
    expect_true(absF32(out[3] - 6.0) < 1.0e-5);
});
