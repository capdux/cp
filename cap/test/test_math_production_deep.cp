// Source: cp/cap/test/test_math_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:math & cap.math

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Vec2,
    Vec3,
    Vec4,
    Mat4,
    Stats,
    pearsonCorrelation,
    linearRegression,
    dotF32,
    l2NormF32,
    normalizeL2,
    cosineSimilarityF32,
    euclideanDistanceF32,
    manhattanDistanceF32,
    softmaxInPlace,
    argMaxF32,
    topKIndices,
    quantizeSymmetricI8,
    dequantizeSymmetricI8,
    dotQuantizedI8,
} from "cap:math";

const EPS_F32: f32 = 0.00001;
const EPS_F32_COARSE: f32 = 0.0001;
const EPS_F64: f64 = 0.0000000001;

fn approxF32(a: f32, b: f32, eps: f32): bool {
    return @abs(a - b) <= eps;
}

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testVecAndMat4(): !void {
    // 1. Vec2 operations
    const v2a = Vec2.init(3.0, 4.0);
    const v2b = Vec2.init(1.0, -2.0);
    expect_eq(approxF32(v2a.length(), 5.0, EPS_F32), true);
    expect_eq(v2a.add(v2b).approxEql(Vec2.init(4.0, 2.0), EPS_F32), true);
    expect_eq(v2a.sub(v2b).approxEql(Vec2.init(2.0, 6.0), EPS_F32), true);
    expect_eq(v2a.scale(2.0).approxEql(Vec2.init(6.0, 8.0), EPS_F32), true);
    expect_eq(approxF32(v2a.dot(v2b), -5.0, EPS_F32), true);
    expect_eq(approxF32(v2a.normalize().length(), 1.0, EPS_F32), true);
    expect_eq(v2a.lerp(v2b, 0.5).approxEql(Vec2.init(2.0, 1.0), EPS_F32), true);
    expect_eq(approxF32(Vec2.init(0.0, 0.0).distance(Vec2.init(3.0, 4.0)), 5.0, EPS_F32), true);

    // 2. Vec3 cross product orthogonality & operations
    const ex = Vec3.init(1.0, 0.0, 0.0);
    const ey = Vec3.init(0.0, 1.0, 0.0);
    const ez = ex.cross(ey);
    expect_eq(ez.approxEql(Vec3.init(0.0, 0.0, 1.0), EPS_F32), true);
    expect_eq(approxF32(ez.dot(ex), 0.0, EPS_F32), true);
    expect_eq(approxF32(ez.dot(ey), 0.0, EPS_F32), true);

    const v3a = Vec3.init(2.0, -3.0, 5.0);
    const v3b = Vec3.init(-1.0, 4.0, 2.0);
    const v3c = v3a.cross(v3b);
    expect_eq(approxF32(v3c.dot(v3a), 0.0, EPS_F32), true);
    expect_eq(approxF32(v3c.dot(v3b), 0.0, EPS_F32), true);
    expect_eq(v3a.cross(v3b).approxEql(v3b.cross(v3a).scale(-1.0), EPS_F32), true);
    expect_eq(approxF32(v3a.normalize().length(), 1.0, EPS_F32), true);
    expect_eq(approxF32(Vec3.init(1.0, 2.0, 3.0).distance(Vec3.init(4.0, 6.0, 3.0)), 5.0, EPS_F32), true);

    // 3. Vec4 operations
    const v4a = Vec4.init(1.0, 2.0, 3.0, 4.0);
    const v4b = Vec4.init(5.0, 6.0, 7.0, 8.0);
    expect_eq(approxF32(v4a.dot(v4b), 70.0, EPS_F32), true);
    expect_eq(approxF32(Vec4.init(1.0, 1.0, 1.0, 1.0).length(), 2.0, EPS_F32), true);
    expect_eq(approxF32(v4a.normalize().length(), 1.0, EPS_F32), true);
    expect_eq(v4a.lerp(v4b, 0.25).approxEql(Vec4.init(2.0, 3.0, 4.0, 5.0), EPS_F32), true);
    expect_eq(approxF32(v4a.distance(v4b), 8.0, EPS_F32), true);

    // 4. Mat4 column-major transforms: T * R_z(pi/2) * S * p
    const pi_half: f32 = @as(f32, std.math.pi) * 0.5;
    const t_mat = Mat4.translation(10.0, 20.0, -5.0);
    const r_mat = Mat4.rotationZ(pi_half);
    const s_mat = Mat4.scaling(2.0, 3.0, 4.0);
    const m = t_mat.mul(r_mat).mul(s_mat);

    // Point (1, 0, 1, 1) -> scale (2, 0, 4, 1) -> rotZ(90deg) (0, 2, 4, 1) -> translate (10, 22, -1, 1)
    const p = Vec4.init(1.0, 0.0, 1.0, 1.0);
    const out = m.mulVec4(p);
    expect_eq(out.approxEql(Vec4.init(10.0, 22.0, -1.0, 1.0), EPS_F32_COARSE), true);

    // Identity & transpose invariants
    const id = Mat4.identity();
    expect_eq(m.mul(id).approxEql(m, EPS_F32), true);
    expect_eq(m.transpose().transpose().approxEql(m, EPS_F32), true);
    // Rotation matrix transpose == inverse: R^T * R == I
    expect_eq(r_mat.transpose().mul(r_mat).approxEql(id, EPS_F32), true);
}

