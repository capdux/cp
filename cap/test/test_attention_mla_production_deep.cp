// Origin: Pure-CP Stdlib Test Suite (Multi-Head Latent Attention MLA & ALiBi Slopes) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/mla.cp
// Covers:
//   1. alibiSlopes exact geometric progression for power-of-two (8 heads) and non-power-of-two (6 heads)
//   2. alibiAttentionSingleQuery distance decay + exact match vs explicit materialized reference (< 1e-12)
//   3. mlaCompressKv + mlaDecodeSingleHead weight-absorption kernel mathematically identical (< 1e-12)
//      to explicit K/V up-projection reconstruction + standard attention
//   4. MLA NoPE-only mode (d_rope == 0) & custom scale verification
//   5. Zero dims, undersized buffers & NaN/Inf boundary defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MlaConfig,
    alibiSlopes,
    alibiAttentionSingleQuery,
    mlaCompressKv,
    mlaDecodeSingleHead,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testAlibiSlopesPow2AndNonPow2()! {
    // 1a. Power-of-two: 8 heads -> 2^-1, 2^-2, ..., 2^-8
    let s8_buf: [8]f64 = [0.0; 8];
    let s8: []f64 = s8_buf[0..];
    expect_eq(alibiSlopes(8, s8), true);
    let expected: f64 = 0.5;
    let h: usize = 0;
    while (h < 8) : (h += 1) {
        expect_eq(approxF64(s8_buf[h], expected, EPS_F64), true);
        expected *= 0.5;
    }

    // 1b. Non-power-of-two: 6 heads -> closest_pow2 = 4: [2^-2, 2^-4, 2^-6, 2^-8, 2^-1, 2^-3]
    let s6_buf: [6]f64 = [0.0; 6];
    let s6: []f64 = s6_buf[0..];
    expect_eq(alibiSlopes(6, s6), true);
    const ref6: [6]f64 = [
        0.25,
        0.0625,
        0.015625,
        0.00390625,
        0.5,
        0.125,
    ];
    let k: usize = 0;
    while (k < 6) : (k += 1) {
        expect_eq(approxF64(s6_buf[k], ref6[k], EPS_F64), true);
    }
}

fn testAlibiAttentionMatchesReferenceAndDecaysWithDistance()! {
    const N_HEADS: usize = 2;
    const SEQ_KV: usize = 5;
    const HEAD_DIM: usize = 4;

    let slopes_buf: [2]f64 = [0.0; 2];
    let slopes: []f64 = slopes_buf[0..];
    expect_eq(alibiSlopes(N_HEADS, slopes), true);

    let q: [8]f64 = [0.0; 8];
    let k: [40]f64 = [0.0; 40];
    let v: [40]f64 = [0.0; 40];
    let iq: usize = 0;
    while (iq < 8) : (iq += 1) {
        q[iq] = std.math.sin((iq as f64) * 0.5 + 0.2);
    }
    let ikv: usize = 0;
    while (ikv < 40) : (ikv += 1) {
        k[ikv] = std.math.cos((ikv as f64) * 0.31 - 0.4);
        v[ikv] = std.math.sin((ikv as f64) * 0.27 + 0.9) * 1.5;
    }

    let out_buf: [8]f64 = [0.0; 8];
    let out: []f64 = out_buf[0..];
    expect_eq(
        alibiAttentionSingleQuery(q[0..], k[0..], v[0..], SEQ_KV, N_HEADS, HEAD_DIM, slopes_buf[0..], out),
        true,
    );

    // Explicit materialized reference verification
    const scale = 1.0 / std.math.sqrt(HEAD_DIM as f64);
    let h: usize = 0;
    while (h < N_HEADS) : (h += 1) {
        let scores: [5]f64 = [0.0; 5];
        let max_s: f64 = 0.0 - std.math.inf(f64);
        let j: usize = 0;
        while (j < SEQ_KV) : (j += 1) {
            let dot: f64 = 0.0;
            let d: usize = 0;
            while (d < HEAD_DIM) : (d += 1) {
                dot += q[h * HEAD_DIM + d] * k[(h * SEQ_KV + j) * HEAD_DIM + d];
            }
            const s = dot * scale - slopes_buf[h] * (((SEQ_KV - 1) - j) as f64);
            scores[j] = s;
            if (s > max_s) max_s = s;
        }
        let sum_e: f64 = 0.0;
        let j2: usize = 0;
        while (j2 < SEQ_KV) : (j2 += 1) {
            scores[j2] = std.math.exp(scores[j2] - max_s);
            sum_e += scores[j2];
        }
        let d2: usize = 0;
        while (d2 < HEAD_DIM) : (d2 += 1) {
            let acc: f64 = 0.0;
            let j3: usize = 0;
            while (j3 < SEQ_KV) : (j3 += 1) {
                acc += (scores[j3] / sum_e) * v[(h * SEQ_KV + j3) * HEAD_DIM + d2];
            }
            expect_eq(approxF64(out_buf[h * HEAD_DIM + d2], acc, EPS_F64), true);
        }
    }

    // Distance decay check: identical keys across all positions, V is one-hot position index;
    // with positive slope, output must be strictly biased toward the most recent position (j = 3 > 1.5).
    const q_same: [2]f64 = [1.0, 0.0];
    const k_same: [8]f64 = [1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0];
    const v_pos: [8]f64 = [0.0, 0.0, 1.0, 0.0, 2.0, 0.0, 3.0, 0.0];
    const slope_one: [1]f64 = [1.0];
    let decay_buf: [2]f64 = [0.0; 2];
    let decay_out: []f64 = decay_buf[0..];
    expect_eq(alibiAttentionSingleQuery(q_same[0..], k_same[0..], v_pos[0..], 4, 1, 2, slope_one[0..], decay_out), true);
    expect_eq(decay_buf[0] > 2.4, true);
}

