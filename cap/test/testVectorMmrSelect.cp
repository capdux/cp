// Origin: Pure-CP Stdlib Test Suite (lib/vector/mmrSelect.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Incremental SIMD @dot/@l2sq MMR Diversity Reranker:
//   1. Lambda trade-off: lambda=1.0 (pure relevance) vs lambda=0.5 (balanced) vs lambda=0.0 (pure diversity) + list diversity metric
//   2. Hard redundancy suppression (`redundancy_threshold`) skipping near-duplicate embeddings
//   3. Multi-source quota enforcement (`max_per_source`) & `skipped_source_cap` accounting
//   4. Minimum relevance gate (`min_relevance`) & `skipped_low_relevance` accounting
//   5. Precomputed BM25/Cross-Encoder scoring (`selectMmrWithPrecomputedScores`) + `dot` & `neg_l2` metrics
//   6. Empty/invalid dimension/out-of-bounds offset guards & `cap.vector` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MMR_MAX_CANDIDATES,
    MmrMetric,
    MmrConfig,
    MmrCandidate,
    MmrSelectedItem,
    MmrStats,
    mmrPairSimilarity,
    selectMmr,
    selectMmrWithPrecomputedScores,
    computeListDiversity,
} from "cap:vector";

test("MmrSelect: lambda=1.0 pure relevance vs lambda=0.5 balanced vs lambda=0.0 pure diversity") {
    const query: [2]f32 = [1.0, 0.0];
    // 4 unit-ish candidates:
    //   doc 1: [1.0, 0.0]     (cos=1.00 to query)
    //   doc 2: [0.99, 0.141]  (cos=0.99 to query, ~0.99 to doc 1 -> near-duplicate!)
    //   doc 3: [0.707, 0.707] (cos=0.707 to query, 0.707 to doc 1 -> balanced)
    //   doc 4: [0.0, 1.0]     (cos=0.00 to query, 0.00 to doc 1 -> maximally orthogonal)
    const vecs: [8]f32 = [
        1.0, 0.0,
        0.99, 0.141,
        0.707, 0.707,
        0.0, 1.0,
    ];
    const cands: [4]MmrCandidate = [
        MmrCandidate{ doc_id: 1, source_id: 1, vec_offset: 0 },
        MmrCandidate{ doc_id: 2, source_id: 2, vec_offset: 2 },
        MmrCandidate{ doc_id: 3, source_id: 3, vec_offset: 4 },
        MmrCandidate{ doc_id: 4, source_id: 4, vec_offset: 6 },
    ];

    let out: [3]MmrSelectedItem = [MmrSelectedItem{}; 3];
    let out_s: []MmrSelectedItem = out[0..];
    let st = MmrStats{};

    // 1. lambda = 1.0 (pure relevance) -> picks [1, 2, 3]
    let cfg_rel = MmrConfig.defaults();
    cfg_rel.lambda = 1.0;
    cfg_rel.top_k = 3;
    const n_rel = selectMmr(query[0..], vecs[0..], 2, cands[0..], cfg_rel, *out_s, *st);
    expect_eq(n_rel, @as(usize, 3));
    expect_eq(out[0].doc_id, @as(u64, 1));
    expect_eq(out[1].doc_id, @as(u64, 2));
    expect_eq(out[2].doc_id, @as(u64, 3));
    const div_rel = computeListDiversity(vecs[0..], 2, out[0..n_rel], MmrMetric.cosine);

    // 2. lambda = 0.5 (balanced) -> picks doc 1 first, then doc 3 or doc 4 ahead of redundant doc 2
    let cfg_bal = MmrConfig.defaults();
    cfg_bal.lambda = 0.5;
    cfg_bal.top_k = 2;
    const n_bal = selectMmr(query[0..], vecs[0..], 2, cands[0..], cfg_bal, *out_s, *st);
    expect_eq(n_bal, @as(usize, 2));
    expect_eq(out[0].doc_id, @as(u64, 1));
    expect_true(out[1].doc_id != 2); // doc 2 penalized by high redundancy ~0.99
    const div_bal = computeListDiversity(vecs[0..], 2, out[0..n_bal], MmrMetric.cosine);
    expect_true(div_bal > div_rel);

    // 3. lambda = 0.0 (pure diversity after seed) -> picks doc 1 seed, then orthogonal doc 4
    let cfg_div = MmrConfig.defaults();
    cfg_div.lambda = 0.0;
    cfg_div.top_k = 2;
    const n_div = selectMmr(query[0..], vecs[0..], 2, cands[0..], cfg_div, *out_s, *st);
    expect_eq(n_div, @as(usize, 2));
    expect_eq(out[0].doc_id, @as(u64, 1));
    expect_eq(out[1].doc_id, @as(u64, 4));
    expect_true(computeListDiversity(vecs[0..], 2, out[0..n_div], MmrMetric.cosine) > 0.99);
}

