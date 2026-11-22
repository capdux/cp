// Origin: Pure-CP Stdlib Test Suite (lib/attention/kv_quant.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation online per-token INT8 KV-Cache
// quantization, scale-hoisted query-key dot product, and O(1)-extra-memory Online Softmax
// fused single-query attention: error bounds, zero/outlier robustness, cosine similarity > 0.99
// vs FP32 reference, and invalid dimension/NaN guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KvQuantParams,
    quantizeTokenKvI8,
    dequantizeTokenKvI8,
    dotQueryQuantKeyI8,
    quantizedAttentionSingleQuery,
} from "cap:attention";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn cosineSimilarityF32(a: []const f32, b: []const f32): f64 {
    let dot: f64 = 0.0;
    let na: f64 = 0.0;
    let nb: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const av: f64 = a[i] as f64;
        const bv: f64 = b[i] as f64;
        dot += av * bv;
        na += av * av;
        nb += bv * bv;
    }
    if (na == 0.0 || nb == 0.0) return 0.0;
    return dot / (@sqrt(na) * @sqrt(nb));
}

fn testPerTokenQuantDequantErrorBoundsAndMetadata()! {
    const src: [8]f32 = [-3.25, -1.5, -0.4, 0.0, 0.25, 1.1, 2.75, 4.0];
    let q_buf: [8]i8 = undefined;
    let dst_buf: [8]f32 = undefined;
    let q_s: []i8 = q_buf[0..];
    let dst_s: []f32 = dst_buf[0..];

    const params_opt = cap.attention.quantizeTokenKvI8(src[0..], q_s);
    expect_eq(params_opt != null, true);
    if (p := params_opt) {
        expect_eq(p.zero_point, 0.0);
        expect_eq(absF64((p.min_val as f64) - (-3.25)) < 0.0001, true);
        expect_eq(absF64((p.max_val as f64) - 4.0) < 0.0001, true);
        expect_eq(absF64((p.scale as f64) - (4.0 / 127.0)) < 0.0001, true);

        expect_eq(dequantizeTokenKvI8(q_buf[0..], p, dst_s), true);
        const half_step: f64 = (p.scale as f64) * 0.5 + 0.0001;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            const err: f64 = absF64((src[i] as f64) - (dst_buf[i] as f64));
            expect_eq(err <= half_step, true);
        }
        // Exact 0.0 preservation.
        expect_eq(dst_buf[3], 0.0);
    }
}

fn testZeroVectorAndOutlierTokenRobustness()! {
    // 1. All-zero token vector -> scale == 0.0, clean 0.0 dequantization (no NaN).
    const z_vec: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    let z_q: [4]i8 = undefined;
    let z_dst: [4]f32 = undefined;
    let z_q_s: []i8 = z_q[0..];
    let z_dst_s: []f32 = z_dst[0..];

    if (zp := quantizeTokenKvI8(z_vec[0..], z_q_s)) {
        expect_eq(zp.scale, 0.0);
        expect_eq(zp.min_val, 0.0);
        expect_eq(zp.max_val, 0.0);
        expect_eq(dequantizeTokenKvI8(z_q[0..], zp, z_dst_s), true);
        let i: usize = 0;
        while (i < 4) : (i += 1) {
            expect_eq(z_dst[i], 0.0);
        }
    } else {
        expect_eq(true, false);
    }

    // 2. Per-token outlier isolation: token 0 has 1000.0 outlier, token 1 has small [-0.5, 0.5]
    //    values -> token 1 retains independent ~0.004 scale precision.
    const outlier_tok: [4]f32 = [1000.0, -250.0, 12.5, 0.0];
    const normal_tok: [4]f32 = [-0.5, -0.125, 0.25, 0.5];
    let q0: [4]i8 = undefined;
    let q1: [4]i8 = undefined;
    let d1: [4]f32 = undefined;
    let q0_s: []i8 = q0[0..];
    let q1_s: []i8 = q1[0..];
    let d1_s: []f32 = d1[0..];

    const p0_opt = quantizeTokenKvI8(outlier_tok[0..], q0_s);
    const p1_opt = quantizeTokenKvI8(normal_tok[0..], q1_s);
    expect_eq(p0_opt != null && p1_opt != null, true);
    if (p0 := p0_opt) {
        if (p1 := p1_opt) {
            expect_eq(p0.scale > 7.0, true);
            expect_eq(p1.scale < 0.005, true);
            expect_eq(dequantizeTokenKvI8(q1[0..], p1, d1_s), true);
            let j: usize = 0;
            while (j < 4) : (j += 1) {
                expect_eq(absF64((normal_tok[j] as f64) - (d1[j] as f64)) < 0.003, true);
            }
        }
    }
}