fn testStatsAndRegression(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Descriptive statistics on classic dataset [2, 4, 4, 4, 5, 5, 7, 9]
    // Unsorted input to verify sorting inside summarize:
    const sample: [8]f64 = [9.0, 2.0, 5.0, 4.0, 7.0, 4.0, 5.0, 4.0];
    const summary = Stats.summarize(alloc, &sample)?;
    expect_eq(summary.count, 8);
    expect_eq(approxF64(summary.min, 2.0, EPS_F64), true);
    expect_eq(approxF64(summary.max, 9.0, EPS_F64), true);
    expect_eq(approxF64(summary.sum, 40.0, EPS_F64), true);
    expect_eq(approxF64(summary.mean, 5.0, EPS_F64), true);
    expect_eq(approxF64(summary.variance, 4.0, EPS_F64), true);
    expect_eq(approxF64(summary.stddev, 2.0, EPS_F64), true);
    expect_eq(approxF64(summary.median, 4.5, EPS_F64), true);

    // 2. Quantiles p95 & p99 on 0..=100 (101 points)
    let seq: [101]f64 = undefined;
    for (i of 0..101) {
        seq[i] = (100 - i) as f64;
    }
    const q_summary = Stats.summarize(alloc, &seq)?;
    expect_eq(q_summary.count, 101);
    expect_eq(approxF64(q_summary.median, 50.0, EPS_F64), true);
    expect_eq(approxF64(q_summary.p95, 95.0, EPS_F64), true);
    expect_eq(approxF64(q_summary.p99, 99.0, EPS_F64), true);

    // 3. Pearson correlation & linear regression (exact line y = 2.5x - 3.0)
    const x: [5]f64 = [1.0, 2.0, 3.0, 4.0, 5.0];
    const y: [5]f64 = [-0.5, 2.0, 4.5, 7.0, 9.5];
    const r = pearsonCorrelation(&x, &y)?;
    expect_eq(approxF64(r, 1.0, EPS_F64), true);

    const reg = linearRegression(&x, &y)?;
    expect_eq(approxF64(reg.slope, 2.5, EPS_F64), true);
    expect_eq(approxF64(reg.intercept, -3.0, EPS_F64), true);
    expect_eq(approxF64(reg.r_squared, 1.0, EPS_F64), true);
    expect_eq(approxF64(reg.predict(10.0), 22.0, EPS_F64), true);

    // 4. Negative correlation
    const y_neg: [5]f64 = [10.0, 8.0, 6.0, 4.0, 2.0];
    const r_neg = Stats.pearsonCorrelation(&x, &y_neg)?;
    expect_eq(approxF64(r_neg, -1.0, EPS_F64), true);
}

