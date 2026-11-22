// Origin: Pure-CP Stdlib Test Suite (GGML-Style Q8_0 & Q4_0 Block Quantization and GEMV) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/quant_matmul.cp
// Covers:
//   1. Q8_0 64D quantize/dequantize relative L2 error < 1% & dotQ8_0 precision
//   2. Q8_0 16x64 gemvQ8_0 vs exact f64 W*x (cosine > 0.9999, rel L2 error < 1%)
//   3. Q4_0 64D nibble pack/unpack & 16x64 mixed-precision gemvQ4_0_Q8_0 (cosine > 0.99 vs exact f64)
//   4. All-zero blocks (zero division safety, exact 0.0 outputs)
//   5. Non-multiple-of-32 lengths, buffer underflow, NaN/Inf & invalid dims defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    QUANT_BLOCK_SIZE,
    BlockQ8_0,
    BlockQ4_0,
    quantizeRowQ8_0,
    dequantizeRowQ8_0,
    dotQ8_0,
    gemvQ8_0,
    quantizeRowQ4_0,
    dequantizeRowQ4_0,
    gemvQ4_0_Q8_0,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn dotF64(a: []const f64, b: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        s += a[i] * b[i];
    }
    return s;
}

fn cosineF64(a: []const f64, b: []const f64): f64 {
    const na = std.math.sqrt(dotF64(a, a));
    const nb = std.math.sqrt(dotF64(b, b));
    return dotF64(a, b) / (na * nb);
}

fn testQ80QuantizeDequantizeAndDot64D()! {
    expect_eq(QUANT_BLOCK_SIZE, 32);

    let va: [64]f64 = [0.0; 64];
    let vb: [64]f64 = [0.0; 64];
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        const fi = i as f64;
        va[i] = std.math.sin(fi * 0.21) * 2.5 + std.math.cos(fi * 0.09) * 0.8;
        vb[i] = std.math.sin(fi * 0.21 + 0.3) * 1.9 - std.math.cos(fi * 0.13) * 0.6;
    }

    let qa_buf: [2]BlockQ8_0 = [BlockQ8_0{}; 2];
    let qb_buf: [2]BlockQ8_0 = [BlockQ8_0{}; 2];
    let qa: []BlockQ8_0 = qa_buf[0..];
    let qb: []BlockQ8_0 = qb_buf[0..];
    expect_eq(quantizeRowQ8_0(va[0..], qa), true);
    expect_eq(quantizeRowQ8_0(vb[0..], qb), true);

    let rec_a_buf: [64]f64 = [0.0; 64];
    let rec_b_buf: [64]f64 = [0.0; 64];
    let rec_a: []f64 = rec_a_buf[0..];
    let rec_b: []f64 = rec_b_buf[0..];
    expect_eq(dequantizeRowQ8_0(qa_buf[0..], rec_a), true);
    expect_eq(dequantizeRowQ8_0(qb_buf[0..], rec_b), true);

    // Relative L2 reconstruction error < 1%
    let err_sq: f64 = 0.0;
    let j: usize = 0;
    while (j < 64) : (j += 1) {
        const d = va[j] - rec_a_buf[j];
        err_sq += d * d;
    }
    const rel_err = std.math.sqrt(err_sq) / std.math.sqrt(dotF64(va[0..], va[0..]));
    expect_eq(rel_err < 0.01, true);

    // dotQ8_0 matches dequantized dot to < 1e-12 and exact f64 dot within < 1%
    const q_dot = dotQ8_0(qa_buf[0..], qb_buf[0..]) ?? 0.0;
    const deq_dot = dotF64(rec_a_buf[0..], rec_b_buf[0..]);
    const exact_dot = dotF64(va[0..], vb[0..]);
    expect_eq(approxF64(q_dot, deq_dot, EPS_F64), true);
    expect_eq(@abs(q_dot - exact_dot) / @abs(exact_dot) < 0.01, true);
}

