// Origin: Pure-CP Stdlib Test Suite (Selective State Space Model Mamba S6 & Causal Depthwise Conv1D) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/ssm.cp
// Covers:
//   1. ZOH discretization analytical check: zero-input exponential decay h_t = exp(-dt * exp(a_log))^t * h_0
//   2. ssmSelectiveScan full-sequence scan matches step-by-step ssmSelectiveStep bit-identically (< 1e-12)
//   3. Chunked prefill + autoregressive single-step decode continuation equals single full scan
//   4. causalConv1dStep matches full reference causal depthwise 1D convolution across sequence
//   5. softplus(500.0) & softplus(-500.0) overflow/underflow immunity
//   6. Zero dims, negative dt, undersized buffers & NaN/Inf boundary defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    softplus,
    ssmSelectiveStep,
    ssmSelectiveScan,
    causalConv1dStep,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testZohAnalyticalExponentialDecay()! {
    const D_INNER: usize = 2;
    const D_STATE: usize = 3;

    const x_zero: [2]f64 = [0.0, 0.0];
    const dt: [2]f64 = [0.25, 0.5];
    const a_log: [6]f64 = [
        -0.5, 0.0, 0.4,
        -0.2, 0.3, 0.7,
    ];
    const b_vec: [3]f64 = [1.0, -0.5, 0.25];
    const c_vec: [3]f64 = [0.6, 0.3, 0.1];
    const no_d: []const f64 = x_zero[0..0];

    const h0: [6]f64 = [1.5, -2.0, 0.8, 3.0, 1.2, -0.9];
    let state_buf: [6]f64 = h0;
    let state: []f64 = state_buf[0..];
    let y_buf: [2]f64 = [0.0; 2];
    let y: []f64 = y_buf[0..];

    const STEPS: usize = 5;
    let step: usize = 0;
    while (step < STEPS) : (step += 1) {
        expect_eq(
            ssmSelectiveStep(x_zero[0..], dt[0..], a_log[0..], b_vec[0..], c_vec[0..], no_d, D_INNER, D_STATE, state, y),
            true,
        );
    }

    // Analytical check: h_T[i, n] = exp(-dt[i] * exp(a_log[i, n]))^T * h_0[i, n]
    let i: usize = 0;
    while (i < D_INNER) : (i += 1) {
        let expected_y: f64 = 0.0;
        let n: usize = 0;
        while (n < D_STATE) : (n += 1) {
            const idx = i * D_STATE + n;
            const decay_t = std.math.exp((0.0 - (STEPS as f64) * dt[i]) * std.math.exp(a_log[idx]));
            const expected_h = decay_t * h0[idx];
            expect_eq(approxF64(state_buf[idx], expected_h, EPS_F64), true);
            expected_y += c_vec[n] * expected_h;
        }
        expect_eq(approxF64(y_buf[i], expected_y, EPS_F64), true);
    }
}

fn testSelectiveScanMatchesStepByStep()! {
    const SEQ: usize = 6;
    const D_INNER: usize = 3;
    const D_STATE: usize = 4;

    let x_seq: [18]f64 = [0.0; 18];
    let dt_seq: [18]f64 = [0.0; 18];
    let b_seq: [24]f64 = [0.0; 24];
    let c_seq: [24]f64 = [0.0; 24];
    let a_log: [12]f64 = [0.0; 12];
    const d_skip: [3]f64 = [0.2, -0.5, 1.1];

    let idx0: usize = 0;
    while (idx0 < 18) : (idx0 += 1) {
        const fi = idx0 as f64;
        x_seq[idx0] = std.math.sin(fi * 0.41 + 0.3) * 1.5;
        dt_seq[idx0] = softplus(std.math.cos(fi * 0.29) * 0.8);
    }
    let idx1: usize = 0;
    while (idx1 < 24) : (idx1 += 1) {
        const fi = idx1 as f64;
        b_seq[idx1] = std.math.cos(fi * 0.37 - 0.2) * 0.9;
        c_seq[idx1] = std.math.sin(fi * 0.23 + 0.7) * 0.8;
    }
    let idx2: usize = 0;
    while (idx2 < 12) : (idx2 += 1) {
        a_log[idx2] = -0.6 + (idx2 as f64) * 0.12;
    }

    // Path A: full ssmSelectiveScan
    let st_scan_buf: [12]f64 = [0.0; 12];
    let st_scan: []f64 = st_scan_buf[0..];
    let y_scan_buf: [18]f64 = [0.0; 18];
    let y_scan: []f64 = y_scan_buf[0..];
    expect_eq(
        ssmSelectiveScan(
            x_seq[0..],
            dt_seq[0..],
            a_log[0..],
            b_seq[0..],
            c_seq[0..],
            d_skip[0..],
            SEQ,
            D_INNER,
            D_STATE,
            st_scan,
            y_scan,
        ),
        true,
    );

    // Path B: step-by-step ssmSelectiveStep
    let st_step_buf: [12]f64 = [0.0; 12];
    let st_step: []f64 = st_step_buf[0..];
    let y_step_buf: [18]f64 = [0.0; 18];
    let t: usize = 0;
    while (t < SEQ) : (t += 1) {
        const x_off = t * D_INNER;
        const bc_off = t * D_STATE;
        let y_t = y_step_buf[x_off .. x_off + D_INNER];
        expect_eq(
            ssmSelectiveStep(
                x_seq[x_off .. x_off + D_INNER],
                dt_seq[x_off .. x_off + D_INNER],
                a_log[0..],
                b_seq[bc_off .. bc_off + D_STATE],
                c_seq[bc_off .. bc_off + D_STATE],
                d_skip[0..],
                D_INNER,
                D_STATE,
                st_step,
                y_t,
            ),
            true,
        );
    }

    let k: usize = 0;
    while (k < 18) : (k += 1) {
        expect_eq(approxF64(y_scan_buf[k], y_step_buf[k], EPS_F64), true);
    }
    let s: usize = 0;
    while (s < 12) : (s += 1) {
        expect_eq(approxF64(st_scan_buf[s], st_step_buf[s], EPS_F64), true);
    }
}

