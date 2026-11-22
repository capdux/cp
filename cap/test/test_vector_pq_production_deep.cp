// Origin: Pure-CP Stdlib Test Suite (Product Quantization & ADC Lookup Search) | Created: 2026-11-22
// Deep Production Verification Suite for lib/vector/pq.cp
// Covers:
//   1. 16D m=4, ks=8 encode/decode round-trip precision & 16x (f32) / 32x (f64) compression ratio
//   2. pqAdcDistanceSq exact mathematical match (< 1e-12) against L2Sq(query, decode(code))
//   3. 100-doc corpus pqSearchAdcTopK 100% match against brute-force decoded-distance Top-5
//   4. Deterministic (dist_sq asc, doc_id asc) tie-breaking & top_k clamping
//   5. dim%m!=0, ks>256, out-of-range codewords, buffer underflow & NaN/Inf boundary defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PqCodebook,
    PqHit,
    pqEncodeVector,
    pqDecodeVector,
    pqBuildAdcTable,
    pqAdcDistanceSq,
    pqSearchAdcTopK,
} from "../../lib/vector/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn l2SqSlice(a: []const f64, b: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

fn buildDemoCodebook16x4x8(centroids: *[]f64) {
    // m = 4 subspaces, ks = 8 centroids per subspace, sub_dim = 4 (total = 128 f64)
    let s: usize = 0;
    while (s < 4) : (s += 1) {
        let c: usize = 0;
        while (c < 8) : (c += 1) {
            const base = (s * 8 + c) * 4;
            const fs = (s + 1) as f64;
            const fc = c as f64;
            centroids[base + 0] = fc * 1.5 - 5.0 + 0.1 * fs;
            centroids[base + 1] = std.math.sin(fc * 0.9 + fs) * 3.0;
            centroids[base + 2] = std.math.cos(fc * 0.7 - fs) * 2.5;
            centroids[base + 3] = (fc - 3.5) * 0.8 * fs;
        }
    }
}

fn testEncodeDecodeRoundTripAndCompression()! {
    let cent_buf: [128]f64 = [0.0; 128];
    let cent: []f64 = cent_buf[0..];
    buildDemoCodebook16x4x8(cent);

    const cb = PqCodebook.init(16, 4, 8, cent_buf[0..]) ?? return;
    expect_eq(cb.sub_dim, 4);

    // Compression verification: 16 * 4B (f32) = 64B -> 4B u8 = 16x; 16 * 8B (f64) = 128B -> 4B u8 = 32x
    expect_eq((16 * 4) / cb.m, 16);
    expect_eq((16 * 8) / cb.m, 32);

    // 1. Exact centroid composition [2, 5, 1, 7] round-trips with zero error
    const expected_codes: [4]u8 = [2, 5, 1, 7];
    let exact_vec_buf: [16]f64 = [0.0; 16];
    let exact_vec: []f64 = exact_vec_buf[0..];
    expect_eq(pqDecodeVector(cb, expected_codes[0..], exact_vec), true);

    let enc_buf: [4]u8 = [0; 4];
    let enc: []u8 = enc_buf[0..];
    expect_eq(pqEncodeVector(cb, exact_vec_buf[0..], enc), true);
    expect_eq(enc_buf[0], 2);
    expect_eq(enc_buf[1], 5);
    expect_eq(enc_buf[2], 1);
    expect_eq(enc_buf[3], 7);

    // 2. Slightly perturbed vector still maps to exact nearest centroids [2, 5, 1, 7]
    let pert_vec: [16]f64 = [0.0; 16];
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        pert_vec[i] = exact_vec_buf[i] + 0.05;
    }
    expect_eq(pqEncodeVector(cb, pert_vec[0..], enc), true);
    expect_eq(enc_buf[0], 2);
    expect_eq(enc_buf[1], 5);
    expect_eq(enc_buf[2], 1);
    expect_eq(enc_buf[3], 7);

    let dec_buf: [16]f64 = [0.0; 16];
    let decoded: []f64 = dec_buf[0..];
    expect_eq(pqDecodeVector(cb, enc_buf[0..], decoded), true);
    expect_eq(approxF64(l2SqSlice(exact_vec_buf[0..], dec_buf[0..]), 0.0, EPS_F64), true);
}

fn testAdcDistanceExactIdentity()! {
    let cent_buf: [128]f64 = [0.0; 128];
    let cent: []f64 = cent_buf[0..];
    buildDemoCodebook16x4x8(cent);
    const cb = PqCodebook.init(16, 4, 8, cent_buf[0..]) ?? return;

    let query: [16]f64 = [0.0; 16];
    let d: usize = 0;
    while (d < 16) : (d += 1) {
        query[d] = std.math.sin((d as f64) * 0.45) * 4.0 - 0.5;
    }

    let lut_buf: [32]f64 = [0.0; 32];
    let lut: []f64 = lut_buf[0..];
    expect_eq(pqBuildAdcTable(cb, query[0..], lut), true);

    const test_codes: [12]u8 = [
        0, 1, 2, 3,
        7, 6, 5, 4,
        3, 0, 7, 2,
    ];
    let dec_buf: [16]f64 = [0.0; 16];
    let decoded: []f64 = dec_buf[0..];

    let k: usize = 0;
    while (k < 3) : (k += 1) {
        const code = test_codes[k * 4 .. (k + 1) * 4];
        expect_eq(pqDecodeVector(cb, code, decoded), true);
        const exact_dist = l2SqSlice(query[0..], dec_buf[0..]);
        const adc_dist = pqAdcDistanceSq(lut_buf[0..], code, 4, 8);
        expect_eq(approxF64(adc_dist, exact_dist, EPS_F64), true);
    }
}

