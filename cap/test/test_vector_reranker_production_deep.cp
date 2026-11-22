// Origin: Pure-CP Stdlib Test Suite (RAG Reranker: MMR, ColBERT MaxSim & IR Metrics) | Created: 2026-11-22
// Deep Production Verification Suite for lib/vector/reranker.cp
// Covers:
//   1. mmrSelect relevance vs diversity tradeoff (lambda=1.0 picks near-duplicate; lambda=0.5 picks diverse doc)
//   2. colbertMaxSim exact token-level late-interaction match vs noisy/orthogonal documents
//   3. minMaxNormalizeInPlace & zScoreNormalizeInPlace exact values + zero-variance constant-input safety
//   4. ndcgAtK (ideal==1.0, reversed<1.0, duplicates, all-zero==0.0), mrrScore & recallAtK (with truth dedup)
//   5. Empty/zero/out-of-range/undersized/NaN boundary defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    mmrSelect,
    colbertMaxSim,
    minMaxNormalizeInPlace,
    zScoreNormalizeInPlace,
    ndcgAtK,
    mrrScore,
    recallAtK,
} from "../../lib/vector/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testMmrSelectRelevanceVsDiversity()! {
    // Doc 0 and Doc 1 are near-duplicates along +x with highest relevance (0.95, 0.94);
    // Doc 2 is orthogonal along +y with strong relevance (0.85); Doc 3 along +z (0.60).
    const q_sims: [4]f64 = [0.95, 0.94, 0.85, 0.60];
    const embs: [12]f64 = [
        1.0, 0.0, 0.0,
        0.9999, 0.014, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    ];
    let out_buf: [3]usize = [0; 3];
    let out: []usize = out_buf[0..];

    // lambda = 1.0 (pure relevance): picks [0, 1, 2]
    expect_eq(mmrSelect(q_sims[0..], embs[0..], 4, 3, 3, 1.0, out) ?? 0, 3);
    expect_eq(out_buf[0], 0);
    expect_eq(out_buf[1], 1);
    expect_eq(out_buf[2], 2);

    // lambda = 0.5 (balanced relevance & diversity): penalizes near-duplicate Doc 1 -> picks [0, 2, 3]
    expect_eq(mmrSelect(q_sims[0..], embs[0..], 4, 3, 3, 0.5, out) ?? 0, 3);
    expect_eq(out_buf[0], 0);
    expect_eq(out_buf[1], 2);
    expect_eq(out_buf[2], 3);

    // k > n_docs clamps to n_docs
    let out4_buf: [4]usize = [0; 4];
    let out4: []usize = out4_buf[0..];
    expect_eq(mmrSelect(q_sims[0..], embs[0..], 4, 3, 10, 0.7, out4) ?? 0, 4);
}

