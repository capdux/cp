// Origin: Pure-CP Stdlib Test Suite (lib/vector/bm25f_index.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc BM25F Multi-Field Weighted Inverted Index:
//   1. High-weight title/symbol field match ranks strictly above body-only match
//   2. Per-field independent length normalization (b_f): concise title beats verbose diluted title
//   3. Unified k1 saturation over pseudo_tf prevents single-term stuffing from beating multi-term coverage
//   4. Rare term high IDF dominates common boilerplate term low IDF
//   5. remove_document evicts doc from search results & updates avg_field_lens accurately
//   6. Query term deduplication & deterministic ascending doc_id tie-breaking
//   7. Boundary guards: empty query, invalid config, duplicate doc_id & mismatched field_texts.len

import { test, expect_eq } from "cap:test";
import {
    BM25F_MAX_FIELDS,
    BM25F_MAX_DOCS,
    BM25F_MAX_POSTINGS,
    Bm25fFieldSpec,
    Bm25fConfig,
    Bm25fHit,
    Bm25fIndexStats,
    Bm25fIndex,
} from "cap:vector";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: High-Weight Title/Symbol Match Beats Body-Only Match
// ============================================================================
test("bm25f_index: symbol/title field boost strictly outranks body-only match", () => {
    expect_eq(BM25F_MAX_FIELDS, 4);
    expect_eq(BM25F_MAX_DOCS, 128);
    expect_eq(BM25F_MAX_POSTINGS, 2048);

    let idx = Bm25fIndex.init(Bm25fConfig.defaultsCodeSearch()) ?? return;

    // doc 10: hits "paged_kv" in body only (weight=1.0)
    const d10: [4][]const u8 = ["cache overview", "helper_fn", "uses paged_kv allocator", "storage"];
    // doc 20: hits "paged_kv" in symbol field (weight=4.0)
    const d20: [4][]const u8 = ["memory manager", "paged_kv", "block pool implementation", "runtime"];
    // doc 30: hits "paged_kv" in title field (weight=3.0)
    const d30: [4][]const u8 = ["paged_kv design", "init_pool", "block pool implementation", "runtime"];

    expect_eq(idx.add_document(10, d10[0..]), true);
    expect_eq(idx.add_document(20, d20[0..]), true);
    expect_eq(idx.add_document(30, d30[0..]), true);

    let hits: [3]Bm25fHit = [Bm25fHit{}; 3];
    let hits_s: []Bm25fHit = hits[0..];
    const n = idx.search_topk("PAGED_KV", 3, hits_s);
    expect_eq(n, 3);
    // Ranking: symbol (20) > title (30) > body (10)
    expect_eq(hits[0].doc_id, 20);
    expect_eq(hits[1].doc_id, 30);
    expect_eq(hits[2].doc_id, 10);
    expect_eq(hits[0].score_f32 > hits[1].score_f32, true);
    expect_eq(hits[1].score_f32 > hits[2].score_f32, true);
});

// ============================================================================
// Test 2: Per-Field Length Normalization (Concise Title Beats Verbose Title)
// ============================================================================
test("bm25f_index: per-field length normalization favors concise title over verbose title", () => {
    const cfg = Bm25fConfig{
        k1_f32: 1.2,
        field_count: 2,
        fields: [
            Bm25fFieldSpec{ weight_f32: 2.0, b_f32: 0.85 },
            Bm25fFieldSpec{ weight_f32: 1.0, b_f32: 0.75 },
            Bm25fFieldSpec{},
            Bm25fFieldSpec{},
        ],
    };
    let idx = Bm25fIndex.init(cfg) ?? return;

    // doc 1: concise 2-word title
    const d1: [2][]const u8 = ["dinic flow", "graph network solver"];
    // doc 2: verbose 10-word title diluting the exact same term
    const d2: [2][]const u8 = ["comprehensive guide to modern algorithms including dinic flow and many extra topics", "graph network solver"];

    expect_eq(idx.add_document(1, d1[0..]), true);
    expect_eq(idx.add_document(2, d2[0..]), true);

    let hits: [2]Bm25fHit = [Bm25fHit{}; 2];
    let hits_s: []Bm25fHit = hits[0..];
    const n = idx.search_topk("dinic", 2, hits_s);
    expect_eq(n, 2);
    expect_eq(hits[0].doc_id, 1);
    expect_eq(hits[1].doc_id, 2);
    expect_eq(hits[0].score_f32 > hits[1].score_f32, true);
});

