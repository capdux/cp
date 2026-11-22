// Origin: Pure-CP Stdlib Test Suite (Embedding Lookup, Masked Pooling & Matryoshka MRL) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/embedding.cp
// Covers:
//   1. embeddingLookup exact row gather & out-of-range token_id rejection
//   2. Mean / Cls / LastToken / Max pooling accurately ignores trailing [PAD] (mask == 0) tokens
//   3. l2NormalizeInPlace returns exact original norm & matryoshkaProject produces unit L2 norm == 1.0
//   4. cosineSimilarityMatrix exact diagonal (+1.0), orthogonal (0.0), opposite (-1.0) & zero-vec handling
//   5. All-zero mask, zero vector, target_dim > full_vec.len, undersized buffers & NaN/Inf defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    PoolKind,
    embeddingLookup,
    poolHiddenStates,
    l2NormalizeInPlace,
    matryoshkaProject,
    cosineSimilarityMatrix,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testEmbeddingLookupAndOobRejection()! {
    // vocab_size = 4, dim = 3
    const table: [12]f64 = [
        0.0, 0.0, 0.0,   // id 0 (PAD)
        1.0, 2.0, 3.0,   // id 1
        -4.0, 5.0, -6.0, // id 2
        7.5, -8.5, 9.5,  // id 3
    ];
    const ids: [3]u32 = [3, 1, 2];
    let out_buf: [9]f64 = [0.0; 9];
    let out: []f64 = out_buf[0..];

    expect_eq(embeddingLookup(table[0..], 4, 3, ids[0..], out), true);
    expect_eq(approxF64(out_buf[0], 7.5, EPS_F64), true);
    expect_eq(approxF64(out_buf[1], -8.5, EPS_F64), true);
    expect_eq(approxF64(out_buf[2], 9.5, EPS_F64), true);
    expect_eq(approxF64(out_buf[3], 1.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[6], -4.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[8], -6.0, EPS_F64), true);

    // Out-of-range token_id == 4 must be rejected without corrupting out_buf
    const bad_ids: [2]u32 = [1, 4];
    out_buf[0] = 42.0;
    expect_eq(embeddingLookup(table[0..], 4, 3, bad_ids[0..], out), false);
    expect_eq(approxF64(out_buf[0], 42.0, EPS_F64), true);
}

fn testMaskedPoolingAllFourKinds()! {
    // seq_len = 4, dim = 3; last 2 positions are [PAD] with extreme values that MUST be ignored!
    const hidden: [12]f64 = [
        1.0,  6.0, -2.0,    // t=0 (CLS, active)
        5.0,  2.0,  4.0,    // t=1 (LastToken, active)
        999.0, 999.0, 999.0, // t=2 (PAD, masked out)
        -999.0, -999.0, -999.0, // t=3 (PAD, masked out)
    ];
    const mask: [4]u8 = [1, 1, 0, 0];
    let out_buf: [3]f64 = [0.0; 3];
    let out: []f64 = out_buf[0..];

    // Mean over active tokens [t=0, t=1] -> [3.0, 4.0, 1.0]
    expect_eq(poolHiddenStates(hidden[0..], mask[0..], 4, 3, PoolKind.Mean, out), true);
    expect_eq(approxF64(out_buf[0], 3.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[1], 4.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[2], 1.0, EPS_F64), true);

    // Cls -> t=0: [1.0, 6.0, -2.0]
    expect_eq(poolHiddenStates(hidden[0..], mask[0..], 4, 3, PoolKind.Cls, out), true);
    expect_eq(approxF64(out_buf[0], 1.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[1], 6.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[2], -2.0, EPS_F64), true);

    // LastToken -> t=1: [5.0, 2.0, 4.0]
    expect_eq(poolHiddenStates(hidden[0..], mask[0..], 4, 3, PoolKind.LastToken, out), true);
    expect_eq(approxF64(out_buf[0], 5.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[1], 2.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[2], 4.0, EPS_F64), true);

    // Max over active tokens [t=0, t=1] -> [5.0, 6.0, 4.0]
    expect_eq(poolHiddenStates(hidden[0..], mask[0..], 4, 3, PoolKind.Max, out), true);
    expect_eq(approxF64(out_buf[0], 5.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[1], 6.0, EPS_F64), true);
    expect_eq(approxF64(out_buf[2], 4.0, EPS_F64), true);

    // Empty mask defaults to all active (seq_len=2)
    const empty_mask: []const u8 = mask[0..0];
    expect_eq(poolHiddenStates(hidden[0..6], empty_mask, 2, 3, PoolKind.Mean, out), true);
    expect_eq(approxF64(out_buf[0], 3.0, EPS_F64), true);
}