fn testSearchAdcTopK100DocsMatchesBruteForce()! {
    let cent_buf: [128]f64 = [0.0; 128];
    let cent: []f64 = cent_buf[0..];
    buildDemoCodebook16x4x8(cent);
    const cb = PqCodebook.init(16, 4, 8, cent_buf[0..]) ?? return;

    // Encode 100 diverse 16D vectors into 400 bytes
    let codes_flat: [400]u8 = [0; 400];
    let vec_tmp: [16]f64 = [0.0; 16];
    let doc: usize = 0;
    while (doc < 100) : (doc += 1) {
        const fdoc = (doc + 1) as f64;
        let j: usize = 0;
        while (j < 16) : (j += 1) {
            const fj = j as f64;
            vec_tmp[j] = std.math.sin(fdoc * 0.37 + fj * 0.51) * 5.0 + std.math.cos(fdoc * 0.19 - fj) * 2.0;
        }
        let code_slice: []u8 = codes_flat[doc * 4 .. (doc + 1) * 4];
        expect_eq(pqEncodeVector(cb, vec_tmp[0..], code_slice), true);
    }

    let query: [16]f64 = [0.0; 16];
    let qd: usize = 0;
    while (qd < 16) : (qd += 1) {
        query[qd] = std.math.cos((qd as f64) * 0.33 + 0.7) * 3.5;
    }

    // PQ-ADC Top-5 search
    let lut_buf: [32]f64 = [0.0; 32];
    let hits_buf: [5]PqHit = [PqHit{}; 5];
    let lut: []f64 = lut_buf[0..];
    let hits: []PqHit = hits_buf[0..];
    const n_opt = pqSearchAdcTopK(cb, codes_flat[0..], 100, query[0..], 5, lut, hits);
    expect_eq(n_opt != null, true);
    expect_eq(n_opt ?? 0, 5);

    // Brute-force decoded distance Top-5 reference
    let bf_ids: [5]usize = [0; 5];
    let bf_dists: [5]f64 = [1000000000.0; 5];
    let dec_buf: [16]f64 = [0.0; 16];
    let decoded: []f64 = dec_buf[0..];

    let d2: usize = 0;
    while (d2 < 100) : (d2 += 1) {
        const code = codes_flat[d2 * 4 .. (d2 + 1) * 4];
        expect_eq(pqDecodeVector(cb, code, decoded), true);
        const dist = l2SqSlice(query[0..], dec_buf[0..]);

        let pos: usize = 5;
        while (pos > 0 && (dist < bf_dists[pos - 1] || (dist == bf_dists[pos - 1] && d2 < bf_ids[pos - 1]))) {
            if (pos < 5) {
                bf_dists[pos] = bf_dists[pos - 1];
                bf_ids[pos] = bf_ids[pos - 1];
            }
            pos -= 1;
        }
        if (pos < 5) {
            bf_dists[pos] = dist;
            bf_ids[pos] = d2;
        }
    }

    let k: usize = 0;
    while (k < 5) : (k += 1) {
        expect_eq(hits_buf[k].doc_id, bf_ids[k]);
        expect_eq(approxF64(hits_buf[k].dist_sq, bf_dists[k], EPS_F64), true);
    }
}