test("MmrSelect: hard redundancy_threshold skips near-duplicates") {
    const query: [2]f32 = [1.0, 0.0];
    const vecs: [6]f32 = [
        1.0, 0.0,     // doc 10
        0.999, 0.044, // doc 11: cosine ~0.999 with doc 10 -> exceeds 0.95 threshold
        0.6, 0.8,     // doc 12: cosine 0.6 with doc 10 -> passes
    ];
    const cands: [3]MmrCandidate = [
        MmrCandidate{ doc_id: 10, source_id: 1, vec_offset: 0 },
        MmrCandidate{ doc_id: 11, source_id: 2, vec_offset: 2 },
        MmrCandidate{ doc_id: 12, source_id: 3, vec_offset: 4 },
    ];

    let cfg = MmrConfig.defaults();
    cfg.lambda = 0.9;
    cfg.top_k = 3;
    cfg.redundancy_threshold = 0.95;

    let out: [3]MmrSelectedItem = [MmrSelectedItem{}; 3];
    let out_s: []MmrSelectedItem = out[0..];
    let st = MmrStats{};
    const n = selectMmr(query[0..], vecs[0..], 2, cands[0..], cfg, *out_s, *st);

    expect_eq(n, @as(usize, 2));
    expect_eq(out[0].doc_id, @as(u64, 10));
    expect_eq(out[1].doc_id, @as(u64, 12));
    expect_eq(st.skipped_redundant, @as(usize, 1));
    expect_eq(st.selected_count, @as(usize, 2));
}

test("MmrSelect: max_per_source quota enforcement across multi-source documents") {
    const query: [2]f32 = [1.0, 0.0];
    const vecs: [8]f32 = [
        1.0, 0.0,   // doc 1, src 100
        0.95, 0.31, // doc 2, src 100
        0.90, 0.43, // doc 3, src 100 (should be capped!)
        0.50, 0.86, // doc 4, src 200 (must be selected once src 100 hits cap=1)
    ];
    const cands: [4]MmrCandidate = [
        MmrCandidate{ doc_id: 1, source_id: 100, vec_offset: 0 },
        MmrCandidate{ doc_id: 2, source_id: 100, vec_offset: 2 },
        MmrCandidate{ doc_id: 3, source_id: 100, vec_offset: 4 },
        MmrCandidate{ doc_id: 4, source_id: 200, vec_offset: 6 },
    ];

    let cfg = MmrConfig.defaults();
    cfg.lambda = 1.0;
    cfg.top_k = 3;
    cfg.max_per_source = 1;

    let out: [3]MmrSelectedItem = [MmrSelectedItem{}; 3];
    let out_s: []MmrSelectedItem = out[0..];
    let st = MmrStats{};
    const n = selectMmr(query[0..], vecs[0..], 2, cands[0..], cfg, *out_s, *st);

    expect_eq(n, @as(usize, 2));
    expect_eq(out[0].doc_id, @as(u64, 1));
    expect_eq(out[0].source_id, @as(u32, 100));
    expect_eq(out[1].doc_id, @as(u64, 4));
    expect_eq(out[1].source_id, @as(u32, 200));
    expect_eq(st.skipped_source_cap, @as(usize, 2));
}

test("MmrSelect: min_relevance floor filters low-relevance noise candidates") {
    const query: [2]f32 = [1.0, 0.0];
    const vecs: [6]f32 = [
        1.0, 0.0,  // cos = 1.0
        0.2, 0.98, // cos = 0.2 (< 0.5)
        0.0, 1.0,  // cos = 0.0 (< 0.5)
    ];
    const cands: [3]MmrCandidate = [
        MmrCandidate{ doc_id: 1, source_id: 1, vec_offset: 0 },
        MmrCandidate{ doc_id: 2, source_id: 2, vec_offset: 2 },
        MmrCandidate{ doc_id: 3, source_id: 3, vec_offset: 4 },
    ];

    let cfg = MmrConfig.defaults();
    cfg.top_k = 3;
    cfg.min_relevance = 0.5;

    let out: [3]MmrSelectedItem = [MmrSelectedItem{}; 3];
    let out_s: []MmrSelectedItem = out[0..];
    let st = MmrStats{};
    const n = selectMmr(query[0..], vecs[0..], 2, cands[0..], cfg, *out_s, *st);

    expect_eq(n, @as(usize, 1));
    expect_eq(out[0].doc_id, @as(u64, 1));
    expect_eq(st.skipped_low_relevance, @as(usize, 2));
}