fn testChunkedPrefillPlusDecodeContinuation()! {
    const SEQ: usize = 5;
    const PREFILL: usize = 3;
    const D_INNER: usize = 2;
    const D_STATE: usize = 2;

    const x_seq: [10]f64 = [0.5, -0.3, 1.2, 0.7, -0.8, 0.4, 0.9, -1.1, 0.2, 0.6];
    const dt_seq: [10]f64 = [0.1, 0.2, 0.15, 0.25, 0.3, 0.1, 0.2, 0.35, 0.05, 0.4];
    const b_seq: [10]f64 = [0.4, -0.2, 0.6, 0.3, -0.5, 0.7, 0.1, -0.8, 0.5, 0.2];
    const c_seq: [10]f64 = [0.8, 0.1, -0.3, 0.9, 0.2, -0.4, 0.7, 0.5, -0.6, 0.3];
    const a_log: [4]f64 = [-0.4, 0.1, -0.2, 0.5];
    const d_skip: [2]f64 = [0.3, 0.7];

    // Full scan in one call
    let st_full_buf: [4]f64 = [0.0; 4];
    let st_full: []f64 = st_full_buf[0..];
    let y_full_buf: [10]f64 = [0.0; 10];
    let y_full: []f64 = y_full_buf[0..];
    expect_eq(
        ssmSelectiveScan(
            x_seq[0..],
            dt_seq[0..],
            a_log[0..],
            b_seq[0..],
            c_seq[0..],
            d_skip[0..],
            SEQ,
            D_INNER,
            D_STATE,
            st_full,
            y_full,
        ),
        true,
    );

    // Chunked prefill (3 tokens) + 2 autoregressive single-step decodes
    let st_chunk_buf: [4]f64 = [0.0; 4];
    let st_chunk: []f64 = st_chunk_buf[0..];
    let y_chunk_buf: [10]f64 = [0.0; 10];
    let y_prefill = y_chunk_buf[0 .. PREFILL * D_INNER];
    expect_eq(
        ssmSelectiveScan(
            x_seq[0 .. PREFILL * D_INNER],
            dt_seq[0 .. PREFILL * D_INNER],
            a_log[0..],
            b_seq[0 .. PREFILL * D_STATE],
            c_seq[0 .. PREFILL * D_STATE],
            d_skip[0..],
            PREFILL,
            D_INNER,
            D_STATE,
            st_chunk,
            y_prefill,
        ),
        true,
    );

    let t: usize = PREFILL;
    while (t < SEQ) : (t += 1) {
        const x_off = t * D_INNER;
        const bc_off = t * D_STATE;
        let y_t = y_chunk_buf[x_off .. x_off + D_INNER];
        expect_eq(
            ssmSelectiveStep(
                x_seq[x_off .. x_off + D_INNER],
                dt_seq[x_off .. x_off + D_INNER],
                a_log[0..],
                b_seq[bc_off .. bc_off + D_STATE],
                c_seq[bc_off .. bc_off + D_STATE],
                d_skip[0..],
                D_INNER,
                D_STATE,
                st_chunk,
                y_t,
            ),
            true,
        );
    }

    let i: usize = 0;
    while (i < 10) : (i += 1) {
        expect_eq(approxF64(y_chunk_buf[i], y_full_buf[i], EPS_F64), true);
    }
    let s: usize = 0;
    while (s < 4) : (s += 1) {
        expect_eq(approxF64(st_chunk_buf[s], st_full_buf[s], EPS_F64), true);
    }
}

