// Origin: Pure-CP Stdlib Test Suite (FlashAttention-2 Tiled Online Softmax & Sliding Window) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/flash.cp
// Covers:
//   1. Non-causal & causal flashAttentionForward vs materialized N x N Softmax (< 1e-12) for block_kv = 1, 3, 7, 32
//   2. Mistral sliding window (window_size = 3) exact masking of distant historical tokens
//   3. flashAttentionSingleQuery decode fast path matches last row of causal forward (< 1e-12)
//   4. Extreme logits (+500.0 / -500.0) overflow immunity & OnlineSoftmaxState invariants
//   5. Zero dims, buffer underflow & NaN/Inf boundary defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    OnlineSoftmaxState,
    FlashAttnConfig,
    flashAttentionForward,
    flashAttentionSingleQuery,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

/// Reference materialized N x N Softmax attention (for verification only, max seq <= 16).
fn referenceAttention(
    q: []const f64,
    k: []const f64,
    v: []const f64,
    seq: usize,
    dim: usize,
    causal: bool,
    window_size: usize,
    out_ref: *[]f64,
) {
    const scale = 1.0 / std.math.sqrt(dim as f64);
    let scores: [16]f64 = [0.0; 16];

    let qi: usize = 0;
    while (qi < seq) : (qi += 1) {
        let k_start: usize = 0;
        let k_end: usize = seq;
        if (window_size > 0) {
            k_end = qi + 1;
            k_start = if (k_end > window_size) k_end - window_size else 0;
        } else if (causal) {
            k_end = qi + 1;
        }

        let max_s: f64 = 0.0 - std.math.inf(f64);
        let kj = k_start;
        while (kj < k_end) : (kj += 1) {
            let dot: f64 = 0.0;
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                dot += q[qi * dim + d] * k[kj * dim + d];
            }
            const s = dot * scale;
            scores[kj] = s;
            if (s > max_s) max_s = s;
        }

        let sum_e: f64 = 0.0;
        let kj2 = k_start;
        while (kj2 < k_end) : (kj2 += 1) {
            scores[kj2] = std.math.exp(scores[kj2] - max_s);
            sum_e += scores[kj2];
        }

        let d2: usize = 0;
        while (d2 < dim) : (d2 += 1) {
            let acc: f64 = 0.0;
            let kj3 = k_start;
            while (kj3 < k_end) : (kj3 += 1) {
                acc += (scores[kj3] / sum_e) * v[kj3 * dim + d2];
            }
            out_ref[qi * dim + d2] = acc;
        }
    }
}

fn testFlashMatchesReferenceAcrossTileSizes()! {
    const SEQ: usize = 12;
    const DIM: usize = 8;

    let q: [96]f64 = [0.0; 96];
    let k: [96]f64 = [0.0; 96];
    let v: [96]f64 = [0.0; 96];
    let i: usize = 0;
    while (i < 96) : (i += 1) {
        const fi = i as f64;
        q[i] = std.math.sin(fi * 0.31 + 0.2) * 1.4;
        k[i] = std.math.cos(fi * 0.27 - 0.5) * 1.3;
        v[i] = std.math.sin(fi * 0.19 + 1.1) * 2.0;
    }

    let ref_nc_buf: [96]f64 = [0.0; 96];
    let ref_c_buf: [96]f64 = [0.0; 96];
    let ref_nc: []f64 = ref_nc_buf[0..];
    let ref_c: []f64 = ref_c_buf[0..];
    referenceAttention(q[0..], k[0..], v[0..], SEQ, DIM, false, 0, ref_nc);
    referenceAttention(q[0..], k[0..], v[0..], SEQ, DIM, true, 0, ref_c);

    let out_buf: [96]f64 = [0.0; 96];
    let out: []f64 = out_buf[0..];

    const tile_sizes: [4]usize = [1, 3, 7, 32];
    let t: usize = 0;
    while (t < 4) : (t += 1) {
        const blk = tile_sizes[t];

        // Non-causal
        const cfg_nc = FlashAttnConfig{ scale: 0.0, causal: false, window_size: 0, block_kv: blk };
        expect_eq(flashAttentionForward(q[0..], k[0..], v[0..], SEQ, SEQ, DIM, cfg_nc, out), true);
        let idx: usize = 0;
        while (idx < 96) : (idx += 1) {
            expect_eq(approxF64(out_buf[idx], ref_nc_buf[idx], EPS_F64), true);
        }

        // Causal
        const cfg_c = FlashAttnConfig{ scale: 0.0, causal: true, window_size: 0, block_kv: blk };
        expect_eq(flashAttentionForward(q[0..], k[0..], v[0..], SEQ, SEQ, DIM, cfg_c, out), true);
        let idx2: usize = 0;
        while (idx2 < 96) : (idx2 += 1) {
            expect_eq(approxF64(out_buf[idx2], ref_c_buf[idx2], EPS_F64), true);
        }
    }
}