fn testL2NormalizeAndMatryoshkaProject()! {
    // l2NormalizeInPlace on [3.0, 4.0] -> norm = 5.0, vec = [0.6, 0.8]
    let v_buf: [2]f64 = [3.0, 4.0];
    let v: []f64 = v_buf[0..];
    if (orig_norm := l2NormalizeInPlace(v, 0.0)) {
        expect_eq(approxF64(orig_norm, 5.0, EPS_F64), true);
        expect_eq(approxF64(v_buf[0], 0.6, EPS_F64), true);
        expect_eq(approxF64(v_buf[1], 0.8, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }

    // Matryoshka truncation from 6D -> 3D -> strict unit L2 norm == 1.0
    const full: [6]f64 = [3.0, 0.0, 4.0, 10.0, 20.0, 30.0];
    let mrl_buf: [3]f64 = [0.0; 3];
    let mrl: []f64 = mrl_buf[0..];
    expect_eq(matryoshkaProject(full[0..], 3, mrl), true);
    expect_eq(approxF64(mrl_buf[0], 0.6, EPS_F64), true);
    expect_eq(approxF64(mrl_buf[1], 0.0, EPS_F64), true);
    expect_eq(approxF64(mrl_buf[2], 0.8, EPS_F64), true);

    const norm_sq = mrl_buf[0] * mrl_buf[0] + mrl_buf[1] * mrl_buf[1] + mrl_buf[2] * mrl_buf[2];
    expect_eq(approxF64(std.math.sqrt(norm_sq), 1.0, EPS_F64), true);
}

fn testCosineSimilarityMatrix()! {
    // 2 queries x 3 docs (dim = 2):
    // q0 = [2, 0] (+x), q1 = [0, 3] (+y)
    // d0 = [5, 0] (+x), d1 = [-4, 0] (-x), d2 = [0, 0] (zero vec)
    const queries: [4]f64 = [2.0, 0.0, 0.0, 3.0];
    const docs: [6]f64 = [5.0, 0.0, -4.0, 0.0, 0.0, 0.0];
    let sim_buf: [6]f64 = [0.0; 6];
    let sim: []f64 = sim_buf[0..];

    expect_eq(cosineSimilarityMatrix(queries[0..], 2, docs[0..], 3, 2, sim), true);
    // q0 vs d0=+1, d1=-1, d2=0
    expect_eq(approxF64(sim_buf[0], 1.0, EPS_F64), true);
    expect_eq(approxF64(sim_buf[1], -1.0, EPS_F64), true);
    expect_eq(approxF64(sim_buf[2], 0.0, EPS_F64), true);
    // q1 vs d0=0 (orthogonal), d1=0 (orthogonal), d2=0
    expect_eq(approxF64(sim_buf[3], 0.0, EPS_F64), true);
    expect_eq(approxF64(sim_buf[4], 0.0, EPS_F64), true);
    expect_eq(approxF64(sim_buf[5], 0.0, EPS_F64), true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_val = std.math.nan(f64);
    const ok4: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const nan4: [4]f64 = [1.0, nan_val, 3.0, 4.0];
    const zero_mask: [2]u8 = [0, 0];
    const ok_mask: [2]u8 = [1, 1];

    let out2_buf: [2]f64 = [0.0; 2];
    let out2: []f64 = out2_buf[0..];
    let out1: []f64 = out2_buf[0..1];

    // All-zero mask, zero dims, undersized buffers, NaN in poolHiddenStates
    expect_eq(poolHiddenStates(ok4[0..], zero_mask[0..], 2, 2, PoolKind.Mean, out2), false);
    expect_eq(poolHiddenStates(ok4[0..], ok_mask[0..], 0, 2, PoolKind.Mean, out2), false);
    expect_eq(poolHiddenStates(ok4[0..], ok_mask[0..], 2, 2, PoolKind.Mean, out1), false);
    expect_eq(poolHiddenStates(nan4[0..], ok_mask[0..], 2, 2, PoolKind.Mean, out2), false);

    // Zero vector & negative eps & NaN in l2NormalizeInPlace
    let z_buf: [2]f64 = [0.0, 0.0];
    let z: []f64 = z_buf[0..];
    let nz_buf: [2]f64 = [1.0, nan_val];
    let nz: []f64 = nz_buf[0..];
    expect_eq(l2NormalizeInPlace(z, 0.0) == null, true);
    expect_eq(l2NormalizeInPlace(z_buf[0..0], 0.0) == null, true);
    expect_eq(l2NormalizeInPlace(nz, 0.0) == null, true);
    let ok2_buf: [2]f64 = [1.0, 0.0];
    let ok2: []f64 = ok2_buf[0..];
    expect_eq(l2NormalizeInPlace(ok2, -0.1) == null, true);

    // matryoshkaProject defenses: target_dim == 0, target_dim > full_vec.len, zero prefix, undersized out
    expect_eq(matryoshkaProject(ok4[0..], 0, out2), false);
    expect_eq(matryoshkaProject(ok4[0..], 5, out2), false);
    expect_eq(matryoshkaProject(ok4[0..], 2, out1), false);
    const z_prefix: [4]f64 = [0.0, 0.0, 5.0, 6.0];
    expect_eq(matryoshkaProject(z_prefix[0..], 2, out2), false);
    expect_eq(matryoshkaProject(nan4[0..], 2, out2), false);

    // cosineSimilarityMatrix defenses
    expect_eq(cosineSimilarityMatrix(ok4[0..], 0, ok4[0..], 1, 2, out2), false);
    expect_eq(cosineSimilarityMatrix(ok4[0..], 2, ok4[0..], 2, 2, out2), false); // out2 len=2 < 4
    expect_eq(cosineSimilarityMatrix(nan4[0..], 1, ok4[0..], 1, 2, out2), false);
}

fn main() {
    test("1. embedding: embeddingLookup exact row gather & OOB token_id rejection", () => {
        testEmbeddingLookupAndOobRejection()?;
    });
    test("2. embedding: Mean/Cls/LastToken/Max pooling ignores [PAD] (mask==0) tokens", () => {
        testMaskedPoolingAllFourKinds()?;
    });
    test("3. embedding: l2NormalizeInPlace & matryoshkaProject unit L2 norm == 1.0", () => {
        testL2NormalizeAndMatryoshkaProject()?;
    });
    test("4. embedding: cosineSimilarityMatrix exact +1/0/-1 & zero-vector handling", () => {
        testCosineSimilarityMatrix()?;
    });
    test("5. embedding: all-zero mask, zero vec, target_dim>len, buffer & NaN defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