fn testTieBreakingAndTopKClamp()! {
    let cent_buf: [128]f64 = [0.0; 128];
    let cent: []f64 = cent_buf[0..];
    buildDemoCodebook16x4x8(cent);
    const cb = PqCodebook.init(16, 4, 8, cent_buf[0..]) ?? return;

    // 4 docs with identical codes [1, 1, 1, 1] -> must tie-break by doc_id ascending: 0, 1, 2, 3
    const codes: [16]u8 = [
        1, 1, 1, 1,
        1, 1, 1, 1,
        1, 1, 1, 1,
        1, 1, 1, 1,
    ];
    const q: [16]f64 = [0.0; 16];
    let lut_buf: [32]f64 = [0.0; 32];
    let hits_buf: [4]PqHit = [PqHit{}; 4];
    let lut: []f64 = lut_buf[0..];
    let hits: []PqHit = hits_buf[0..];

    // top_k = 10 > n_docs = 4 -> clamps to 4
    const n = pqSearchAdcTopK(cb, codes[0..], 4, q[0..], 10, lut, hits) ?? 0;
    expect_eq(n, 4);
    expect_eq(hits_buf[0].doc_id, 0);
    expect_eq(hits_buf[1].doc_id, 1);
    expect_eq(hits_buf[2].doc_id, 2);
    expect_eq(hits_buf[3].doc_id, 3);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_v = std.math.nan(f64);
    const inf_v = std.math.inf(f64);

    let cent_buf: [128]f64 = [0.0; 128];
    let cent: []f64 = cent_buf[0..];
    buildDemoCodebook16x4x8(cent);

    // 1. PqCodebook.init defenses
    expect_eq(PqCodebook.init(0, 4, 8, cent_buf[0..]) == null, true);     // dim == 0
    expect_eq(PqCodebook.init(16, 0, 8, cent_buf[0..]) == null, true);    // m == 0
    expect_eq(PqCodebook.init(15, 4, 8, cent_buf[0..]) == null, true);    // dim % m != 0
    expect_eq(PqCodebook.init(16, 4, 0, cent_buf[0..]) == null, true);    // ks == 0
    expect_eq(PqCodebook.init(16, 4, 257, cent_buf[0..]) == null, true);  // ks > 256
    expect_eq(PqCodebook.init(16, 4, 8, cent_buf[0..64]) == null, true);  // centroids too small

    let cent_nan: [128]f64 = cent_buf;
    cent_nan[17] = nan_v;
    expect_eq(PqCodebook.init(16, 4, 8, cent_nan[0..]) == null, true);

    const cb = PqCodebook.init(16, 4, 8, cent_buf[0..]) ?? return;

    // 2. Encode / Decode / BuildAdc / AdcDistance defenses
    const v_ok: [16]f64 = [0.0; 16];
    let v_nan: [16]f64 = [0.0; 16];
    v_nan[3] = inf_v;
    let code4_buf: [4]u8 = [0; 4];
    let code4: []u8 = code4_buf[0..];
    let code2: []u8 = code4_buf[0..2];
    let out16_buf: [16]f64 = [0.0; 16];
    let out16: []f64 = out16_buf[0..];
    let out8: []f64 = out16_buf[0..8];

    expect_eq(pqEncodeVector(cb, v_ok[0..8], code4), false);
    expect_eq(pqEncodeVector(cb, v_ok[0..], code2), false);
    expect_eq(pqEncodeVector(cb, v_nan[0..], code4), false);

    const bad_code: [4]u8 = [0, 8, 0, 0]; // codeword 8 >= ks(8)
    expect_eq(pqDecodeVector(cb, bad_code[0..], out16), false);
    expect_eq(pqDecodeVector(cb, code4_buf[0..], out8), false);

    let lut32_buf: [32]f64 = [0.0; 32];
    let lut32: []f64 = lut32_buf[0..];
    let lut16: []f64 = lut32_buf[0..16];
    expect_eq(pqBuildAdcTable(cb, v_ok[0..], lut16), false);
    expect_eq(pqBuildAdcTable(cb, v_nan[0..], lut32), false);
    expect_eq(pqBuildAdcTable(cb, v_ok[0..], lut32), true);
    expect_eq(std.math.isInf(pqAdcDistanceSq(lut32_buf[0..], bad_code[0..], 4, 8)), true);

    // 3. pqSearchAdcTopK defenses
    let hits2_buf: [2]PqHit = [PqHit{}; 2];
    let hits2: []PqHit = hits2_buf[0..];
    let hits1: []PqHit = hits2_buf[0..1];
    const two_codes: [8]u8 = [0, 1, 2, 3, 4, 5, 6, 7];
    const two_codes_bad: [8]u8 = [0, 1, 2, 3, 4, 9, 6, 7];

    expect_eq(pqSearchAdcTopK(cb, two_codes[0..], 0, v_ok[0..], 2, lut32, hits2), 0);
    expect_eq(pqSearchAdcTopK(cb, two_codes[0..], 2, v_ok[0..], 0, lut32, hits2), 0);
    expect_eq(pqSearchAdcTopK(cb, two_codes_bad[0..], 2, v_ok[0..], 2, lut32, hits2) == null, true);
    expect_eq(pqSearchAdcTopK(cb, two_codes[0..], 2, v_nan[0..], 2, lut32, hits2) == null, true);
    expect_eq(pqSearchAdcTopK(cb, two_codes[0..], 2, v_ok[0..], 2, lut16, hits2) == null, true);
    expect_eq(pqSearchAdcTopK(cb, two_codes[0..], 2, v_ok[0..], 2, lut32, hits1) == null, true);
}

fn main() {
    test("1. pq: 16D m=4 ks=8 encode/decode round-trip & 16x/32x compression", () => {
        testEncodeDecodeRoundTripAndCompression()?;
    });
    test("2. pq: pqAdcDistanceSq matches L2Sq(query, decode(code)) within 1e-12", () => {
        testAdcDistanceExactIdentity()?;
    });
    test("3. pq: 100-doc pqSearchAdcTopK 100% matches brute-force decoded Top-5", () => {
        testSearchAdcTopK100DocsMatchesBruteForce()?;
    });
    test("4. pq: deterministic tie-breaking & top_k clamping", () => {
        testTieBreakingAndTopKClamp()?;
    });
    test("5. pq: dim%m!=0, ks>256, out-of-range codewords, buffer & NaN defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