fn testScaleHoistedDotProductPrecision()! {
    const query: [8]f32 = [0.7, -1.2, 0.5, 2.1, -0.3, 0.9, -1.5, 0.4];
    const key_f32: [8]f32 = [-0.8, 1.4, 0.6, -1.9, 0.5, 1.2, -0.7, 2.5];

    let k_q: [8]i8 = undefined;
    let k_deq: [8]f32 = undefined;
    let k_q_s: []i8 = k_q[0..];
    let k_deq_s: []f32 = k_deq[0..];

    if (kp := quantizeTokenKvI8(key_f32[0..], k_q_s)) {
        expect_eq(dequantizeTokenKvI8(k_q[0..], kp, k_deq_s), true);

        let exact_dot: f64 = 0.0;
        let deq_dot: f64 = 0.0;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            exact_dot += (query[i] as f64) * (key_f32[i] as f64);
            deq_dot += (query[i] as f64) * (k_deq[i] as f64);
        }

        if (fused_dot := cap.attention.dotQueryQuantKeyI8(query[0..], k_q[0..], kp)) {
            // Fused scale-hoisted dot matches dequantized dot within float rounding.
            expect_eq(absF64(fused_dot - deq_dot) < 0.0001, true);
            // And closely matches original unquantized FP32 dot product.
            expect_eq(absF64(fused_dot - exact_dot) < 0.05, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Zero-scale key fast path returns exact 0.0.
    const zero_k: [4]i8 = [0, 0, 0, 0];
    if (zd := dotQueryQuantKeyI8(query[0..4], zero_k[0..], KvQuantParams{})) {
        expect_eq(zd, 0.0);
    } else {
        expect_eq(true, false);
    }
}

fn testQuantizedAttentionVsFp32BaselineCosineSimilarity()! {
    const seq_len: usize = 8;
    const head_dim: usize = 8;
    const query: [8]f32 = [0.9, -0.5, 1.2, 0.3, -0.8, 0.6, 0.4, -1.1];

    // 8 diverse KV tokens with varying magnitudes to exercise both Online Softmax branches
    // (score > max_score rescaling and score <= max_score accumulation).
    const keys_f32: [64]f32 = [
        -0.4,  0.2, -0.7,  0.5,  0.3, -0.2,  0.1,  0.6,
         0.5, -0.3,  0.8,  0.1, -0.4,  0.5,  0.2, -0.6,
         0.1,  0.1, -0.2,  0.4,  0.2, -0.5,  0.3,  0.1,
         1.8, -1.0,  2.4,  0.6, -1.6,  1.2,  0.8, -2.2, // high-affinity token triggers rescaling
        -1.2,  0.8, -1.5, -0.3,  1.0, -0.9, -0.4,  1.3,
         0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0, // all-zero token
         0.7, -0.4,  0.9,  0.2, -0.6,  0.5,  0.3, -0.8,
        -0.3,  0.5, -0.1,  0.7, -0.2,  0.4, -0.6,  0.2,
    ];
    const vals_f32: [64]f32 = [
         1.0, -0.5,  0.2,  0.8, -1.1,  0.4,  0.6, -0.3,
        -0.6,  1.2,  0.9, -0.4,  0.7, -0.8,  0.5,  1.1,
         0.3,  0.4, -1.2,  0.6,  0.2,  1.0, -0.7,  0.5,
         2.5, -1.8,  3.1,  0.9, -2.2,  1.7,  1.4, -0.9,
        -1.5,  0.7, -0.8,  2.1,  1.3, -1.9,  0.2,  0.6,
         0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,
         1.1, -0.9,  1.4,  0.5, -0.7,  0.8,  0.3, -0.4,
         0.4,  0.6, -0.5,  0.9,  0.1, -0.3,  1.2, -0.8,
    ];

    // 1. Quantize all K and V tokens online into INT8 KV-Cache.
    let keys_q: [64]i8 = undefined;
    let vals_q: [64]i8 = undefined;
    let k_params: [8]KvQuantParams = undefined;
    let v_params: [8]KvQuantParams = undefined;

    let t: usize = 0;
    while (t < seq_len) : (t += 1) {
        const base: usize = t * head_dim;
        if (kp := quantizeTokenKvI8(keys_f32[base .. base + head_dim], keys_q[base .. base + head_dim])) {
            k_params[t] = kp;
        } else {
            expect_eq(true, false);
        }
        if (vp := quantizeTokenKvI8(vals_f32[base .. base + head_dim], vals_q[base .. base + head_dim])) {
            v_params[t] = vp;
        } else {
            expect_eq(true, false);
        }
    }

    // 2. Run fused INT8 KV-Cache single-query attention.
    let out_q: [8]f32 = undefined;
    let out_q_s: []f32 = out_q[0..];
    expect_eq(
        cap.attention.quantizedAttentionSingleQuery(
            query[0..],
            keys_q[0..],
            k_params[0..],
            vals_q[0..],
            v_params[0..],
            seq_len,
            head_dim,
            out_q_s,
        ),
        true,
    );

    // 3. Compute exact FP32 scaled dot-product attention baseline.
    let scores: [8]f64 = undefined;
    let max_s: f64 = -(std.math.inf(f64));
    const inv_sqrt_d: f64 = 1.0 / @sqrt(head_dim as f64);
    t = 0;
    while (t < seq_len) : (t += 1) {
        let dot: f64 = 0.0;
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            dot += (query[d] as f64) * (keys_f32[t * head_dim + d] as f64);
        }
        scores[t] = dot * inv_sqrt_d;
        if (scores[t] > max_s) {
            max_s = scores[t];
        }
    }
    let sum_e: f64 = 0.0;
    let weights: [8]f64 = undefined;
    t = 0;
    while (t < seq_len) : (t += 1) {
        weights[t] = std.math.exp(scores[t] - max_s);
        sum_e += weights[t];
    }
    let out_ref: [8]f32 = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        let acc: f64 = 0.0;
        t = 0;
        while (t < seq_len) : (t += 1) {
            acc += (weights[t] / sum_e) * (vals_f32[t * head_dim + d] as f64);
        }
        out_ref[d] = acc as f32;
    }

    // 4. Verify cosine similarity > 0.999 and tight per-element error bound.
    const cos_sim: f64 = cosineSimilarityF32(out_q[0..], out_ref[0..]);
    expect_eq(cos_sim > 0.999, true);

    d = 0;
    while (d < head_dim) : (d += 1) {
        expect_eq(absF64((out_q[d] as f64) - (out_ref[d] as f64)) < 0.05, true);
    }
}