fn testEmbeddingDistancesAndNormalization(): !void {
    // Test with > 8 elements (19 elements = 2 SIMD blocks of 8 + 3 tail) to cover both SIMD & scalar tail
    let a: [19]f32 = undefined;
    let b: [19]f32 = undefined;
    for (i of 0..19) {
        a[i] = (i + 1) as f32;
        b[i] = ((i + 1) * 2) as f32;
    }

    // Collinear positive vectors => cosineSimilarity == 1.0
    expect_eq(approxF32(cosineSimilarityF32(&a, &b), 1.0, EPS_F32), true);

    // Opposite vector => cosineSimilarity == -1.0
    let neg_a: [19]f32 = undefined;
    for (i of 0..19) {
        neg_a[i] = 0.0 - a[i];
    }
    expect_eq(approxF32(cosineSimilarityF32(&a, &neg_a), -1.0, EPS_F32), true);

    // Orthogonal 16-dim vectors
    let u: [16]f32 = [0.0; 16];
    let v: [16]f32 = [0.0; 16];
    u[0] = 3.0;
    u[2] = 4.0;
    v[1] = 5.0;
    v[3] = 12.0;
    expect_eq(approxF32(dotF32(&u, &v), 0.0, EPS_F32), true);
    expect_eq(approxF32(cosineSimilarityF32(&u, &v), 0.0, EPS_F32), true);
    expect_eq(approxF32(l2NormF32(&u), 5.0, EPS_F32), true);
    expect_eq(approxF32(l2NormF32(&v), 13.0, EPS_F32), true);
    expect_eq(approxF32(manhattanDistanceF32(&u, &v), 24.0, EPS_F32), true);

    // Euclidean distance (3-4-5 triangle across SIMD boundary)
    let p1: [11]f32 = [0.0; 11];
    let p2: [11]f32 = [0.0; 11];
    p1[0] = 1.0;
    p2[0] = 4.0;
    p1[10] = 2.0;
    p2[10] = 6.0;
    expect_eq(approxF32(euclideanDistanceF32(&p1, &p2), 5.0, EPS_F32), true);
    expect_eq(approxF32(manhattanDistanceF32(&p1, &p2), 7.0, EPS_F32), true);

    // In-place L2 normalization
    normalizeL2(&a);
    expect_eq(approxF32(l2NormF32(&a), 1.0, EPS_F32), true);
    expect_eq(approxF32(cosineSimilarityF32(&a, &b), 1.0, EPS_F32), true);
}