fn testMistralSlidingWindowMasksDistantHistory()! {
    const SEQ: usize = 8;
    const DIM: usize = 4;

    let q: [32]f64 = [0.0; 32];
    let k: [32]f64 = [0.0; 32];
    let v: [32]f64 = [0.0; 32];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        const fi = i as f64;
        q[i] = std.math.sin(fi * 0.4) * 0.8;
        k[i] = std.math.cos(fi * 0.3) * 0.8;
        v[i] = 1.0 + 0.1 * fi;
    }

    // Plant huge value at token 0 and strong key alignment with query 5
    v[0] = 10000.0;
    k[0] = q[5 * DIM + 0] * 10.0;
    k[1] = q[5 * DIM + 1] * 10.0;
    k[2] = q[5 * DIM + 2] * 10.0;
    k[3] = q[5 * DIM + 3] * 10.0;

    let ref_win_buf: [32]f64 = [0.0; 32];
    let ref_win: []f64 = ref_win_buf[0..];
    referenceAttention(q[0..], k[0..], v[0..], SEQ, DIM, true, 3, ref_win);

    let out_buf: [32]f64 = [0.0; 32];
    let out: []f64 = out_buf[0..];
    const cfg_win = FlashAttnConfig{ scale: 0.0, causal: true, window_size: 3, block_kv: 2 };
    expect_eq(flashAttentionForward(q[0..], k[0..], v[0..], SEQ, SEQ, DIM, cfg_win, out), true);

    let idx: usize = 0;
    while (idx < 32) : (idx += 1) {
        expect_eq(approxF64(out_buf[idx], ref_win_buf[idx], EPS_F64), true);
    }
    // Query 5 attends only to [3, 4, 5], so token 0's 10000.0 MUST NOT leak into out[5*4 + 0]
    expect_eq(out_buf[5 * DIM + 0] < 10.0, true);
}

fn testSingleQueryDecodeMatchesCausalLastRow()! {
    const SEQ: usize = 10;
    const DIM: usize = 6;

    let q: [60]f64 = [0.0; 60];
    let k: [60]f64 = [0.0; 60];
    let v: [60]f64 = [0.0; 60];
    let i: usize = 0;
    while (i < 60) : (i += 1) {
        const fi = i as f64;
        q[i] = std.math.sin(fi * 0.25);
        k[i] = std.math.cos(fi * 0.35);
        v[i] = std.math.sin(fi * 0.55) * 3.0;
    }

    let full_out_buf: [60]f64 = [0.0; 60];
    let full_out: []f64 = full_out_buf[0..];
    const cfg_c = FlashAttnConfig{ scale: 0.0, causal: true, window_size: 0, block_kv: 4 };
    expect_eq(flashAttentionForward(q[0..], k[0..], v[0..], SEQ, SEQ, DIM, cfg_c, full_out), true);

    let dec_out_buf: [6]f64 = [0.0; 6];
    let dec_out: []f64 = dec_out_buf[0..];
    const last_q = q[(SEQ - 1) * DIM .. SEQ * DIM];
    expect_eq(flashAttentionSingleQuery(last_q, k[0..], v[0..], SEQ, DIM, 0.0, dec_out), true);

    let d: usize = 0;
    while (d < DIM) : (d += 1) {
        expect_eq(approxF64(dec_out_buf[d], full_out_buf[(SEQ - 1) * DIM + d], EPS_F64), true);
    }
}