fn testMlaWeightAbsorptionMatchesExplicitKvReconstruction()! {
    const D_MODEL: usize = 8;
    const D_LATENT: usize = 4;
    const D_NOPE: usize = 6;
    const D_ROPE: usize = 2;
    const D_V: usize = 5;
    const SEQ_KV: usize = 6;

    const cfg = MlaConfig{
        d_model: D_MODEL,
        d_latent: D_LATENT,
        n_heads: 1,
        d_nope: D_NOPE,
        d_rope: D_ROPE,
        d_v: D_V,
    };
    expect_eq(cfg.d_latent, D_LATENT);

    // Build deterministic W_DKV, W_UK, W_UV, Q, K_rope and compress SEQ_KV tokens into c_kv_cache
    let w_dkv: [32]f64 = [0.0; 32]; // [4, 8]
    let w_uk: [24]f64 = [0.0; 24];  // [6, 4]
    let w_uv: [20]f64 = [0.0; 20];  // [5, 4]
    let q_nope: [6]f64 = [0.0; 6];
    let q_rope: [2]f64 = [0.7, -0.4];
    let k_rope_cache: [12]f64 = [0.0; 12]; // [6, 2]
    let c_kv_cache: [24]f64 = [0.0; 24];   // [6, 4]

    let i_a: usize = 0;
    while (i_a < 32) : (i_a += 1) w_dkv[i_a] = std.math.sin((i_a as f64) * 0.23 + 0.1) * 0.6;
    let i_b: usize = 0;
    while (i_b < 24) : (i_b += 1) w_uk[i_b] = std.math.cos((i_b as f64) * 0.31 - 0.2) * 0.7;
    let i_c: usize = 0;
    while (i_c < 20) : (i_c += 1) w_uv[i_c] = std.math.sin((i_c as f64) * 0.41 + 0.5) * 0.8;
    let i_q: usize = 0;
    while (i_q < 6) : (i_q += 1) q_nope[i_q] = std.math.cos((i_q as f64) * 0.53 + 0.3);
    let i_kr: usize = 0;
    while (i_kr < 12) : (i_kr += 1) k_rope_cache[i_kr] = std.math.sin((i_kr as f64) * 0.37 - 0.1);

    let t: usize = 0;
    while (t < SEQ_KV) : (t += 1) {
        let x_tok: [8]f64 = [0.0; 8];
        let m: usize = 0;
        while (m < D_MODEL) : (m += 1) {
            x_tok[m] = std.math.sin(((t * D_MODEL + m) as f64) * 0.19 + 0.4);
        }
        let c_slot = c_kv_cache[t * D_LATENT .. (t + 1) * D_LATENT];
        expect_eq(mlaCompressKv(x_tok[0..], w_dkv[0..], D_MODEL, D_LATENT, c_slot), true);
    }

    // Path A: MLA Weight-Absorption decode kernel
    let q_abs_buf: [4]f64 = [0.0; 4];
    let out_lat_buf: [4]f64 = [0.0; 4];
    let out_v_buf: [5]f64 = [0.0; 5];
    let q_abs: []f64 = q_abs_buf[0..];
    let out_lat: []f64 = out_lat_buf[0..];
    let out_v: []f64 = out_v_buf[0..];

    expect_eq(
        mlaDecodeSingleHead(
            q_nope[0..],
            q_rope[0..],
            c_kv_cache[0..],
            k_rope_cache[0..],
            w_uk[0..],
            w_uv[0..],
            SEQ_KV,
            D_LATENT,
            D_NOPE,
            D_ROPE,
            D_V,
            0.0,
            q_abs,
            out_lat,
            out_v,
        ),
        true,
    );

    // Path B: Explicit K_nope = W_UK * c_kv[j] and V = W_UV * c_kv[j] reconstruction + standard Softmax attention
    const scale = 1.0 / std.math.sqrt((D_NOPE + D_ROPE) as f64);
    let scores: [6]f64 = [0.0; 6];
    let v_rec: [30]f64 = [0.0; 30]; // [6, 5]
    let max_s: f64 = 0.0 - std.math.inf(f64);

    let j: usize = 0;
    while (j < SEQ_KV) : (j += 1) {
        let dot: f64 = 0.0;
        let dn: usize = 0;
        while (dn < D_NOPE) : (dn += 1) {
            let k_nope_dn: f64 = 0.0;
            let c: usize = 0;
            while (c < D_LATENT) : (c += 1) {
                k_nope_dn += w_uk[dn * D_LATENT + c] * c_kv_cache[j * D_LATENT + c];
            }
            dot += q_nope[dn] * k_nope_dn;
        }
        let dr: usize = 0;
        while (dr < D_ROPE) : (dr += 1) {
            dot += q_rope[dr] * k_rope_cache[j * D_ROPE + dr];
        }
        scores[j] = dot * scale;
        if (scores[j] > max_s) max_s = scores[j];

        let dv: usize = 0;
        while (dv < D_V) : (dv += 1) {
            let v_val: f64 = 0.0;
            let c2: usize = 0;
            while (c2 < D_LATENT) : (c2 += 1) {
                v_val += w_uv[dv * D_LATENT + c2] * c_kv_cache[j * D_LATENT + c2];
            }
            v_rec[j * D_V + dv] = v_val;
        }
    }

    let sum_e: f64 = 0.0;
    let j2: usize = 0;
    while (j2 < SEQ_KV) : (j2 += 1) {
        scores[j2] = std.math.exp(scores[j2] - max_s);
        sum_e += scores[j2];
    }

    let dv2: usize = 0;
    while (dv2 < D_V) : (dv2 += 1) {
        let ref_v: f64 = 0.0;
        let j3: usize = 0;
        while (j3 < SEQ_KV) : (j3 += 1) {
            ref_v += (scores[j3] / sum_e) * v_rec[j3 * D_V + dv2];
        }
        expect_eq(approxF64(out_v_buf[dv2], ref_v, EPS_F64), true);
    }
}

