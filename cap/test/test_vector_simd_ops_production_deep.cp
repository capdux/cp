// Origin: Pure-CP Stdlib Test Suite (lib/vector/simd_ops.cp) | Created: 2026-11-22
// Production Deep Verification Suite for CP native SIMD intrinsics (@dot, @l2sq, @fma) vector
// kernels: multi-dimension (1/7/8/19/128) dot/L2/cosine accuracy, @fma AXPY in-place updates,
// batch Top-K Cosine & L2 ranking with deterministic tie-breaking, and zero/mismatch/NaN guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    simdDotF32,
    simdDotF64,
    simdL2SqF32,
    simdL2DistF32,
    simdCosineF32,
    simdAxpyInPlaceF32,
    simdBatchTopKCosineF32,
    simdBatchTopKL2F32,
} from "cap:vector";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn verifyDimAccuracy(dim: usize)! {
    let a: [128]f32 = undefined;
    let b: [128]f32 = undefined;
    let a64: [128]f64 = undefined;
    let b64: [128]f64 = undefined;

    let ref_dot: f64 = 0.0;
    let ref_l2sq: f64 = 0.0;
    let ref_na: f64 = 0.0;
    let ref_nb: f64 = 0.0;

    let i: usize = 0;
    while (i < dim) : (i += 1) {
        const av: f32 = (((i % 11) as f32) - 5.0) * 0.25;
        const bv: f32 = (((i % 7) as f32) - 3.0) * 0.3;
        a[i] = av;
        b[i] = bv;
        a64[i] = av as f64;
        b64[i] = bv as f64;

        const ad: f64 = av as f64;
        const bd: f64 = bv as f64;
        ref_dot += ad * bd;
        const diff: f64 = ad - bd;
        ref_l2sq += diff * diff;
        ref_na += ad * ad;
        ref_nb += bd * bd;
    }

    const ref_l2: f64 = @sqrt(ref_l2sq);
    let ref_cos: f64 = 0.0;
    if (ref_na > 0.0 && ref_nb > 0.0) {
        ref_cos = ref_dot / (@sqrt(ref_na) * @sqrt(ref_nb));
    }

    if (d32 := cap.vector.simdDotF32(a[0..dim], b[0..dim])) {
        expect_eq(absF64((d32 as f64) - ref_dot) < 0.001, true);
    } else {
        expect_eq(true, false);
    }

    if (d64 := simdDotF64(a64[0..dim], b64[0..dim])) {
        expect_eq(absF64(d64 - ref_dot) < 0.0000001, true);
    } else {
        expect_eq(true, false);
    }

    if (l2sq := simdL2SqF32(a[0..dim], b[0..dim])) {
        expect_eq(absF64((l2sq as f64) - ref_l2sq) < 0.002, true);
    } else {
        expect_eq(true, false);
    }

    if (l2d := simdL2DistF32(a[0..dim], b[0..dim])) {
        expect_eq(absF64((l2d as f64) - ref_l2) < 0.001, true);
    } else {
        expect_eq(true, false);
    }

    if (cs := simdCosineF32(a[0..dim], b[0..dim])) {
        expect_eq(absF64((cs as f64) - ref_cos) < 0.0005, true);
    } else {
        expect_eq(true, false);
    }
}

fn testMultiDimSimdDotL2CosineAccuracy()! {
    // Test 1, 7 (< 8-lane), 8 (exact 8-lane), 19 (2x8 + 3 tail), and 128 (16x8) dimensions.
    verifyDimAccuracy(1)?;
    verifyDimAccuracy(7)?;
    verifyDimAccuracy(8)?;
    verifyDimAccuracy(19)?;
    verifyDimAccuracy(128)?;
}

fn testFmaAxpyInPlaceUpdates()! {
    const x: [19]f32 = [
        1.0, -2.0, 3.5, 0.25, -4.0, 5.0, 6.0, -7.5,
        0.5, 1.5, -2.5, 3.0, 4.5, -5.5, 6.5, 7.0,
        -8.0, 9.25, -0.75,
    ];
    let y: [19]f32 = [
        0.5, 1.0, -1.5, 2.0, 2.5, -3.0, 3.5, 4.0,
        -4.5, 5.0, 5.5, -6.0, 6.5, 7.0, -7.5, 8.0,
        8.5, -9.0, 9.5,
    ];
    let expected: [19]f32 = undefined;
    const alpha: f32 = -1.5;

    let i: usize = 0;
    while (i < 19) : (i += 1) {
        expected[i] = alpha * x[i] + y[i];
    }

    let y_s: []f32 = y[0..];
    expect_eq(cap.vector.simdAxpyInPlaceF32(alpha, x[0..], y_s), true);

    i = 0;
    while (i < 19) : (i += 1) {
        expect_eq(absF64((y[i] as f64) - (expected[i] as f64)) < 0.0001, true);
    }

    // Zero alpha leaves y completely unchanged.
    const snap0: f32 = y[0];
    expect_eq(simdAxpyInPlaceF32(0.0, x[0..], y_s), true);
    expect_eq(y[0], snap0);
}