fn testExtremeLogitsOverflowImmunity()! {
    // Direct OnlineSoftmaxState test with -500.0, +500.0, +499.0
    let st = OnlineSoftmaxState.init();
    const a0 = st.update(-500.0);
    const a1 = st.update(500.0);
    const a2 = st.update(499.0);
    expect_eq(approxF64(a0, 0.0, EPS_F64), true);
    expect_eq(approxF64(a1, 0.0, EPS_F64), true); // exp(-1000) underflows cleanly to 0.0
    expect_eq(approxF64(a2, 1.0, EPS_F64), true);
    expect_eq(approxF64(st.max_val, 500.0, EPS_F64), true);
    expect_eq(approxF64(st.sum_exp, 1.0 + std.math.exp(-1.0), EPS_F64), true);

    // Full forward pass producing +500.0 and -500.0 scores with scale = 1.0
    const q: [2]f64 = [1.0, 0.0];
    const k: [4]f64 = [-500.0, 0.0, 500.0, 0.0];
    const v: [4]f64 = [11.0, 22.0, 77.0, 88.0];
    let out_buf: [2]f64 = [0.0; 2];
    let out: []f64 = out_buf[0..];

    const cfg = FlashAttnConfig{ scale: 1.0, causal: false, window_size: 0, block_kv: 1 };
    expect_eq(flashAttentionForward(q[0..], k[0..], v[0..], 1, 2, 2, cfg, out), true);
    // Softmax concentrates 100% weight on key 1 (+500.0) -> exact [77.0, 88.0], no NaN/Inf!
    expect_eq(approxF64(out_buf[0], 77.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[1], 88.0, EPS_F64), true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_v = std.math.nan(f64);
    const inf_v = std.math.inf(f64);

    const ok4: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const nan4: [4]f64 = [1.0, nan_v, 3.0, 4.0];
    const inf4: [4]f64 = [1.0, inf_v, 3.0, 4.0];
    let out4_buf: [4]f64 = [0.0; 4];
    let out4: []f64 = out4_buf[0..];
    let out2: []f64 = out4_buf[0..2];

    const def_cfg = FlashAttnConfig.default();
    const nan_cfg = FlashAttnConfig{ scale: nan_v, causal: false, window_size: 0, block_kv: 32 };

    // Zero dimensions
    expect_eq(flashAttentionForward(ok4[0..], ok4[0..], ok4[0..], 0, 2, 2, def_cfg, out4), false);
    expect_eq(flashAttentionForward(ok4[0..], ok4[0..], ok4[0..], 2, 0, 2, def_cfg, out4), false);
    expect_eq(flashAttentionForward(ok4[0..], ok4[0..], ok4[0..], 2, 2, 0, def_cfg, out4), false);

    // Buffer underflow
    expect_eq(flashAttentionForward(ok4[0..2], ok4[0..], ok4[0..], 2, 2, 2, def_cfg, out4), false);
    expect_eq(flashAttentionForward(ok4[0..], ok4[0..2], ok4[0..], 2, 2, 2, def_cfg, out4), false);
    expect_eq(flashAttentionForward(ok4[0..], ok4[0..], ok4[0..], 2, 2, 2, def_cfg, out2), false);

    // NaN / Inf defenses
    expect_eq(flashAttentionForward(nan4[0..], ok4[0..], ok4[0..], 2, 2, 2, def_cfg, out4), false);
    expect_eq(flashAttentionForward(ok4[0..], nan4[0..], ok4[0..], 2, 2, 2, def_cfg, out4), false);
    expect_eq(flashAttentionForward(ok4[0..], ok4[0..], inf4[0..], 2, 2, 2, def_cfg, out4), false);
    expect_eq(flashAttentionForward(ok4[0..], ok4[0..], ok4[0..], 2, 2, 2, nan_cfg, out4), false);

    // flashAttentionSingleQuery defenses
    expect_eq(flashAttentionSingleQuery(ok4[0..2], ok4[0..], ok4[0..], 2, 0, 0.0, out2), false);
    expect_eq(flashAttentionSingleQuery(ok4[0..1], ok4[0..], ok4[0..], 2, 2, 0.0, out2), false);
    expect_eq(flashAttentionSingleQuery(ok4[0..2], ok4[0..], ok4[0..], 2, 2, 0.0, out4_buf[0..1]), false);
    expect_eq(flashAttentionSingleQuery(nan4[0..2], ok4[0..], ok4[0..], 2, 2, 0.0, out2), false);
}

fn main() {
    test("1. flash: non-causal & causal match materialized Softmax (<1e-12) for block_kv=1,3,7,32", () => {
        testFlashMatchesReferenceAcrossTileSizes()?;
    });
    test("2. flash: Mistral sliding window (window_size=3) masks distant history tokens", () => {
        testMistralSlidingWindowMasksDistantHistory()?;
    });
    test("3. flash: flashAttentionSingleQuery matches last row of causal forward (<1e-12)", () => {
        testSingleQueryDecodeMatchesCausalLastRow()?;
    });
    test("4. flash: extreme logits (+500/-500) overflow immunity & OnlineSoftmaxState", () => {
        testExtremeLogitsOverflowImmunity()?;
    });
    test("5. flash: zero dims, buffer underflow & NaN/Inf defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