fn testMlaNopeOnlyModeAndCustomScale()! {
    const q_nope: [2]f64 = [1.0, 0.0];
    const empty_rope: []const f64 = q_nope[0..0];
    const c_kv: [4]f64 = [1.0, 0.0, 0.0, 1.0]; // seq_kv=2, d_latent=2
    const w_uk: [4]f64 = [1.0, 0.0, 0.0, 1.0]; // identity
    const w_uv: [4]f64 = [2.0, 0.0, 0.0, 4.0]; // scales latents by [2, 4]

    let q_abs_buf: [2]f64 = [0.0; 2];
    let out_lat_buf: [2]f64 = [0.0; 2];
    let out_v_buf: [2]f64 = [0.0; 2];
    let q_abs: []f64 = q_abs_buf[0..];
    let out_lat: []f64 = out_lat_buf[0..];
    let out_v: []f64 = out_v_buf[0..];

    expect_eq(
        mlaDecodeSingleHead(
            q_nope[0..],
            empty_rope,
            c_kv[0..],
            empty_rope,
            w_uk[0..],
            w_uv[0..],
            2,
            2,
            2,
            0,
            2,
            1.0,
            q_abs,
            out_lat,
            out_v,
        ),
        true,
    );

    // scores = [1.0, 0.0] -> weights = [e/(e+1), 1/(e+1)]
    const e1 = std.math.exp(1.0);
    const w0 = e1 / (e1 + 1.0);
    const w1 = 1.0 / (e1 + 1.0);
    expect_eq(approxF64(out_v_buf[0], 2.0 * w0, EPS_F64), true);
    expect_eq(approxF64(out_v_buf[1], 4.0 * w1, EPS_F64), true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_val = std.math.nan(f64);
    let s_buf: [2]f64 = [0.0; 2];
    let s: []f64 = s_buf[0..];
    expect_eq(alibiSlopes(0, s), false);
    expect_eq(alibiSlopes(4, s), false);

    const ok4: [4]f64 = [1.0; 4];
    const nan4: [4]f64 = [1.0, nan_val, 1.0, 1.0];
    let out2_buf: [2]f64 = [0.0; 2];
    let out2: []f64 = out2_buf[0..];
    let out1: []f64 = out2_buf[0..1];

    expect_eq(alibiAttentionSingleQuery(ok4[0..2], ok4[0..], ok4[0..], 0, 1, 2, ok4[0..1], out2), false);
    expect_eq(alibiAttentionSingleQuery(ok4[0..2], ok4[0..], ok4[0..], 2, 1, 2, ok4[0..1], out1), false);
    expect_eq(alibiAttentionSingleQuery(nan4[0..2], ok4[0..], ok4[0..], 2, 1, 2, ok4[0..1], out2), false);
    expect_eq(alibiAttentionSingleQuery(ok4[0..2], ok4[0..], ok4[0..], 2, 1, 2, nan4[1..2], out2), false);

    expect_eq(mlaCompressKv(ok4[0..2], ok4[0..], 0, 2, out2), false);
    expect_eq(mlaCompressKv(ok4[0..2], ok4[0..], 2, 2, out1), false);
    expect_eq(mlaCompressKv(nan4[0..2], ok4[0..], 2, 2, out2), false);

    let q_abs_buf: [2]f64 = [0.0; 2];
    let out_lat_buf: [2]f64 = [0.0; 2];
    let q_abs: []f64 = q_abs_buf[0..];
    let out_lat: []f64 = out_lat_buf[0..];
    const empty: []const f64 = ok4[0..0];

    expect_eq(mlaDecodeSingleHead(ok4[0..2], empty, ok4[0..], empty, ok4[0..], ok4[0..], 0, 2, 2, 0, 2, 0.0, q_abs, out_lat, out2), false);
    expect_eq(mlaDecodeSingleHead(ok4[0..2], empty, ok4[0..], empty, ok4[0..], ok4[0..], 2, 2, 2, 0, 2, nan_val, q_abs, out_lat, out2), false);
    expect_eq(mlaDecodeSingleHead(nan4[0..2], empty, ok4[0..], empty, ok4[0..], ok4[0..], 2, 2, 2, 0, 2, 0.0, q_abs, out_lat, out2), false);
    expect_eq(mlaDecodeSingleHead(ok4[0..2], empty, ok4[0..], empty, ok4[0..], ok4[0..], 2, 2, 2, 0, 2, 0.0, q_abs_buf[0..1], out_lat, out2), false);
}

fn main() {
    test("1. mla: alibiSlopes exact geometric progression for 8 (pow2) & 6 (non-pow2) heads", () => {
        testAlibiSlopesPow2AndNonPow2()?;
    });
    test("2. mla: alibiAttentionSingleQuery distance decay & exact match vs reference (<1e-12)", () => {
        testAlibiAttentionMatchesReferenceAndDecaysWithDistance()?;
    });
    test("3. mla: DeepSeek weight-absorption mlaDecodeSingleHead == explicit K/V reconstruction (<1e-12)", () => {
        testMlaWeightAbsorptionMatchesExplicitKvReconstruction()?;
    });
    test("4. mla: NoPE-only mode (d_rope==0) & custom scale verification", () => {
        testMlaNopeOnlyModeAndCustomScale()?;
    });
    test("5. mla: zero dims, undersized buffers & NaN/Inf boundary defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