test("MmrSelect: precomputed BM25/Cross-Encoder scores + dot & neg_l2 metrics") {
    const vecs: [6]f32 = [
        1.0, 0.0, // doc 100: base_score = 0.95
        1.0, 0.0, // doc 101: base_score = 0.90 (identical vector to doc 100!)
        0.0, 1.0, // doc 102: base_score = 0.80 (orthogonal to doc 100)
    ];
    const cands: [3]MmrCandidate = [
        MmrCandidate{ doc_id: 100, source_id: 1, base_score: 0.95, vec_offset: 0 },
        MmrCandidate{ doc_id: 101, source_id: 2, base_score: 0.90, vec_offset: 2 },
        MmrCandidate{ doc_id: 102, source_id: 3, base_score: 0.80, vec_offset: 4 },
    ];

    let cfg = MmrConfig.defaults();
    cfg.lambda = 0.5;
    cfg.top_k = 2;
    cfg.metric = MmrMetric.cosine;

    let out: [2]MmrSelectedItem = [MmrSelectedItem{}; 2];
    let out_s: []MmrSelectedItem = out[0..];
    let st = MmrStats{};
    const n = selectMmrWithPrecomputedScores(vecs[0..], 2, cands[0..], cfg, *out_s, *st);
    expect_eq(n, @as(usize, 2));
    expect_eq(out[0].doc_id, @as(u64, 100));
    expect_eq(out[1].doc_id, @as(u64, 102)); // doc 102 beats identical-vector doc 101

    // Verify native @dot and @l2sq pairwise kernels directly
    const a: [2]f32 = [3.0, 4.0];
    const b: [2]f32 = [3.0, 0.0];
    const dot_val = mmrPairSimilarity(a[0..], b[0..], MmrMetric.dot);
    expect_true(dot_val > 8.99 && dot_val < 9.01); // 3*3 + 4*0 = 9
    const neg_l2 = mmrPairSimilarity(a[0..], b[0..], MmrMetric.neg_l2);
    expect_true(neg_l2 < -15.99 && neg_l2 > -16.01); // -((0)^2 + (4)^2) = -16
}

test("MmrSelect: invalid config/dim/empty bounds guards & cap.vector facade parity") {
    const query: [2]f32 = [1.0, 0.0];
    const vecs: [2]f32 = [1.0, 0.0];
    const cands: [2]MmrCandidate = [
        MmrCandidate{ doc_id: 1, source_id: 1, vec_offset: 0 },
        MmrCandidate{ doc_id: 2, source_id: 2, vec_offset: 100 }, // out-of-bounds offset safely skipped
    ];

    let out: [2]MmrSelectedItem = [MmrSelectedItem{}; 2];
    let out_s: []MmrSelectedItem = out[0..];
    let st = MmrStats{};

    // Invalid lambda > 1.0 returns 0
    let bad_cfg = cap.vector.MmrConfig.defaults();
    bad_cfg.lambda = 1.5;
    expect_true(!bad_cfg.isValid());
    expect_eq(cap.vector.selectMmr(query[0..], vecs[0..], 2, cands[0..], bad_cfg, *out_s, *st), @as(usize, 0));

    // Mismatched dim returns 0
    expect_eq(cap.vector.selectMmr(query[0..], vecs[0..], 4, cands[0..], cap.vector.MmrConfig.defaults(), *out_s, *st), @as(usize, 0));

    // Valid call skips out-of-bounds candidate 2 and selects candidate 1 cleanly
    const n = cap.vector.selectMmr(query[0..], vecs[0..], 2, cands[0..], cap.vector.MmrConfig.defaults(), *out_s, *st);
    expect_eq(n, @as(usize, 1));
    expect_eq(out[0].doc_id, @as(u64, 1));
    expect_eq(st.candidates_evaluated, @as(usize, 1));
    expect_eq(MMR_MAX_CANDIDATES, @as(usize, 128));
}