fn testInvalidDimensionsAndDefensiveGuards()! {
    const ok_vec: [4]f32 = [1.0, -1.0, 0.5, 0.25];
    const nan_vec: [4]f32 = [1.0, std.math.nan(f32), 0.5, 0.25];
    const inf_vec: [4]f32 = [1.0, std.math.inf(f32), 0.5, 0.25];

    let q_buf: [4]i8 = undefined;
    let out_f: [4]f32 = undefined;
    let q_s: []i8 = q_buf[0..];
    let out_s: []f32 = out_f[0..];

    // quantizeTokenKvI8 guards: empty, short out_q, NaN, Inf.
    expect_eq(quantizeTokenKvI8(ok_vec[0..0], q_s) == null, true);
    expect_eq(quantizeTokenKvI8(ok_vec[0..], q_buf[0..3]) == null, true);
    expect_eq(quantizeTokenKvI8(nan_vec[0..], q_s) == null, true);
    expect_eq(quantizeTokenKvI8(inf_vec[0..], q_s) == null, true);

    // dequantizeTokenKvI8 guards: empty, short out_vec, negative/NaN scale.
    const bad_neg_p = KvQuantParams{ scale: -0.1, zero_point: 0.0, min_val: 0.0, max_val: 0.0 };
    const bad_nan_p = KvQuantParams{ scale: std.math.nan(f32), zero_point: 0.0, min_val: 0.0, max_val: 0.0 };
    const ok_p = KvQuantParams{ scale: 0.1, zero_point: 0.0, min_val: -1.0, max_val: 1.0 };
    expect_eq(dequantizeTokenKvI8(q_buf[0..0], ok_p, out_s), false);
    expect_eq(dequantizeTokenKvI8(q_buf[0..], ok_p, out_f[0..2]), false);
    expect_eq(dequantizeTokenKvI8(q_buf[0..], bad_neg_p, out_s), false);
    expect_eq(dequantizeTokenKvI8(q_buf[0..], bad_nan_p, out_s), false);

    // dotQueryQuantKeyI8 guards: length mismatch, empty, NaN query, invalid params.
    expect_eq(dotQueryQuantKeyI8(ok_vec[0..3], q_buf[0..4], ok_p) == null, true);
    expect_eq(dotQueryQuantKeyI8(ok_vec[0..0], q_buf[0..0], ok_p) == null, true);
    expect_eq(dotQueryQuantKeyI8(nan_vec[0..], q_buf[0..], ok_p) == null, true);
    expect_eq(dotQueryQuantKeyI8(ok_vec[0..], q_buf[0..], bad_neg_p) == null, true);

    // quantizedAttentionSingleQuery guards: zero dims, truncated buffers, NaN query, bad params.
    const ps_ok: [1]KvQuantParams = [ok_p];
    const ps_bad: [1]KvQuantParams = [bad_neg_p];
    expect_eq(quantizedAttentionSingleQuery(ok_vec[0..], q_buf[0..], ps_ok[0..], q_buf[0..], ps_ok[0..], 0, 4, out_s), false);
    expect_eq(quantizedAttentionSingleQuery(ok_vec[0..], q_buf[0..], ps_ok[0..], q_buf[0..], ps_ok[0..], 1, 0, out_s), false);
    expect_eq(quantizedAttentionSingleQuery(ok_vec[0..], q_buf[0..3], ps_ok[0..], q_buf[0..], ps_ok[0..], 1, 4, out_s), false);
    expect_eq(quantizedAttentionSingleQuery(ok_vec[0..], q_buf[0..], ps_ok[0..], q_buf[0..], ps_ok[0..], 1, 4, out_f[0..2]), false);
    expect_eq(quantizedAttentionSingleQuery(nan_vec[0..], q_buf[0..], ps_ok[0..], q_buf[0..], ps_ok[0..], 1, 4, out_s), false);
    expect_eq(quantizedAttentionSingleQuery(ok_vec[0..], q_buf[0..], ps_bad[0..], q_buf[0..], ps_ok[0..], 1, 4, out_s), false);
}

fn main() {
    test("1. attention/kv_quant: per-token INT8 quant/dequant error bounds & metadata", () => {
        testPerTokenQuantDequantErrorBoundsAndMetadata()?;
    });

    test("2. attention/kv_quant: zero-vector handling & per-token outlier isolation", () => {
        testZeroVectorAndOutlierTokenRobustness()?;
    });

    test("3. attention/kv_quant: scale-hoisted query-key dot product precision", () => {
        testScaleHoistedDotProductPrecision()?;
    });

    test("4. attention/kv_quant: fused INT8 KV attention vs FP32 baseline (cosine > 0.999)", () => {
        testQuantizedAttentionVsFp32BaselineCosineSimilarity()?;
    });

    test("5. attention/kv_quant: invalid dimensions, truncated slices & NaN/Inf guards", () => {
        testInvalidDimensionsAndDefensiveGuards()?;
    });
}