fn testBatchTopKCosineRetrievalAndTieBreaking()! {
    // 5 docs in 8D space:
    // doc 0: orthogonal [0, 1, 0, ...] -> cos = 0.0
    // doc 1: identical to query -> cos = 1.0 (tie with doc 4, doc 1 wins by smaller id)
    // doc 2: opposite to query -> cos = -1.0
    // doc 3: zero vector -> cos = 0.0
    // doc 4: positively scaled query (2x) -> cos = 1.0
    const query: [8]f32 = [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0];
    const corpus: [40]f32 = [
         0.0, 1.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0, // doc 0: orthogonal
         1.0, 0.0, 0.0, 0.0,  1.0, 0.0, 0.0, 0.0, // doc 1: cos = 1.0
        -1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, // doc 2: cos = -1.0
         0.0, 0.0, 0.0, 0.0,  0.0, 0.0, 0.0, 0.0, // doc 3: zero vector -> 0.0
         2.0, 0.0, 0.0, 0.0,  2.0, 0.0, 0.0, 0.0, // doc 4: cos = 1.0
    ];

    let ids: [3]usize = [0, 0, 0];
    let scores: [3]f32 = [0.0, 0.0, 0.0];
    let ids_s: []usize = ids[0..];
    let scores_s: []f32 = scores[0..];

    if (n := simdBatchTopKCosineF32(query[0..], corpus[0..], 5, 8, 3, ids_s, scores_s)) {
        expect_eq(n, 3);
        expect_eq(ids[0], 1);
        expect_eq(ids[1], 4);
        expect_eq(ids[2], 0); // tie between doc 0 (0.0) and doc 3 (0.0) -> doc 0 wins
        expect_eq(absF64((scores[0] as f64) - 1.0) < 0.0001, true);
        expect_eq(absF64((scores[1] as f64) - 1.0) < 0.0001, true);
        expect_eq(absF64((scores[2] as f64) - 0.0) < 0.0001, true);
    } else {
        expect_eq(true, false);
    }
}

fn testBatchTopKL2RetrievalAndTieBreaking()! {
    // 4 docs in 3D space vs query [1.0, 2.0, 3.0]:
    // doc 0: [4.0, 6.0, 3.0] -> dist = 5.0
    // doc 1: [1.0, 2.0, 3.0] -> dist = 0.0 (exact match)
    // doc 2: [1.0, 2.0, 6.0] -> dist = 3.0 (tie with doc 3 -> doc 2 wins)
    // doc 3: [1.0, 5.0, 3.0] -> dist = 3.0
    const query: [3]f32 = [1.0, 2.0, 3.0];
    const corpus: [12]f32 = [
        4.0, 6.0, 3.0,
        1.0, 2.0, 3.0,
        1.0, 2.0, 6.0,
        1.0, 5.0, 3.0,
    ];

    let ids: [6]usize = [0, 0, 0, 0, 0, 0];
    let dists: [6]f32 = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    let ids_s: []usize = ids[0..];
    let dists_s: []f32 = dists[0..];

    // Request k = 6 > n_docs = 4 -> safely returns 4 sorted ascending.
    if (n := cap.vector.simdBatchTopKL2F32(query[0..], corpus[0..], 4, 3, 6, ids_s, dists_s)) {
        expect_eq(n, 4);
        expect_eq(ids[0], 1);
        expect_eq(ids[1], 2);
        expect_eq(ids[2], 3);
        expect_eq(ids[3], 0);
        expect_eq(absF64((dists[0] as f64) - 0.0) < 0.0001, true);
        expect_eq(absF64((dists[1] as f64) - 3.0) < 0.0001, true);
        expect_eq(absF64((dists[2] as f64) - 3.0) < 0.0001, true);
        expect_eq(absF64((dists[3] as f64) - 5.0) < 0.0001, true);
    } else {
        expect_eq(true, false);
    }
}