// ============================================================================
// Test 3: Unified k1 Saturation Prevents Single-Term Stuffing
// ============================================================================
test("bm25f_index: unified k1 saturation prevents single-term stuffing from beating multi-term match", () => {
    const cfg = Bm25fConfig{
        k1_f32: 1.2,
        field_count: 2,
        fields: [
            Bm25fFieldSpec{ weight_f32: 2.0, b_f32: 0.0 },
            Bm25fFieldSpec{ weight_f32: 1.0, b_f32: 0.0 },
            Bm25fFieldSpec{},
            Bm25fFieldSpec{},
        ],
    };
    let idx = Bm25fIndex.init(cfg) ?? return;

    // doc 1: stuffs "simd" 6 times across both fields, misses "quant"
    const d1: [2][]const u8 = ["simd simd simd", "simd simd simd other"];
    // doc 2: mentions both "simd" and "quant" once each in title
    const d2: [2][]const u8 = ["simd quant", "kernel implementation"];
    // doc 3: background doc so IDF for both terms is meaningful
    const d3: [2][]const u8 = ["network socket", "tcp stream buffer"];

    expect_eq(idx.add_document(1, d1[0..]), true);
    expect_eq(idx.add_document(2, d2[0..]), true);
    expect_eq(idx.add_document(3, d3[0..]), true);

    let hits: [3]Bm25fHit = [Bm25fHit{}; 3];
    let hits_s: []Bm25fHit = hits[0..];
    const n = idx.search_topk("simd quant", 3, hits_s);
    expect_eq(n, 2);
    // Multi-term coverage (doc 2) beats single-term stuffing (doc 1) due to k1 saturation!
    expect_eq(hits[0].doc_id, 2);
    expect_eq(hits[0].matched_terms, 2);
    expect_eq(hits[1].doc_id, 1);
    expect_eq(hits[1].matched_terms, 1);
});

// ============================================================================
// Test 4: Rare Term High IDF Dominates Common Term Low IDF
// ============================================================================
test("bm25f_index: rare term high IDF dominates common term low IDF", () => {
    const cfg = Bm25fConfig{
        k1_f32: 1.2,
        field_count: 1,
        fields: [
            Bm25fFieldSpec{ weight_f32: 1.0, b_f32: 0.0 },
            Bm25fFieldSpec{},
            Bm25fFieldSpec{},
            Bm25fFieldSpec{},
        ],
    };
    let idx = Bm25fIndex.init(cfg) ?? return;

    // "common" appears in all 4 docs; "charikar" appears only in doc 4
    const d1: [1][]const u8 = ["common alpha"];
    const d2: [1][]const u8 = ["common beta"];
    const d3: [1][]const u8 = ["common gamma"];
    const d4: [1][]const u8 = ["charikar delta"];
    expect_eq(idx.add_document(1, d1[0..]), true);
    expect_eq(idx.add_document(2, d2[0..]), true);
    expect_eq(idx.add_document(3, d3[0..]), true);
    expect_eq(idx.add_document(4, d4[0..]), true);

    let hits: [4]Bm25fHit = [Bm25fHit{}; 4];
    let hits_s: []Bm25fHit = hits[0..];
    const n = idx.search_topk("common charikar", 4, hits_s);
    expect_eq(n, 4);
    expect_eq(hits[0].doc_id, 4); // rare term "charikar" strictly outranks "common"
    expect_eq(hits[0].score_f32 > hits[1].score_f32, true);
});

// ============================================================================
// Test 5: remove_document Evicts From Search & Updates avg_field_lens
// ============================================================================
test("bm25f_index: remove_document excludes doc & updates avg_field_lens accurately", () => {
    const cfg = Bm25fConfig{
        k1_f32: 1.2,
        field_count: 2,
        fields: [
            Bm25fFieldSpec{ weight_f32: 2.0, b_f32: 0.75 },
            Bm25fFieldSpec{ weight_f32: 1.0, b_f32: 0.75 },
            Bm25fFieldSpec{},
            Bm25fFieldSpec{},
        ],
    };
    let idx = Bm25fIndex.init(cfg) ?? return;

    const d1: [2][]const u8 = ["alpha beta", "one two three four"];       // lens: 2, 4
    const d2: [2][]const u8 = ["gamma delta epsilon zeta", "five six"];   // lens: 4, 2
    expect_eq(idx.add_document(100, d1[0..]), true);
    expect_eq(idx.add_document(200, d2[0..]), true);

    const st1: Bm25fIndexStats = idx.stats();
    expect_eq(st1.active_docs, 2);
    expect_eq(approxEqF32(st1.avg_field_lens[0], 3.0, 0.001), true);
    expect_eq(approxEqF32(st1.avg_field_lens[1], 3.0, 0.001), true);

    // Remove doc 200 -> only doc 100 remains (lens: 2, 4)
    expect_eq(idx.remove_document(200), true);
    expect_eq(idx.remove_document(200), false); // already removed

    const st2 = idx.stats();
    expect_eq(st2.active_docs, 1);
    expect_eq(approxEqF32(st2.avg_field_lens[0], 2.0, 0.001), true);
    expect_eq(approxEqF32(st2.avg_field_lens[1], 4.0, 0.001), true);

    let hits: [2]Bm25fHit = [Bm25fHit{}; 2];
    let hits_s: []Bm25fHit = hits[0..];
    expect_eq(idx.search_topk("gamma", 2, hits_s), 0);
    expect_eq(idx.search_topk("alpha", 2, hits_s), 1);
    expect_eq(hits[0].doc_id, 100);
});