fn testColbertMaxSimExactVsNoise()! {
    // 2 query tokens: [1, 0, 0] and [0, 1, 0]
    const q_toks: [6]f64 = [
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
    ];

    // Exact match doc (plus an extra distractor token [0, 0, 1]): MaxSim == 1.0 + 1.0 == 2.0
    const doc_match: [9]f64 = [
        0.0, 0.0, 1.0,
        0.0, 1.0, 0.0,
        1.0, 0.0, 0.0,
    ];
    // Noisy doc: orthogonal to q[1], partial match to q[0]
    const doc_noisy: [6]f64 = [
        0.6, 0.0, 0.8,
        0.0, 0.0, 1.0,
    ];

    if (s_match := colbertMaxSim(q_toks[0..], 2, doc_match[0..], 3, 3)) {
        if (s_noisy := colbertMaxSim(q_toks[0..], 2, doc_noisy[0..], 2, 3)) {
            expect_eq(approxF64(s_match, 2.0, EPS_F64), true);
            expect_eq(approxF64(s_noisy, 0.6, EPS_F64), true);
            expect_eq(s_match > s_noisy, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testInPlaceScoreNormalizationsAndZeroVariance()! {
    // MinMax normal case: [10, 20, 30, 50] -> [0.0, 0.25, 0.5, 1.0]
    let mm_buf: [4]f64 = [10.0, 20.0, 30.0, 50.0];
    let mm: []f64 = mm_buf[0..];
    expect_eq(minMaxNormalizeInPlace(mm), true);
    expect_eq(approxF64(mm_buf[0], 0.0, EPS_F64), true);
    expect_eq(approxF64(mm_buf[1], 0.25, EPS_F64), true);
    expect_eq(approxF64(mm_buf[2], 0.5, EPS_F64), true);
    expect_eq(approxF64(mm_buf[3], 1.0, EPS_F64), true);

    // MinMax constant (zero range) -> all 0.0 without NaN
    let mm_const_buf: [3]f64 = [7.0, 7.0, 7.0];
    let mm_const: []f64 = mm_const_buf[0..];
    expect_eq(minMaxNormalizeInPlace(mm_const), true);
    expect_eq(approxF64(mm_const_buf[0], 0.0, EPS_F64), true);
    expect_eq(approxF64(mm_const_buf[2], 0.0, EPS_F64), true);

    // ZScore normal case: [2, 4, 4, 4, 5, 5, 7, 9] -> mean = 5.0, std = 2.0
    let zs_buf: [8]f64 = [2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0];
    let zs: []f64 = zs_buf[0..];
    expect_eq(zScoreNormalizeInPlace(zs), true);
    expect_eq(approxF64(zs_buf[0], -1.5, EPS_F64), true);
    expect_eq(approxF64(zs_buf[4], 0.0, EPS_F64), true);
    expect_eq(approxF64(zs_buf[7], 2.0, EPS_F64), true);

    // ZScore constant (zero variance) -> all 0.0 without NaN
    let zs_const_buf: [3]f64 = [3.5, 3.5, 3.5];
    let zs_const: []f64 = zs_const_buf[0..];
    expect_eq(zScoreNormalizeInPlace(zs_const), true);
    expect_eq(approxF64(zs_const_buf[1], 0.0, EPS_F64), true);
}

fn testIrEvaluationMetrics()! {
    // 4a. NDCG@K: ideal sorted descending with duplicates -> exact 1.0
    const ideal_rels: [5]f64 = [3.0, 3.0, 2.0, 1.0, 0.0];
    expect_eq(approxF64(ndcgAtK(ideal_rels[0..], 5) ?? -1.0, 1.0, EPS_F64), true);
    expect_eq(approxF64(ndcgAtK(ideal_rels[0..], 3) ?? -1.0, 1.0, EPS_F64), true);

    // Suboptimal order [0, 3, 2, 1]: strictly < 1.0 and matches manual DCG/IDCG
    const sub_rels: [4]f64 = [0.0, 3.0, 2.0, 1.0];
    const dcg_sub = 7.0 / std.math.log2(3.0) + 3.0 / std.math.log2(4.0) + 1.0 / std.math.log2(5.0);
    const idcg_sub = 7.0 / std.math.log2(2.0) + 3.0 / std.math.log2(3.0) + 1.0 / std.math.log2(4.0);
    expect_eq(approxF64(ndcgAtK(sub_rels[0..], 4) ?? -1.0, dcg_sub / idcg_sub, EPS_F64), true);

    // All-zero relevances -> 0.0
    const zero_rels: [3]f64 = [0.0, 0.0, 0.0];
    expect_eq(approxF64(ndcgAtK(zero_rels[0..], 3) ?? -1.0, 0.0, EPS_F64), true);

    // 4b. MRR
    const ranked: [5]u32 = [10, 20, 30, 40, 50];
    const truth_hit3: [2]u32 = [30, 50];
    const truth_miss: [2]u32 = [99, 100];
    expect_eq(approxF64(mrrScore(ranked[0..], truth_hit3[0..]) ?? -1.0, 1.0 / 3.0, EPS_F64), true);
    expect_eq(approxF64(mrrScore(ranked[0..], truth_miss[0..]) ?? -1.0, 0.0, EPS_F64), true);

    // 4c. Recall@K (with duplicate ground-truth IDs deduplicated)
    const truth_dups: [4]u32 = [20, 40, 20, 99]; // 3 unique relevant docs: {20, 40, 99}
    expect_eq(approxF64(recallAtK(ranked[0..], truth_dups[0..], 2) ?? -1.0, 1.0 / 3.0, EPS_F64), true);
    expect_eq(approxF64(recallAtK(ranked[0..], truth_dups[0..], 4) ?? -1.0, 2.0 / 3.0, EPS_F64), true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_val = std.math.nan(f64);
    const ok2: [2]f64 = [1.0, 0.5];
    const nan2: [2]f64 = [1.0, nan_val];
    const ok4: [4]f64 = [1.0, 0.0, 0.0, 1.0];
    let out2_buf: [2]usize = [0; 2];
    let out2: []usize = out2_buf[0..];
    let out1: []usize = out2_buf[0..1];

    // mmrSelect defenses
    expect_eq(mmrSelect(ok2[0..], ok4[0..], 0, 2, 1, 0.5, out2) == null, true);
    expect_eq(mmrSelect(ok2[0..], ok4[0..], 2, 2, 0, 0.5, out2) == null, true);
    expect_eq(mmrSelect(ok2[0..], ok4[0..], 2, 2, 2, 1.5, out2) == null, true);
    expect_eq(mmrSelect(ok2[0..], ok4[0..], 2, 2, 2, -0.1, out2) == null, true);
    expect_eq(mmrSelect(ok2[0..], ok4[0..], 2, 2, 2, 0.5, out1) == null, true);
    expect_eq(mmrSelect(nan2[0..], ok4[0..], 2, 2, 1, 0.5, out2) == null, true);

    // colbertMaxSim defenses
    expect_eq(colbertMaxSim(ok4[0..], 0, ok4[0..], 2, 2) == null, true);
    expect_eq(colbertMaxSim(ok4[0..], 2, ok4[0..], 0, 2) == null, true);
    expect_eq(colbertMaxSim(ok4[0..2], 2, ok4[0..], 2, 2) == null, true);
    expect_eq(colbertMaxSim(nan2[0..], 1, ok4[0..], 2, 2) == null, true);

    // Normalization defenses
    let empty_f: [1]f64 = [0.0];
    let nan_buf: [2]f64 = [1.0, nan_val];
    let nan_s: []f64 = nan_buf[0..];
    expect_eq(minMaxNormalizeInPlace(empty_f[0..0]), false);
    expect_eq(minMaxNormalizeInPlace(nan_s), false);
    expect_eq(zScoreNormalizeInPlace(empty_f[0..0]), false);
    expect_eq(zScoreNormalizeInPlace(nan_s), false);

    // IR metrics defenses
    const neg_rels: [2]f64 = [1.0, -0.5];
    expect_eq(ndcgAtK(ok2[0..0], 2) == null, true);
    expect_eq(ndcgAtK(ok2[0..], 0) == null, true);
    expect_eq(ndcgAtK(neg_rels[0..], 2) == null, true);
    expect_eq(ndcgAtK(nan2[0..], 2) == null, true);

    const ids: [2]u32 = [1, 2];
    expect_eq(mrrScore(ids[0..0], ids[0..]) == null, true);
    expect_eq(mrrScore(ids[0..], ids[0..0]) == null, true);
    expect_eq(recallAtK(ids[0..], ids[0..], 0) == null, true);
    expect_eq(recallAtK(ids[0..], ids[0..0], 2) == null, true);
}

fn main() {
    test("1. reranker: mmrSelect relevance vs diversity tradeoff (lambda=1.0 vs 0.5)", () => {
        testMmrSelectRelevanceVsDiversity()?;
    });
    test("2. reranker: colbertMaxSim exact token-level late-interaction vs noisy doc", () => {
        testColbertMaxSimExactVsNoise()?;
    });
    test("3. reranker: minMaxNormalizeInPlace & zScoreNormalizeInPlace + zero-variance safety", () => {
        testInPlaceScoreNormalizationsAndZeroVariance()?;
    });
    test("4. reranker: ndcgAtK (ideal/suboptimal/dups/zero), mrrScore & recallAtK dedup", () => {
        testIrEvaluationMetrics()?;
    });
    test("5. reranker: empty, zero, invalid lambda, neg rel & NaN boundary defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