fn testGemvQ80AndQ40Q8016x64()! {
    const ROWS: usize = 16;
    const COLS: usize = 64;
    const BPR: usize = 2;

    let x_fp: [64]f64 = [0.0; 64];
    let c: usize = 0;
    while (c < COLS) : (c += 1) {
        const fc = c as f64;
        x_fp[c] = std.math.sin(fc * 0.19 + 0.5) * 1.8 + 0.4 * std.math.cos(fc * 0.31);
    }

    let x_q8_buf: [2]BlockQ8_0 = [BlockQ8_0{}; 2];
    let x_q8: []BlockQ8_0 = x_q8_buf[0..];
    expect_eq(quantizeRowQ8_0(x_fp[0..], x_q8), true);

    let w_q8_buf: [32]BlockQ8_0 = [BlockQ8_0{}; 32]; // 16 * 2
    let w_q4_buf: [32]BlockQ4_0 = [BlockQ4_0{}; 32]; // 16 * 2
    let y_fp: [16]f64 = [0.0; 16];
    let row_fp: [64]f64 = [0.0; 64];
    let row_deq_q4_buf: [64]f64 = [0.0; 64];
    let row_deq_q4: []f64 = row_deq_q4_buf[0..];
    let x_deq_q8_buf: [64]f64 = [0.0; 64];
    let x_deq_q8: []f64 = x_deq_q8_buf[0..];
    expect_eq(dequantizeRowQ8_0(x_q8_buf[0..], x_deq_q8), true);

    let y_deq_q4_ref: [16]f64 = [0.0; 16];

    let r: usize = 0;
    while (r < ROWS) : (r += 1) {
        const fr = (r + 1) as f64;
        let col: usize = 0;
        while (col < COLS) : (col += 1) {
            const fc = col as f64;
            row_fp[col] = std.math.sin(fr * 0.73 + fc * 0.17) * 1.5 + std.math.cos(fr * 0.29 - fc * 0.23);
        }
        y_fp[r] = dotF64(row_fp[0..], x_fp[0..]);

        let w8_row: []BlockQ8_0 = w_q8_buf[r * BPR .. (r + 1) * BPR];
        let w4_row: []BlockQ4_0 = w_q4_buf[r * BPR .. (r + 1) * BPR];
        expect_eq(quantizeRowQ8_0(row_fp[0..], w8_row), true);
        expect_eq(quantizeRowQ4_0(row_fp[0..], w4_row), true);

        expect_eq(dequantizeRowQ4_0(w_q4_buf[r * BPR .. (r + 1) * BPR], row_deq_q4), true);
        y_deq_q4_ref[r] = dotF64(row_deq_q4_buf[0..], x_deq_q8_buf[0..]);
    }

    // 1. gemvQ8_0 accuracy
    let y_q8_buf: [16]f64 = [0.0; 16];
    let y_q8: []f64 = y_q8_buf[0..];
    expect_eq(gemvQ8_0(w_q8_buf[0..], x_q8_buf[0..], ROWS, BPR, y_q8), true);
    expect_eq(cosineF64(y_q8_buf[0..], y_fp[0..]) > 0.9999, true);

    // 2. gemvQ4_0_Q8_0 accuracy: cosine > 0.99 vs full-precision AND exact match to dequantized dot
    let y_q4_buf: [16]f64 = [0.0; 16];
    let y_q4: []f64 = y_q4_buf[0..];
    expect_eq(gemvQ4_0_Q8_0(w_q4_buf[0..], x_q8_buf[0..], ROWS, BPR, y_q4), true);
    expect_eq(cosineF64(y_q4_buf[0..], y_fp[0..]) > 0.99, true);

    let r2: usize = 0;
    while (r2 < ROWS) : (r2 += 1) {
        expect_eq(approxF64(y_q4_buf[r2], y_deq_q4_ref[r2], EPS_F64), true);
    }
}