// ============================================================================
// Test 6: Query Term Deduplication & Deterministic Ascending doc_id Tie-Break
// ============================================================================
test("bm25f_index: query term deduplication & ascending doc_id tie-break", () => {
    const cfg = Bm25fConfig{
        k1_f32: 1.2,
        field_count: 1,
        fields: [
            Bm25fFieldSpec{ weight_f32: 1.0, b_f32: 0.75 },
            Bm25fFieldSpec{},
            Bm25fFieldSpec{},
            Bm25fFieldSpec{},
        ],
    };
    let idx = Bm25fIndex.init(cfg) ?? return;

    const same: [1][]const u8 = ["zero alloc vector search"];
    // Insert higher doc_id first to verify tie-break sorts by ascending doc_id
    expect_eq(idx.add_document(50, same[0..]), true);
    expect_eq(idx.add_document(10, same[0..]), true);
    expect_eq(idx.add_document(30, same[0..]), true);

    let h1: [3]Bm25fHit = [Bm25fHit{}; 3];
    let h2: [3]Bm25fHit = [Bm25fHit{}; 3];
    const n1 = idx.search_topk("vector", 3, h1[0..]);
    const n2 = idx.search_topk("vector VECTOR vector", 3, h2[0..]);
    expect_eq(n1, 3);
    expect_eq(n2, 3);

    // Duplicate query terms must produce identical score & matched_terms == 1
    expect_eq(approxEqF32(h1[0].score_f32, h2[0].score_f32, 0.0001), true);
    expect_eq(h2[0].matched_terms, 1);

    // Ascending doc_id tie-break: 10, 30, 50
    expect_eq(h1[0].doc_id, 10);
    expect_eq(h1[1].doc_id, 30);
    expect_eq(h1[2].doc_id, 50);
});

// ============================================================================
// Test 7: Boundary Guards (Invalid Config, Field Mismatch, Duplicate ID & Empty Query)
// ============================================================================
test("bm25f_index: invalid config, field mismatch, duplicate ID & empty query guards", () => {
    // Invalid configs
    let bad_k1 = Bm25fConfig.defaultsCodeSearch();
    bad_k1.k1_f32 = 0.0;
    expect_eq(Bm25fIndex.init(bad_k1) == null, true);

    let bad_fc = Bm25fConfig.defaultsCodeSearch();
    bad_fc.field_count = 5;
    expect_eq(Bm25fIndex.init(bad_fc) == null, true);

    let bad_b = Bm25fConfig.defaultsCodeSearch();
    bad_b.fields[0].b_f32 = 1.5;
    expect_eq(Bm25fIndex.init(bad_b) == null, true);

    let all_zero_w = Bm25fConfig{
        k1_f32: 1.2,
        field_count: 1,
        fields: [
            Bm25fFieldSpec{ weight_f32: 0.0, b_f32: 0.5 },
            Bm25fFieldSpec{},
            Bm25fFieldSpec{},
            Bm25fFieldSpec{},
        ],
    };
    expect_eq(Bm25fIndex.init(all_zero_w) == null, true);

    let idx = Bm25fIndex.init(Bm25fConfig.defaultsCodeSearch()) ?? return;
    // Mismatched field count (2 != 4) rejected
    const two_fields: [2][]const u8 = ["a", "b"];
    expect_eq(idx.add_document(1, two_fields[0..]), false);

    // Duplicate doc_id rejected
    const four_fields: [4][]const u8 = ["a", "b", "c", "d"];
    expect_eq(idx.add_document(1, four_fields[0..]), true);
    expect_eq(idx.add_document(1, four_fields[0..]), false);

    // Empty / punctuation-only query & k == 0 safely return 0
    let hits: [2]Bm25fHit = [Bm25fHit{}; 2];
    let hits_s: []Bm25fHit = hits[0..];
    expect_eq(idx.search_topk("", 2, hits_s), 0);
    expect_eq(idx.search_topk("   !!! --- ", 2, hits_s), 0);
    expect_eq(idx.search_topk("a", 0, hits_s), 0);
});
