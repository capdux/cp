// Source: cp/cap/test/test_dct_production_deep.cp (Production deep tests for cap:dct)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DCT_BLOCK_DIM,
    DCT_BLOCK_SIZE,
    JPEG_ZIGZAG_8X8,
    JPEG_LUMA_QUANT_50,
    JPEG_CHROMA_QUANT_50,
    dct1d,
    idct1d,
    dct8x8,
    idct8x8,
    zigzagReorder8x8,
    unzigzagReorder8x8,
    quantize8x8,
    dequantize8x8,
    scaleQuantTable,
} from "cap:dct";

fn absDiff(a: f64, b: f64): f64 {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

test("1. 1D orthonormal DCT-II / IDCT-III exact round-trip (< 1e-11) and Parseval energy conservation", () => {
    expect_eq(DCT_BLOCK_DIM, 8 as usize);
    expect_eq(DCT_BLOCK_SIZE, 64 as usize);

    const sig: [13]f64 = [
        12.5, -3.25, 7.0, 0.0, 44.125, -19.5, 8.75, 2.0,
        -11.0, 31.5, 6.25, -0.5, 15.0,
    ];
    let freq_arr: [13]f64 = [0.0; 13];
    let rec_arr: [13]f64 = [0.0; 13];
    let freq: []f64 = freq_arr[0..];
    let rec: []f64 = rec_arr[0..];

    expect_eq(dct1d(sig[0..], *freq), true);
    expect_eq(idct1d(freq_arr[0..], *rec), true);

    let energy_in: f64 = 0.0;
    let energy_freq: f64 = 0.0;
    let i: usize = 0;
    while (i < 13) {
        expect_eq(absDiff(rec_arr[i], sig[i]) < 1.0e-11, true);
        energy_in += sig[i] * sig[i];
        energy_freq += freq_arr[i] * freq_arr[i];
        i += 1;
    }
    // Parseval's theorem: sum(x^2) == sum(X^2).
    expect_eq(absDiff(energy_in, energy_freq) < 1.0e-10, true);
});

test("2. 2D 8x8 constant block (DC == 8*c, all AC == 0) and arbitrary block exact round-trip (< 1e-10)", () => {
    // Constant block of value c = 42.5 -> DC must equal 8 * 42.5 = 340.0, all 63 AC == 0.
    let const_blk: [64]f64 = [42.5; 64];
    const const_dct: [64]f64 = dct8x8(const_blk);
    expect_eq(absDiff(const_dct[0], 340.0) < 1.0e-10, true);
    let ac: usize = 1;
    while (ac < 64) {
        expect_eq(absDiff(const_dct[ac], 0.0) < 1.0e-10, true);
        ac += 1;
    }

    // Arbitrary 8x8 pixel block (level-shifted -128..127) round-trip + Parseval check.
    let blk: [64]f64 = [0.0; 64];
    let r: usize = 0;
    while (r < 8) {
        let c: usize = 0;
        while (c < 8) {
            blk[r * 8 + c] = (((r * 17 + c * 29 + r * c * 3) % 256) as f64) - 128.0;
            c += 1;
        }
        r += 1;
    }

    const coeffs: [64]f64 = dct8x8(blk);
    const rebuilt: [64]f64 = idct8x8(coeffs);

    let e_in: f64 = 0.0;
    let e_out: f64 = 0.0;
    let i: usize = 0;
    while (i < 64) {
        expect_eq(absDiff(rebuilt[i], blk[i]) < 1.0e-10, true);
        e_in += blk[i] * blk[i];
        e_out += coeffs[i] * coeffs[i];
        i += 1;
    }
    expect_eq(absDiff(e_in, e_out) < 1.0e-9, true);
});

test("3. JPEG ZigZag / UnZigZag 64-element bijective permutation round-trip", () => {
    // Verify JPEG_ZIGZAG_8X8 contains every index 0..63 exactly once.
    let seen: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 64) {
        const idx: usize = JPEG_ZIGZAG_8X8[i] as usize;
        expect_eq(idx < 64, true);
        seen[idx] += 1;
        i += 1;
    }
    let j: usize = 0;
    while (j < 64) {
        expect_eq(seen[j], 1 as u8);
        j += 1;
    }

    let block: [64]i32 = [0; 64];
    let k: usize = 0;
    while (k < 64) {
        block[k] = (k as i32) * 10 - 300;
        k += 1;
    }
    const zz: [64]i32 = zigzagReorder8x8(block);
    // First 4 zigzag entries must come from raster positions 0, 1, 8, 16.
    expect_eq(zz[0], block[0]);
    expect_eq(zz[1], block[1]);
    expect_eq(zz[2], block[8]);
    expect_eq(zz[3], block[16]);

    const restored: [64]i32 = unzigzagReorder8x8(zz);
    let m: usize = 0;
    while (m < 64) {
        expect_eq(restored[m], block[m]);
        m += 1;
    }
});