fn testAllZeroBlocksNoDivByZero()! {
    const zeros: [64]f64 = [0.0; 64];
    let q8_buf: [2]BlockQ8_0 = [BlockQ8_0{}; 2];
    let q4_buf: [2]BlockQ4_0 = [BlockQ4_0{}; 2];
    let q8: []BlockQ8_0 = q8_buf[0..];
    let q4: []BlockQ4_0 = q4_buf[0..];

    expect_eq(quantizeRowQ8_0(zeros[0..], q8), true);
    expect_eq(quantizeRowQ4_0(zeros[0..], q4), true);
    expect_eq(q8_buf[0].scale == 0.0, true);
    expect_eq(q4_buf[0].scale == 0.0, true);

    let out_buf: [64]f64 = [9.0; 64];
    let out: []f64 = out_buf[0..];
    expect_eq(dequantizeRowQ8_0(q8_buf[0..], out), true);
    expect_eq(approxF64(out_buf[0], 0.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[63], 0.0, EPS_F64), true);

    out_buf[0] = 9.0;
    out_buf[63] = 9.0;
    expect_eq(dequantizeRowQ4_0(q4_buf[0..], out), true);
    expect_eq(approxF64(out_buf[0], 0.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[63], 0.0, EPS_F64), true);

    expect_eq(approxF64(dotQ8_0(q8_buf[0..], q8_buf[0..]) ?? 99.0, 0.0, EPS_F64), true);

    let y1_buf: [1]f64 = [99.0];
    let y1: []f64 = y1_buf[0..];
    expect_eq(gemvQ4_0_Q8_0(q4_buf[0..], q8_buf[0..], 1, 2, y1), true);
    expect_eq(approxF64(y1_buf[0], 0.0, EPS_F64), true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_v = std.math.nan(f64);
    const inf_v = std.math.inf(f64);

    let v32: [32]f64 = [1.0; 32];
    let v32_nan: [32]f64 = [1.0; 32];
    v32_nan[17] = nan_v;
    let v32_inf: [32]f64 = [1.0; 32];
    v32_inf[5] = inf_v;
    const v30: [30]f64 = [1.0; 30];
    const v_empty: []const f64 = v32[0..0];

    let q8_1_buf: [1]BlockQ8_0 = [BlockQ8_0{}; 1];
    let q4_1_buf: [1]BlockQ4_0 = [BlockQ4_0{}; 1];
    let q8_1: []BlockQ8_0 = q8_1_buf[0..];
    let q4_1: []BlockQ4_0 = q4_1_buf[0..];
    let q8_0: []BlockQ8_0 = q8_1_buf[0..0];
    let q4_0: []BlockQ4_0 = q4_1_buf[0..0];

    // Non-multiple of 32, empty, NaN/Inf, buffer too small
    expect_eq(quantizeRowQ8_0(v_empty, q8_1), false);
    expect_eq(quantizeRowQ8_0(v30[0..], q8_1), false);
    expect_eq(quantizeRowQ8_0(v32[0..], q8_0), false);
    expect_eq(quantizeRowQ8_0(v32_nan[0..], q8_1), false);
    expect_eq(quantizeRowQ8_0(v32_inf[0..], q8_1), false);

    expect_eq(quantizeRowQ4_0(v_empty, q4_1), false);
    expect_eq(quantizeRowQ4_0(v30[0..], q4_1), false);
    expect_eq(quantizeRowQ4_0(v32[0..], q4_0), false);
    expect_eq(quantizeRowQ4_0(v32_nan[0..], q4_1), false);

    // Dequantize small output & NaN scale defenses
    let dst30_buf: [30]f64 = [0.0; 30];
    let dst32_buf: [32]f64 = [0.0; 32];
    let dst30: []f64 = dst30_buf[0..];
    let dst32: []f64 = dst32_buf[0..];
    let bad_q8: [1]BlockQ8_0 = [BlockQ8_0{ scale: nan_v, qs: [0; 32] }];
    let bad_q4: [1]BlockQ4_0 = [BlockQ4_0{ scale: inf_v, qs: [0x88; 16] }];
    expect_eq(dequantizeRowQ8_0(q8_1_buf[0..], dst30), false);
    expect_eq(dequantizeRowQ8_0(bad_q8[0..], dst32), false);
    expect_eq(dequantizeRowQ4_0(q4_1_buf[0..], dst30), false);
    expect_eq(dequantizeRowQ4_0(bad_q4[0..], dst32), false);

    // dotQ8_0 & gemv defenses
    let q8_2_buf: [2]BlockQ8_0 = [BlockQ8_0{}; 2];
    expect_eq(dotQ8_0(q8_1_buf[0..], q8_2_buf[0..]) == null, true);
    expect_eq(dotQ8_0(bad_q8[0..], q8_1_buf[0..]) == null, true);

    let y_buf: [2]f64 = [0.0; 2];
    let y2: []f64 = y_buf[0..];
    let y1: []f64 = y_buf[0..1];
    expect_eq(gemvQ8_0(q8_2_buf[0..], q8_1_buf[0..], 2, 0, y2), false);
    expect_eq(gemvQ8_0(q8_2_buf[0..], q8_1_buf[0..], 2, 1, y1), false);
    expect_eq(gemvQ4_0_Q8_0(bad_q4[0..], q8_1_buf[0..], 1, 1, y1), false);
    expect_eq(gemvQ4_0_Q8_0(q4_1_buf[0..], bad_q8[0..], 1, 1, y1), false);
    expect_eq(gemvQ4_0_Q8_0(q4_1_buf[0..], q8_1_buf[0..], 2, 1, y2), false); // w_q4 len 1 < 2*1
}

fn main() {
    test("1. quant_matmul: Q8_0 64D quant/dequant (<1% L2 err) & dotQ8_0 precision", () => {
        testQ80QuantizeDequantizeAndDot64D()?;
    });
    test("2. quant_matmul: 16x64 gemvQ8_0 & mixed gemvQ4_0_Q8_0 (cosine > 0.99)", () => {
        testGemvQ80AndQ40Q8016x64()?;
    });
    test("3. quant_matmul: all-zero blocks safe without division by zero", () => {
        testAllZeroBlocksNoDivByZero()?;
    });
    test("4. quant_matmul: non-32-multiple, buffer underflow & NaN/Inf defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
    test("5. quant_matmul: Q4_0 exact nibble round-trip symmetry for [-7..+7]", () => {
        let ramp: [32]f64 = [0.0; 32];
        let i: usize = 0;
        while (i < 32) : (i += 1) {
            const v: i32 = ((i % 15) as i32) - 7; // exact integers in [-7..+7]
            ramp[i] = v as f64;
        }
        let q4_buf: [1]BlockQ4_0 = [BlockQ4_0{}; 1];
        let q4: []BlockQ4_0 = q4_buf[0..];
        expect_eq(quantizeRowQ4_0(ramp[0..], q4), true);
        let rec_buf: [32]f64 = [0.0; 32];
        let rec: []f64 = rec_buf[0..];
        expect_eq(dequantizeRowQ4_0(q4_buf[0..], rec), true);
        let j: usize = 0;
        while (j < 32) : (j += 1) {
            expect_eq(approxF64(rec_buf[j], ramp[j], EPS_F64), true);
        }
    });
}