fn testCausalConv1dStepMatchesFullConv()! {
    const SEQ: usize = 6;
    const D_INNER: usize = 2;
    const K: usize = 4;

    const x_seq: [12]f64 = [
        1.0, -0.5,
        2.0,  0.25,
        -1.5, 1.0,
        0.5,  2.0,
        3.0, -1.0,
        -0.5, 0.75,
    ];
    // weight[i, 0..3]: oldest tap -> newest tap
    const weight: [8]f64 = [
        0.1, 0.2, 0.3, 0.4,
        -0.2, 0.5, -0.1, 0.8,
    ];
    const bias: [2]f64 = [0.25, -0.5];

    let conv_state_buf: [6]f64 = [0.0; 6]; // D_INNER * (K - 1) = 2 * 3
    let conv_state: []f64 = conv_state_buf[0..];
    let y_step_buf: [2]f64 = [0.0; 2];
    let y_step: []f64 = y_step_buf[0..];

    let t: usize = 0;
    while (t < SEQ) : (t += 1) {
        expect_eq(
            causalConv1dStep(
                x_seq[t * D_INNER .. (t + 1) * D_INNER],
                weight[0..],
                bias[0..],
                D_INNER,
                K,
                conv_state,
                y_step,
            ),
            true,
        );

        // Compare with direct causal convolution reference at time t
        let c: usize = 0;
        while (c < D_INNER) : (c += 1) {
            let ref_val: f64 = bias[c];
            let k: usize = 0;
            while (k < K) : (k += 1) {
                const lag = (K - 1) - k;
                if (t >= lag) {
                    const src_t = t - lag;
                    ref_val += x_seq[src_t * D_INNER + c] * weight[c * K + k];
                }
            }
            expect_eq(approxF64(y_step_buf[c], ref_val, EPS_F64), true);
        }
    }

    // Kernel size == 1 works with empty conv_state
    const w_k1: [2]f64 = [2.0, -3.0];
    let empty_state: []f64 = conv_state_buf[0..0];
    expect_eq(causalConv1dStep(x_seq[0..2], w_k1[0..], bias[0..], 2, 1, empty_state, y_step), true);
    expect_eq(approxF64(y_step_buf[0], 1.0 * 2.0 + 0.25, EPS_F64), true);
    expect_eq(approxF64(y_step_buf[1], (-0.5) * (-3.0) - 0.5, EPS_F64), true);
}

