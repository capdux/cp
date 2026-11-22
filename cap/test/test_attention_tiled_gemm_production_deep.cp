// Origin: Pure-CP Stdlib Test Suite (lib/attention/tiled_gemm.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation Tiled GEMM & Batched MatMul
// using @dot, @fma, and @splitAtMut: Transposed-B accuracy & BLAS alpha/beta semantics,
// 32x48x24 large non-square tiled GEMM, multi-head batched QK^T, @splitAtMut row-split
// equivalence, and invalid dimension/NaN guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    gemmTransposedBF32,
    gemmF32,
    batchedGemmTransposedBF32,
    splitGemmRowsF32,
} from "cap:attention";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn testGemmTransposedBAndAlphaBetaSemantics()! {
    const m: usize = 5;
    const n: usize = 6;
    const k: usize = 19; // non-multiple of 8 to exercise SIMD main loop + scalar tail

    let a: [95]f32 = undefined;   // 5 * 19
    let b_t: [114]f32 = undefined; // 6 * 19
    let c: [30]f32 = undefined;   // 5 * 6

    let idx: usize = 0;
    while (idx < 95) : (idx += 1) {
        a[idx] = (((idx % 9) as f32) - 4.0) * 0.25;
    }
    idx = 0;
    while (idx < 114) : (idx += 1) {
        b_t[idx] = (((idx % 7) as f32) - 3.0) * 0.3;
    }
    // Pre-fill C with NaN to verify BLAS beta == 0.0 never reads existing C entries.
    idx = 0;
    while (idx < 30) : (idx += 1) {
        c[idx] = std.math.nan(f32);
    }

    let c_s: []f32 = c[0..];
    expect_eq(cap.attention.gemmTransposedBF32(a[0..], b_t[0..], m, n, k, 1.5, 0.0, c_s), true);

    // Compare against naive reference in f64.
    let c_ref: [30]f64 = undefined;
    let i: usize = 0;
    while (i < m) : (i += 1) {
        let j: usize = 0;
        while (j < n) : (j += 1) {
            let acc: f64 = 0.0;
            let p: usize = 0;
            while (p < k) : (p += 1) {
                acc += (a[i * k + p] as f64) * (b_t[j * k + p] as f64);
            }
            c_ref[i * n + j] = 1.5 * acc;
            expect_eq(absF64((c[i * n + j] as f64) - c_ref[i * n + j]) < 0.001, true);
        }
    }

    // Now test alpha = -0.5, beta = 2.0 accumulation into existing C.
    expect_eq(gemmTransposedBF32(a[0..], b_t[0..], m, n, k, -0.5, 2.0, c_s), true);
    i = 0;
    while (i < m) : (i += 1) {
        let j: usize = 0;
        while (j < n) : (j += 1) {
            let acc: f64 = 0.0;
            let p: usize = 0;
            while (p < k) : (p += 1) {
                acc += (a[i * k + p] as f64) * (b_t[j * k + p] as f64);
            }
            const expected: f64 = -0.5 * acc + 2.0 * c_ref[i * n + j];
            expect_eq(absF64((c[i * n + j] as f64) - expected) < 0.002, true);
        }
    }
}

fn testGemmF32LargeNonSquare32x48x24()! {
    const m: usize = 32;
    const n: usize = 48;
    const k: usize = 24;

    let a: [768]f32 = undefined;   // 32 * 24
    let b: [1152]f32 = undefined;  // 24 * 48
    let c: [1536]f32 = undefined;  // 32 * 48

    let idx: usize = 0;
    while (idx < 768) : (idx += 1) {
        a[idx] = (((idx % 13) as f32) - 6.0) * 0.125;
    }
    idx = 0;
    while (idx < 1152) : (idx += 1) {
        b[idx] = (((idx % 11) as f32) - 5.0) * 0.125;
    }
    idx = 0;
    while (idx < 1536) : (idx += 1) {
        c[idx] = 1.0;
    }

    let c_s: []f32 = c[0..];
    expect_eq(cap.attention.gemmF32(a[0..], b[0..], m, n, k, 1.0, 0.5, c_s), true);

    let i: usize = 0;
    while (i < m) : (i += 1) {
        let j: usize = 0;
        while (j < n) : (j += 1) {
            let acc: f64 = 0.0;
            let p: usize = 0;
            while (p < k) : (p += 1) {
                acc += (a[i * k + p] as f64) * (b[p * n + j] as f64);
            }
            const expected: f64 = acc + 0.5;
            expect_eq(absF64((c[i * n + j] as f64) - expected) < 0.002, true);
        }
    }
}