test("4. IJG scaleQuantTable + JPEG quantize/dequantize pipeline: high-frequency zeroing and PSNR >= 35 dB", () => {
    // quality == 50 must reproduce JPEG_LUMA_QUANT_50 & JPEG_CHROMA_QUANT_50 verbatim.
    const q50: [64]u8 = scaleQuantTable(JPEG_LUMA_QUANT_50, 50);
    const c50: [64]u8 = scaleQuantTable(JPEG_CHROMA_QUANT_50, 50);
    let i: usize = 0;
    while (i < 64) {
        expect_eq(q50[i], JPEG_LUMA_QUANT_50[i]);
        expect_eq(c50[i], JPEG_CHROMA_QUANT_50[i]);
        i += 1;
    }

    // quality == 100 yields all 1s; quality == 10 yields larger quantization steps than quality == 80.
    const q100: [64]u8 = scaleQuantTable(JPEG_LUMA_QUANT_50, 100);
    const q80: [64]u8 = scaleQuantTable(JPEG_LUMA_QUANT_50, 80);
    const q10: [64]u8 = scaleQuantTable(JPEG_LUMA_QUANT_50, 10);
    expect_eq(q100[0], 1 as u8);
    expect_eq(q100[63], 1 as u8);
    expect_eq(q10[0] > q80[0], true);

    // Smooth gradient 8x8 image patch (level-shifted) quantized at quality 75.
    let img: [64]f64 = [0.0; 64];
    let r: usize = 0;
    while (r < 8) {
        let c: usize = 0;
        while (c < 8) {
            img[r * 8 + c] = ((r * 12 + c * 10) as f64) - 80.0;
            c += 1;
        }
        r += 1;
    }

    const qt75: [64]u8 = scaleQuantTable(JPEG_LUMA_QUANT_50, 75);
    const freq: [64]f64 = dct8x8(img);
    const quant: [64]i32 = quantize8x8(freq, qt75);
    const zz: [64]i32 = zigzagReorder8x8(quant);

    // High-frequency tail in ZigZag order should quantize to zero.
    let zeros_tail: usize = 0;
    let t: usize = 32;
    while (t < 64) {
        if (zz[t] == 0) {
            zeros_tail += 1;
        }
        t += 1;
    }
    expect_eq(zeros_tail >= 24, true);

    // Reconstruct and verify MSE is small (PSNR > 35 dB -> MSE < 255^2 / 10^3.5 ~= 20.5).
    const deq: [64]f64 = dequantize8x8(quant, qt75);
    const rec: [64]f64 = idct8x8(deq);
    let mse: f64 = 0.0;
    let p: usize = 0;
    while (p < 64) {
        const diff: f64 = rec[p] - img[p];
        mse += diff * diff;
        p += 1;
    }
    mse /= 64.0;
    expect_eq(mse < 10.0, true);
});

test("5. Boundary guards: empty slice, short output slice, N=1 1D DCT, and zero-entry qtable safety", () => {
    let empty_in: [0]f64 = undefined;
    let out_arr: [4]f64 = [0.0; 4];
    let out: []f64 = out_arr[0..];
    expect_eq(dct1d(empty_in[0..0], *out), false);
    expect_eq(idct1d(empty_in[0..0], *out), false);

    const five: [5]f64 = [1.0, 2.0, 3.0, 4.0, 5.0];
    expect_eq(dct1d(five[0..], *out), false);
    expect_eq(idct1d(five[0..], *out), false);

    // N = 1 identity transform.
    const one: [1]f64 = [99.25];
    expect_eq(dct1d(one[0..], *out), true);
    expect_eq(absDiff(out_arr[0], 99.25) < 1.0e-12, true);

    // qtable containing 0 must not divide by zero (treated as 1).
    const zero_qt: [64]u8 = [0; 64];
    const blk: [64]f64 = [7.4; 64];
    const q: [64]i32 = quantize8x8(blk, zero_qt);
    expect_eq(q[0], 7 as i32);
    const dq: [64]f64 = dequantize8x8(q, zero_qt);
    expect_eq(absDiff(dq[0], 7.0) < 1.0e-12, true);
});