fn testSoftplusStability()! {
    // Large positive x -> linear pass-through without exp overflow
    expect_eq(approxF64(softplus(500.0), 500.0, EPS_F64), true);
    expect_eq(approxF64(softplus(1000.0), 1000.0, EPS_F64), true);
    // Zero -> ln(2)
    expect_eq(approxF64(softplus(0.0), std.math.ln2, EPS_F64), true);
    // Moderate positive & negative values
    const neg2: f64 = -2.0;
    expect_eq(approxF64(softplus(2.0), 2.0 + std.math.log1p(std.math.exp(neg2)), EPS_F64), true);
    expect_eq(approxF64(softplus(-2.0), std.math.log1p(std.math.exp(neg2)), EPS_F64), true);
    // Large negative x -> finite positive near 0, no NaN
    const sp_neg = softplus(-500.0);
    expect_eq(!std.math.isNan(sp_neg) && !std.math.isInf(sp_neg) && sp_neg >= 0.0, true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const x_ok: [2]f64 = [1.0, 2.0];
    const dt_ok: [2]f64 = [0.1, 0.2];
    const dt_neg: [2]f64 = [0.1, -0.01];
    const a_ok: [4]f64 = [-0.5, 0.0, 0.2, 0.4];
    const b_ok: [2]f64 = [0.5, -0.5];
    const c_ok: [2]f64 = [0.3, 0.7];
    const d_ok: [2]f64 = [1.0, 1.0];
    const nan_val = std.math.nan(f64);
    const x_nan: [2]f64 = [1.0, nan_val];
    const a_nan: [4]f64 = [-0.5, nan_val, 0.2, 0.4];

    let st_buf: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    let st: []f64 = st_buf[0..];
    let st_short: []f64 = st_buf[0..2];
    let y_buf: [2]f64 = [0.0; 2];
    let y: []f64 = y_buf[0..];
    let y_short: []f64 = y_buf[0..1];

    // ssmSelectiveStep defenses
    expect_eq(ssmSelectiveStep(x_ok[0..], dt_ok[0..], a_ok[0..], b_ok[0..], c_ok[0..], d_ok[0..], 0, 2, st, y), false);
    expect_eq(ssmSelectiveStep(x_ok[0..], dt_ok[0..], a_ok[0..], b_ok[0..], c_ok[0..], d_ok[0..], 2, 0, st, y), false);
    expect_eq(ssmSelectiveStep(x_ok[0..], dt_neg[0..], a_ok[0..], b_ok[0..], c_ok[0..], d_ok[0..], 2, 2, st, y), false);
    expect_eq(ssmSelectiveStep(x_ok[0..], dt_ok[0..], a_ok[0..], b_ok[0..], c_ok[0..], d_ok[0..], 2, 2, st_short, y), false);
    expect_eq(ssmSelectiveStep(x_ok[0..], dt_ok[0..], a_ok[0..], b_ok[0..], c_ok[0..], d_ok[0..], 2, 2, st, y_short), false);
    expect_eq(ssmSelectiveStep(x_nan[0..], dt_ok[0..], a_ok[0..], b_ok[0..], c_ok[0..], d_ok[0..], 2, 2, st, y), false);
    expect_eq(ssmSelectiveStep(x_ok[0..], dt_ok[0..], a_nan[0..], b_ok[0..], c_ok[0..], d_ok[0..], 2, 2, st, y), false);

    // ssmSelectiveScan atomic no-mutation defense on negative dt in later token
    const x_seq2: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const dt_seq2_bad: [4]f64 = [0.1, 0.2, 0.1, -0.5];
    const bc_seq2: [4]f64 = [0.5, 0.5, 0.5, 0.5];
    let y_seq2_buf: [4]f64 = [0.0; 4];
    let y_seq2: []f64 = y_seq2_buf[0..];
    expect_eq(
        ssmSelectiveScan(x_seq2[0..], dt_seq2_bad[0..], a_ok[0..], bc_seq2[0..], bc_seq2[0..], d_ok[0..], 2, 2, 2, st, y_seq2),
        false,
    );
    expect_eq(approxF64(st_buf[0], 1.0, EPS_F64), true); // state untouched
    expect_eq(
        ssmSelectiveScan(x_seq2[0..], dt_ok[0..], a_ok[0..], bc_seq2[0..], bc_seq2[0..], d_ok[0..], 0, 2, 2, st, y_seq2),
        false,
    );

    // causalConv1dStep defenses
    const w_ok: [6]f64 = [0.1; 6];
    let cs_buf: [4]f64 = [0.0; 4];
    let cs: []f64 = cs_buf[0..];
    let cs_short: []f64 = cs_buf[0..2];
    expect_eq(causalConv1dStep(x_ok[0..], w_ok[0..], d_ok[0..], 0, 3, cs, y), false);
    expect_eq(causalConv1dStep(x_ok[0..], w_ok[0..], d_ok[0..], 2, 0, cs, y), false);
    expect_eq(causalConv1dStep(x_ok[0..], w_ok[0..4], d_ok[0..], 2, 3, cs, y), false);
    expect_eq(causalConv1dStep(x_ok[0..], w_ok[0..], d_ok[0..], 2, 3, cs_short, y), false);
    expect_eq(causalConv1dStep(x_nan[0..], w_ok[0..], d_ok[0..], 2, 3, cs, y), false);
}

fn main() {
    test("1. ssm: ZOH analytical exponential decay h_t = exp(-dt*exp(a_log))^t * h_0", () => {
        testZohAnalyticalExponentialDecay()?;
    });
    test("2. ssm: ssmSelectiveScan matches step-by-step ssmSelectiveStep (<1e-12)", () => {
        testSelectiveScanMatchesStepByStep()?;
    });
    test("3. ssm: chunked prefill + single-step decode equals full scan", () => {
        testChunkedPrefillPlusDecodeContinuation()?;
    });
    test("4. ssm: causalConv1dStep matches full reference causal depthwise Conv1D", () => {
        testCausalConv1dStepMatchesFullConv()?;
    });
    test("5. ssm: softplus(500.0) & softplus(-500.0) numerical stability", () => {
        testSoftplusStability()?;
    });
    test("6. ssm: zero dims, negative dt, undersized buffers & NaN defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