fn testBatchedGemmTransposedBMultiHeadQKT()! {
    const batch: usize = 4;
    const m: usize = 6;
    const n: usize = 8;
    const k: usize = 10;

    let q: [240]f32 = undefined;   // 4 * 6 * 10
    let k_t: [320]f32 = undefined; // 4 * 8 * 10
    let out: [192]f32 = undefined; // 4 * 6 * 8

    let idx: usize = 0;
    while (idx < 240) : (idx += 1) {
        q[idx] = (((idx % 11) as f32) - 5.0) * 0.2;
    }
    idx = 0;
    while (idx < 320) : (idx += 1) {
        k_t[idx] = (((idx % 7) as f32) - 3.0) * 0.25;
    }

    let out_s: []f32 = out[0..];
    expect_eq(cap.attention.batchedGemmTransposedBF32(q[0..], k_t[0..], batch, m, n, k, out_s), true);

    let h: usize = 0;
    while (h < batch) : (h += 1) {
        const q_off: usize = h * m * k;
        const k_off: usize = h * n * k;
        const c_off: usize = h * m * n;
        let i: usize = 0;
        while (i < m) : (i += 1) {
            let j: usize = 0;
            while (j < n) : (j += 1) {
                let acc: f64 = 0.0;
                let p: usize = 0;
                while (p < k) : (p += 1) {
                    acc += (q[q_off + i * k + p] as f64) * (k_t[k_off + j * k + p] as f64);
                }
                expect_eq(absF64((out[c_off + i * n + j] as f64) - acc) < 0.001, true);
            }
        }
    }
}

fn testSplitGemmRowsAtMutEquivalence()! {
    const m: usize = 19;
    const n: usize = 7;
    const k: usize = 12;

    let a: [228]f32 = undefined;   // 19 * 12
    let b_t: [84]f32 = undefined;  // 7 * 12
    let c_ref: [133]f32 = undefined;
    let c_split: [133]f32 = undefined;

    let idx: usize = 0;
    while (idx < 228) : (idx += 1) {
        a[idx] = (((idx % 7) as f32) - 3.0) * 0.25;
    }
    idx = 0;
    while (idx < 84) : (idx += 1) {
        b_t[idx] = (((idx % 5) as f32) - 2.0) * 0.4;
    }

    let c_ref_s: []f32 = c_ref[0..];
    let c_split_s: []f32 = c_split[0..];
    expect_eq(gemmTransposedBF32(a[0..], b_t[0..], m, n, k, 1.0, 0.0, c_ref_s), true);

    // Test split_row = 0 (all bottom), split_row = 9 (middle), and split_row = 19 (all top).
    const splits: [3]usize = [0, 9, 19];
    let s: usize = 0;
    while (s < 3) : (s += 1) {
        expect_eq(cap.attention.splitGemmRowsF32(a[0..], b_t[0..], m, n, k, splits[s], c_split_s), true);
        let elem: usize = 0;
        while (elem < 133) : (elem += 1) {
            expect_eq(absF64((c_split[elem] as f64) - (c_ref[elem] as f64)) < 0.0001, true);
        }
    }
}

fn testInvalidDimensionsAndDefensiveGuards()! {
    const a: [8]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];
    const b: [8]f32 = [1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0];
    let c: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    let c_s: []f32 = c[0..];

    // Zero dimensions rejected.
    expect_eq(gemmTransposedBF32(a[0..], b[0..], 0, 2, 4, 1.0, 0.0, c_s), false);
    expect_eq(gemmTransposedBF32(a[0..], b[0..], 2, 0, 4, 1.0, 0.0, c_s), false);
    expect_eq(gemmTransposedBF32(a[0..], b[0..], 2, 2, 0, 1.0, 0.0, c_s), false);
    expect_eq(gemmF32(a[0..], b[0..], 0, 2, 4, 1.0, 0.0, c_s), false);
    expect_eq(batchedGemmTransposedBF32(a[0..], b[0..], 0, 2, 2, 4, c_s), false);

    // Non-finite alpha/beta rejected.
    expect_eq(gemmTransposedBF32(a[0..], b[0..], 2, 2, 4, std.math.nan(f32), 0.0, c_s), false);
    expect_eq(gemmF32(a[0..], b[0..], 2, 2, 4, 1.0, std.math.inf(f32), c_s), false);

    // Out-of-bounds split_row and truncated buffers rejected.
    expect_eq(splitGemmRowsF32(a[0..], b[0..], 2, 2, 4, 3, c_s), false);
    expect_eq(gemmTransposedBF32(a[0..7], b[0..], 2, 2, 4, 1.0, 0.0, c_s), false);
    expect_eq(gemmF32(a[0..], b[0..], 2, 2, 4, 1.0, 0.0, c[0..3]), false);
    expect_eq(batchedGemmTransposedBF32(a[0..], b[0..], 2, 2, 2, 4, c_s), false);
}

fn main() {
    test("1. attention/tiled_gemm: gemmTransposedBF32 @dot accuracy & BLAS alpha/beta semantics", () => {
        testGemmTransposedBAndAlphaBetaSemantics()?;
    });

    test("2. attention/tiled_gemm: gemmF32 large non-square 32x48x24 tiled ikj @fma accuracy", () => {
        testGemmF32LargeNonSquare32x48x24()?;
    });

    test("3. attention/tiled_gemm: batchedGemmTransposedBF32 multi-head QK^T verification", () => {
        testBatchedGemmTransposedBMultiHeadQKT()?;
    });

    test("4. attention/tiled_gemm: splitGemmRowsF32 @splitAtMut zero-copy row partitioning", () => {
        testSplitGemmRowsAtMutEquivalence()?;
    });

    test("5. attention/tiled_gemm: invalid dimensions, split_row & NaN/Inf defensive guards", () => {
        testInvalidDimensionsAndDefensiveGuards()?;
    });
}