fn testZeroVectorMismatchEmptyAndNanGuards()! {
    const z: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    const v4: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    const v3: [3]f32 = [1.0, 2.0, 3.0];
    const nan4: [4]f32 = [1.0, std.math.nan(f32), 3.0, 4.0];
    const v4_64: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const v3_64: [3]f64 = [1.0, 2.0, 3.0];

    // Zero-vector cosine safely returns 0.0 (no NaN / div-by-zero).
    if (cz := simdCosineF32(z[0..], v4[0..])) {
        expect_eq(cz, 0.0);
    } else {
        expect_eq(true, false);
    }

    // Empty and length-mismatched slices rejected.
    expect_eq(simdDotF32(v4[0..0], v4[0..0]) == null, true);
    expect_eq(simdDotF32(v4[0..], v3[0..]) == null, true);
    expect_eq(simdDotF64(v4_64[0..], v3_64[0..]) == null, true);
    expect_eq(simdL2SqF32(v4[0..], v3[0..]) == null, true);
    expect_eq(simdL2DistF32(v4[0..0], v4[0..0]) == null, true);
    expect_eq(simdCosineF32(v4[0..], v3[0..]) == null, true);

    // NaN propagation rejected.
    expect_eq(simdDotF32(nan4[0..], v4[0..]) == null, true);
    expect_eq(simdL2SqF32(nan4[0..], v4[0..]) == null, true);
    expect_eq(simdCosineF32(nan4[0..], v4[0..]) == null, true);

    // AXPY guards: length mismatch, empty, NaN/Inf alpha.
    let y4: [4]f32 = [1.0, 1.0, 1.0, 1.0];
    let y4_s: []f32 = y4[0..];
    expect_eq(simdAxpyInPlaceF32(1.0, v3[0..], y4_s), false);
    expect_eq(simdAxpyInPlaceF32(1.0, v4[0..0], y4[0..0]), false);
    expect_eq(simdAxpyInPlaceF32(std.math.nan(f32), v4[0..], y4_s), false);
    expect_eq(simdAxpyInPlaceF32(std.math.inf(f32), v4[0..], y4_s), false);

    // Batch Top-K guards: dim == 0, mismatched query len, truncated corpus/output buffers.
    let ids: [2]usize = [0, 0];
    let out_f: [2]f32 = [0.0, 0.0];
    let ids_s: []usize = ids[0..];
    let out_s: []f32 = out_f[0..];
    expect_eq(simdBatchTopKCosineF32(v4[0..], v4[0..], 1, 0, 1, ids_s, out_s) == null, true);
    expect_eq(simdBatchTopKCosineF32(v3[0..], v4[0..], 1, 4, 1, ids_s, out_s) == null, true);
    expect_eq(simdBatchTopKL2F32(v4[0..], v4[0..], 2, 4, 1, ids_s, out_s) == null, true);
    expect_eq(simdBatchTopKL2F32(v4[0..], v4[0..], 1, 4, 3, ids_s, out_s) == null, true);
    // n_docs == 0 or k == 0 returns 0 cleanly.
    expect_eq(simdBatchTopKCosineF32(v4[0..], v4[0..], 0, 4, 1, ids_s, out_s) ?? 99, 0);
    expect_eq(simdBatchTopKL2F32(v4[0..], v4[0..], 1, 4, 0, ids_s, out_s) ?? 99, 0);
}

fn main() {
    test("1. vector/simd_ops: multi-dim (1/7/8/19/128) @dot/@l2sq/cosine accuracy", () => {
        testMultiDimSimdDotL2CosineAccuracy()?;
    });

    test("2. vector/simd_ops: hardware @fma AXPY in-place single-rounding updates", () => {
        testFmaAxpyInPlaceUpdates()?;
    });

    test("3. vector/simd_ops: batch SIMD Top-K Cosine retrieval & deterministic tie-breaking", () => {
        testBatchTopKCosineRetrievalAndTieBreaking()?;
    });

    test("4. vector/simd_ops: batch SIMD Top-K L2 retrieval & k > n_docs clamping", () => {
        testBatchTopKL2RetrievalAndTieBreaking()?;
    });

    test("5. vector/simd_ops: zero-vector, mismatch, empty & NaN/Inf defensive guards", () => {
        testZeroVectorMismatchEmptyAndNanGuards()?;
    });
}
