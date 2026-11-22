// Origin: Pure-CP Stdlib Test Suite (lib/vector/sq8Quant.cp) | Created: 2026-11-23
// Production Deep Verification Suite for 8-Bit Asymmetric Scalar Quantization (SQ8) Vector Index:
//   1. Streaming Sq8Calibrator, zero-variance dimension safe degeneration & 4x encode/decode accuracy
//   2. Quantile clipping (1%~99%) outlier resilience vs unclipped min/max calibration
//   3. Asymmetric (f32 vs u8) & symmetric (u8 vs u8) dot / L2^2 kernels + evaluateSq8Error fidelity
//   4. Stage-1 SQ8 Top-K retrieval for both Dot (desc) and L2^2 (asc) + 4x memory footprint stats
//   5. Stage-2 coarse-to-fine exact f32 rescoreTopK breaking sub-quantization-step ties
//   6. NaN/Inf, dimension mismatch, invalid codebook/config guards & cap.vector facade parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SQ8_MAX_DIM,
    SQ8_MAX_DOCS,
    Sq8Metric,
    Sq8Codebook,
    Sq8Calibrator,
    Sq8ErrorStats,
    Sq8SearchHit,
    Sq8IndexStats,
    sq8CalibrateBatch,
    encodeF32ToU8,
    decodeU8ToF32,
    sq8AsymDotF32,
    sq8AsymL2SqF32,
    sq8SymDotU8,
    evaluateSq8Error,
    Sq8VectorIndex,
} from "cap:vector";

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

test("SQ8: streaming calibrator, zero-variance dimension safety & 4x encode/decode parity") {
    let cal = Sq8Calibrator.init(4, 0.0) orelse {
        expect_true(false);
        return;
    };
    // Dim 2 is constant 3.5 across all samples (zero variance)
    const v0: [4]f32 = [-2.0, 0.0, 3.5, 10.0];
    const v1: [4]f32 = [2.0, 4.0, 3.5, 20.0];
    const v2: [4]f32 = [0.0, 2.0, 3.5, 15.0];

    expect_true(cal.observe(v0[0..]));
    expect_true(cal.observe(v1[0..]));
    expect_true(cal.observe(v2[0..]));
    expect_eq(cal.count(), 3 as u64);

    const cb = cal.finalize() orelse {
        expect_true(false);
        return;
    };
    expect_true(cb.isValid());
    expect_true(absF32(cb.min_vals[2] - 3.5) < 0.00001);
    expect_true(absF32(cb.scales[2]) < 0.000001);
    expect_true(absF32(cb.inv_scales[2]) < 0.000001);

    let q: [4]u8 = [0; 4];
    let q_s: []u8 = q[0..];
    let recon: [4]f32 = [0.0; 4];
    let recon_s: []f32 = recon[0..];

    expect_true(encodeF32ToU8(cb, v2[0..], *q_s));
    expect_eq(q[2], 0 as u8); // zero-variance dim encodes to 0
    expect_true(decodeU8ToF32(cb, q[0..], *recon_s));

    // Reconstructed values match within half a quantization step
    let d: usize = 0;
    while (d < 4) : (d += 1) {
        expect_true(absF32(recon[d] - v2[d]) <= cb.scales[d] * 0.51 + 0.0001);
    }
    expect_true(absF32(recon[2] - 3.5) < 0.00001);
}

test("SQ8: batch quantile clipping significantly reduces MSE under extreme outliers") {
    // 40 normal inlier vectors in [-1, 1] + 1 extreme outlier at +100.0
    const n_inliers: usize = 40;
    const n_total: usize = 41;
    const dim: usize = 4;
    let data: [164]f32 = [0.0; 164];

    let i: usize = 0;
    while (i < n_inliers) : (i += 1) {
        const t: f32 = ((i as f32) / 39.0) * 2.0 - 1.0; // [-1.0, 1.0]
        data[i * dim + 0] = t;
        data[i * dim + 1] = 0.0 - t;
        data[i * dim + 2] = t * 0.5;
        data[i * dim + 3] = 0.25 + t * 0.5;
    }
    // Single extreme outlier at index 40
    data[40 * dim + 0] = 100.0;
    data[40 * dim + 1] = -100.0;
    data[40 * dim + 2] = 50.0;
    data[40 * dim + 3] = 80.0;

    const cb_raw = sq8CalibrateBatch(data[0..], n_total, dim, 0.0) orelse {
        expect_true(false);
        return;
    };
    const cb_clip = sq8CalibrateBatch(data[0..], n_total, dim, 0.03) orelse {
        expect_true(false);
        return;
    };

    const err_raw = evaluateSq8Error(cb_raw, data[0 .. n_inliers * dim], n_inliers) orelse {
        expect_true(false);
        return;
    };
    const err_clip = evaluateSq8Error(cb_clip, data[0 .. n_inliers * dim], n_inliers) orelse {
        expect_true(false);
        return;
    };

    // Clipped codebook preserves precision for the 40 inliers far better than unclipped min/max
    expect_true(err_clip.mse < err_raw.mse * 0.1);
    expect_true(err_clip.cosine_fidelity > 0.995);
    expect_true(absF32(err_clip.compression_ratio - 4.0) < 0.0001);
}