fn testSoftmaxTopKAndQuantization(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Numerically stable softmax with large logits (10000.0 + offset would overflow naive exp!)
    let logits: [4]f32 = [10000.0, 10001.0, 10002.0, 10000.0];
    softmaxInPlace(&logits);
    let prob_sum: f32 = 0.0;
    for (i of 0..4) {
        const p = logits[i];
        expect_eq(p > 0.0 && p < 1.0, true);
        prob_sum += p;
    }
    expect_eq(approxF32(prob_sum, 1.0, EPS_F32), true);
    expect_eq(argMaxF32(&logits).?, 2);
    expect_eq(logits[0] < logits[1] && logits[1] < logits[2], true);

    // 2. argMaxF32 & topKIndices
    const scores: [7]f32 = [0.15, 0.92, 0.44, 0.99, 0.71, 0.92, 0.05];
    expect_eq(argMaxF32(&scores).?, 3);
    const empty_scores: []const f32 = [];
    expect_eq(argMaxF32(empty_scores) == null, true);

    const top3 = topKIndices(alloc, &scores, 3)?;
    defer alloc.free(top3);
    expect_eq(top3.len, 3);
    expect_eq(top3[0], 3); // 0.99
    expect_eq(top3[1], 1); // 0.92 (smaller index tie-break vs 5)
    expect_eq(top3[2], 5); // 0.92

    // 3. Symmetric Int8 quantization round-trip precision & quantized dot product
    let emb: [24]f32 = undefined;
    let query: [24]f32 = undefined;
    for (i of 0..24) {
        const fi = i as f32;
        emb[i] = @sin(fi * 0.37) * 2.5;
        query[i] = @cos(fi * 0.21) * 1.8;
    }

    let q_emb = quantizeSymmetricI8(alloc, &emb)?;
    defer q_emb.deinit(alloc);
    let q_query = quantizeSymmetricI8(alloc, &query)?;
    defer q_query.deinit(alloc);

    const deq_emb = dequantizeSymmetricI8(alloc, q_emb)?;
    defer alloc.free(deq_emb);

    // Max quantization error bounded by scale / 2 + small float tolerance
    const max_expected_err = q_emb.scale * 0.55 + EPS_F32;
    for (i of 0..24) {
        expect_eq(approxF32(deq_emb[i], emb[i], max_expected_err), true);
    }

    // Cosine similarity after Int8 quantization remains > 0.999
    expect_eq(cosineSimilarityF32(&emb, deq_emb) > 0.999, true);

    // Quantized dot product closely approximates f32 dot product
    const exact_dot = dotF32(&emb, &query);
    const q_dot = dotQuantizedI8(q_emb, q_query);
    expect_eq(approxF32(q_dot, exact_dot, 0.35), true);
}

fn testCapMathLazyMount(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Vec3 & Mat4 via cap.math
    const v1 = cap.math.Vec3.init(1.0, 2.0, 3.0);
    const v2 = cap.math.Vec3.init(4.0, 5.0, 6.0);
    const cross = v1.cross(v2);
    expect_eq(approxF32(cross.dot(v1), 0.0, EPS_F32), true);
    expect_eq(approxF32(cross.dot(v2), 0.0, EPS_F32), true);

    const t = cap.math.Mat4.translation(5.0, -3.0, 2.0);
    const pt = cap.math.Vec4.init(1.0, 2.0, 3.0, 1.0);
    expect_eq(t.mulVec4(pt).approxEql(cap.math.Vec4.init(6.0, -1.0, 5.0, 1.0), EPS_F32), true);

    // 2. Stats via cap.math
    const data: [4]f64 = [10.0, 20.0, 30.0, 40.0];
    const st = cap.math.Stats.summarize(alloc, &data)?;
    expect_eq(approxF64(st.mean, 25.0, EPS_F64), true);
    expect_eq(approxF64(st.median, 25.0, EPS_F64), true);

    // 3. Embedding retrieval via cap.math
    const a: [4]f32 = [1.0, 0.0, 1.0, 0.0];
    const b: [4]f32 = [2.0, 0.0, 2.0, 0.0];
    expect_eq(approxF32(cap.math.cosineSimilarityF32(&a, &b), 1.0, EPS_F32), true);

    const idx = cap.math.topKIndices(alloc, &a, 2)?;
    defer alloc.free(idx);
    expect_eq(idx.len, 2);
    expect_eq(idx[0], 0);
    expect_eq(idx[1], 2);
}

fn main() {
    test("1. Vec2/Vec3/Vec4 & Mat4 Transforms + Cross Product Orthogonality", () => {
        testVecAndMat4()?;
    });
    test("2. Stats Summary, Median/Quantiles, Pearson Correlation & Linear Regression", () => {
        testStatsAndRegression()?;
    });
    test("3. SIMD Embedding Cosine/Euclidean/Manhattan Distances & L2 Normalization", () => {
        testEmbeddingDistancesAndNormalization()?;
    });
    test("4. Numerically Stable Softmax, Top-K Indices & Symmetric Int8 Quantization", () => {
        testSoftmaxTopKAndQuantization()?;
    });
    test("5. import cap from \"cap\"; cap.math Automatic Dynamic Lazy Mounting", () => {
        testCapMathLazyMount()?;
    });
}