test("SQ8: asymmetric & symmetric dot/L2^2 kernels match full-precision f32 closely") {
    const dim: usize = 8;
    const corpus: [24]f32 = [
        1.0, 0.2, -0.5, 0.8, 0.1, -0.3, 0.4, 0.9,
        -0.8, 0.9, 0.7, -0.4, 0.6, 0.2, -0.7, 0.1,
        0.3, -0.6, 0.2, 0.5, -0.9, 0.8, 0.3, -0.2,
    ];
    const cb = sq8CalibrateBatch(corpus[0..], 3, dim, 0.0) orelse {
        expect_true(false);
        return;
    };

    let q0: [8]u8 = [0; 8];
    let q1: [8]u8 = [0; 8];
    let q0_s: []u8 = q0[0..];
    let q1_s: []u8 = q1[0..];
    expect_true(encodeF32ToU8(cb, corpus[0..8], *q0_s));
    expect_true(encodeF32ToU8(cb, corpus[8..16], *q1_s));

    // Exact f32 reference dot(v0, v1) and l2_sq(v0, v1)
    let exact_dot: f32 = 0.0;
    let exact_l2: f32 = 0.0;
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        exact_dot += corpus[d] * corpus[8 + d];
        const diff: f32 = corpus[d] - corpus[8 + d];
        exact_l2 += diff * diff;
    }

    const asym_dot = sq8AsymDotF32(cb, corpus[0..8], q1[0..]) orelse { expect_true(false); return; };
    const asym_l2 = sq8AsymL2SqF32(cb, corpus[0..8], q1[0..]) orelse { expect_true(false); return; };
    const sym_dot = sq8SymDotU8(cb, q0[0..], q1[0..]) orelse { expect_true(false); return; };

    expect_true(absF32(asym_dot - exact_dot) < 0.03);
    expect_true(absF32(asym_l2 - exact_l2) < 0.05);
    expect_true(absF32(sym_dot - exact_dot) < 0.04);

    const fidelity = evaluateSq8Error(cb, corpus[0..], 3) orelse { expect_true(false); return; };
    expect_true(fidelity.cosine_fidelity > 0.999);
}

test("SQ8: Stage-1 Top-K search for Dot (desc) and L2^2 (asc) + 4x memory stats") {
    const dim: usize = 4;
    const vecs: [20]f32 = [
        1.0, 0.0, 0.0, 0.0, // doc 101
        0.8, 0.6, 0.0, 0.0, // doc 102
        0.0, 1.0, 0.0, 0.0, // doc 103
        -1.0, 0.0, 0.0, 0.0, // doc 104
        0.95, 0.1, 0.0, 0.0, // doc 105
    ];
    const cb = sq8CalibrateBatch(vecs[0..], 5, dim, 0.0) orelse {
        expect_true(false);
        return;
    };
    let idx = Sq8VectorIndex.init(cb) orelse {
        expect_true(false);
        return;
    };
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        expect_true(idx.addVector((101 + i) as u32, vecs[i * dim .. (i + 1) * dim]));
    }
    expect_eq(idx.len(), 5 as usize);
    const st = idx.stats();
    expect_eq(st.compressed_bytes, 20 as usize);
    expect_eq(st.uncompressed_bytes, 80 as usize);

    const query: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    let hits: [3]Sq8SearchHit = [Sq8SearchHit{}; 3];
    let hits_s: []Sq8SearchHit = hits[0..];

    // Dot descending: 101 (1.0) > 105 (0.95) > 102 (0.8)
    const n_dot = idx.searchTopK(query[0..], Sq8Metric.dot, 3, *hits_s) orelse {
        expect_true(false);
        return;
    };
    expect_eq(n_dot, 3 as usize);
    expect_eq(hits[0].doc_id, 101 as u32);
    expect_eq(hits[1].doc_id, 105 as u32);
    expect_eq(hits[2].doc_id, 102 as u32);
    expect_true(hits[0].score >= hits[1].score && hits[1].score >= hits[2].score);

    // L2^2 ascending: 101 (0.0) < 105 (0.0125) < 102 (0.40)
    const n_l2 = idx.searchTopK(query[0..], Sq8Metric.l2_sq, 3, *hits_s) orelse {
        expect_true(false);
        return;
    };
    expect_eq(n_l2, 3 as usize);
    expect_eq(hits[0].doc_id, 101 as u32);
    expect_eq(hits[1].doc_id, 105 as u32);
    expect_eq(hits[2].doc_id, 102 as u32);
    expect_true(hits[0].score <= hits[1].score && hits[1].score <= hits[2].score);
}

test("SQ8: Stage-2 coarse-to-fine rescoreTopK resolves sub-step precision ties") {
    const dim: usize = 4;
    // Wide dynamic range [-100, 100] -> quantization step ~0.784.
    // Docs 201 and 202 differ by only 0.05 (quantize to identical u8 codes!),
    // so Stage-2 f32 rescoring must break the tie accurately in favor of doc 202.
    const raw: [16]f32 = [
        -100.0, -100.0, -100.0, -100.0, // doc 200 (anchor min)
        50.00, 0.0, 0.0, 0.0,           // doc 201
        50.05, 0.0, 0.0, 0.0,           // doc 202 (closer to query 50.06)
        100.0, 100.0, 100.0, 100.0,     // doc 203 (anchor max)
    ];
    const cb = sq8CalibrateBatch(raw[0..], 4, dim, 0.0) orelse {
        expect_true(false);
        return;
    };
    let idx = Sq8VectorIndex.init(cb) orelse {
        expect_true(false);
        return;
    };
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        expect_true(idx.addVector((200 + i) as u32, raw[i * dim .. (i + 1) * dim]));
    }

    const query: [4]f32 = [50.06, 0.0, 0.0, 0.0];
    let hits: [2]Sq8SearchHit = [Sq8SearchHit{}; 2];
    let hits_s: []Sq8SearchHit = hits[0..];
    const n = idx.rescoreTopK(query[0..], raw[0..], Sq8Metric.l2_sq, 4, 2, *hits_s) orelse {
        expect_true(false);
        return;
    };
    expect_eq(n, 2 as usize);
    expect_eq(hits[0].doc_id, 202 as u32);
    expect_eq(hits[1].doc_id, 201 as u32);
    // Exact f32 L2^2 = (50.06 - 50.05)^2 = 0.0001
    expect_true(absF32(hits[0].score - 0.0001) < 0.00005);
}

test("SQ8: NaN/Inf, dimension mismatch, invalid codebook guards & cap.vector facade parity") {
    // Invalid calibrator configs
    expect_true(Sq8Calibrator.init(0, 0.0) == null);
    expect_true(Sq8Calibrator.init(SQ8_MAX_DIM + 1, 0.0) == null);
    expect_true(Sq8Calibrator.init(4, -0.01) == null);
    expect_true(Sq8Calibrator.init(4, 0.30) == null);
    expect_true(Sq8Calibrator.init(4, std.math.nan(f32)) == null);

    let cal = Sq8Calibrator.init(4, 0.0) orelse { expect_true(false); return; };
    expect_true(cal.finalize() == null); // 0 samples observed

    const nan_vec: [4]f32 = [1.0, std.math.nan(f32), 0.0, 0.0];
    const inf_vec: [4]f32 = [1.0, std.math.inf(f32), 0.0, 0.0];
    const short_vec: [2]f32 = [1.0, 2.0];
    expect_eq(cal.observe(nan_vec[0..]), false);
    expect_eq(cal.observe(inf_vec[0..]), false);
    expect_eq(cal.observe(short_vec[0..]), false);

    // Uncalibrated codebook rejected
    const bad_cb = Sq8Codebook{};
    expect_true(Sq8VectorIndex.init(bad_cb) == null);

    const good_vec: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    expect_true(cal.observe(good_vec[0..]));
    const cb = cal.finalize() orelse { expect_true(false); return; };
    let idx = Sq8VectorIndex.init(cb) orelse { expect_true(false); return; };

    expect_eq(idx.addVector(0, good_vec[0..]), false); // doc_id == 0 rejected
    expect_eq(idx.addVector(1, nan_vec[0..]), false);  // NaN rejected
    expect_eq(idx.addVector(1, short_vec[0..]), false); // wrong dim rejected

    expect_true(idx.addVector(1, good_vec[0..]));
    let hits: [2]Sq8SearchHit = [Sq8SearchHit{}; 2];
    let hits_s: []Sq8SearchHit = hits[0..];
    expect_true(idx.searchTopK(nan_vec[0..], Sq8Metric.dot, 2, *hits_s) == null);
    expect_true(idx.searchTopK(short_vec[0..], Sq8Metric.dot, 2, *hits_s) == null);

    // Clear resets count cleanly
    idx.clear();
    expect_eq(idx.len(), 0 as usize);

    // Verify cap.vector namespace facade parity
    expect_eq(cap.vector.SQ8_MAX_DIM, SQ8_MAX_DIM);
    expect_eq(cap.vector.SQ8_MAX_DOCS, SQ8_MAX_DOCS);
}
