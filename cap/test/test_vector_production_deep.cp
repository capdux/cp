// Source: cp/cap/test/test_vector_production_deep.cp
// Deep Production Verification Suite for cap:vector & cap.vector
// Covers:
//   1. VectorIndex Cosine/Dot/Euclidean search, upsert overwrite, O(1) remove & DimensionMismatch
//   2. Bm25Index TF saturation, IDF discrimination, stopword filtering, removeDocument & ranking
//   3. RrfFusion.fuse dual-hit boosting, deduplication, metadata preservation & top_k
//   4. Empty query, empty index, stopword-only query, top_k = 0 & dim = 0 boundary safety
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.vector`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    VectorError,
    DistanceMetric,
    SearchHit,
    VectorIndex,
    Bm25Config,
    Bm25Index,
    RrfFusion,
} from "../../lib/vector/index.cp";

const EPS_F32: f32 = 0.0001;

fn approxF32(a: f32, b: f32, eps: f32): bool {
    return @abs(a - b) <= eps;
}

fn testVectorIndexMetricsUpsertRemoveAndErrors()! {
    const alloc = std.heap.page_allocator;

    // 1. Cosine similarity with 11-dim vectors (covers 8-lane SIMD + 3 scalar tail)
    let cos_idx = VectorIndex.init(alloc, 11, .Cosine)?;
    defer cos_idx.deinit();

    let v_agent: [11]f32 = [0.0; 11];
    v_agent[0] = 1.0;
    v_agent[10] = 1.0;

    let v_memory: [11]f32 = [0.0; 11];
    v_memory[0] = 0.8;
    v_memory[1] = 0.6;
    v_memory[10] = 0.2;

    let v_other: [11]f32 = [0.0; 11];
    v_other[5] = 1.0;

    cos_idx.upsert("doc-agent", &v_agent, "{\"topic\":\"agent\"}")?;
    cos_idx.upsert("doc-memory", &v_memory, "{\"topic\":\"memory\"}")?;
    cos_idx.upsert("doc-other", &v_other, "{\"topic\":\"other\"}")?;
    expect_eq(cos_idx.len(), 3);

    // DimensionMismatch on upsert
    const bad_vec: [3]f32 = [1.0, 2.0, 3.0];
    let got_upsert_mismatch = false;
    cos_idx.upsert("bad", &bad_vec, "") catch e => {
        if (e == VectorError.DimensionMismatch) got_upsert_mismatch = true;
    };
    expect_eq(got_upsert_mismatch, true);
    expect_eq(cos_idx.len(), 3);

    // DimensionMismatch on search
    let got_search_mismatch = false;
    * = cos_idx.search(alloc, &bad_vec, 2, 0.0) catch e => {
        if (e == VectorError.DimensionMismatch) got_search_mismatch = true;
    };
    expect_eq(got_search_mismatch, true);

    // Cosine search & min_score filter
    const cos_hits = cos_idx.search(alloc, &v_agent, 3, 0.1)?;
    defer alloc.free(cos_hits);
    expect_eq(cos_hits.len, 2);
    expect_eq(std.mem.eql(u8, cos_hits[0].id, "doc-agent"), true);
    expect_eq(approxF32(cos_hits[0].score, 1.0, EPS_F32), true);
    expect_eq(std.mem.eql(u8, cos_hits[0].meta, "{\"topic\":\"agent\"}"), true);
    expect_eq(std.mem.eql(u8, cos_hits[1].id, "doc-memory"), true);

    // Upsert overwrite existing id (replaces vector and meta, keeps len == 3)
    let v_replaced: [11]f32 = [0.0; 11];
    v_replaced[0] = 0.99;
    v_replaced[10] = 0.99;
    cos_idx.upsert("doc-other", &v_replaced, "{\"topic\":\"promoted\"}")?;
    expect_eq(cos_idx.len(), 3);

    const cos_hits_after = cos_idx.search(alloc, &v_agent, 2, 0.5)?;
    defer alloc.free(cos_hits_after);
    expect_eq(cos_hits_after.len, 2);
    expect_eq(std.mem.eql(u8, cos_hits_after[1].id, "doc-other"), true);
    expect_eq(std.mem.eql(u8, cos_hits_after[1].meta, "{\"topic\":\"promoted\"}"), true);

    // O(1) swap-remove middle element & verify remaining entries remain searchable
    expect_eq(cos_idx.remove("doc-agent"), true);
    expect_eq(cos_idx.remove("doc-agent"), false);
    expect_eq(cos_idx.len(), 2);

    const cos_after_rm = cos_idx.search(alloc, &v_agent, 5, 0.0)?;
    defer alloc.free(cos_after_rm);
    expect_eq(cos_after_rm.len, 2);
    expect_eq(std.mem.eql(u8, cos_after_rm[0].id, "doc-other"), true);
    expect_eq(std.mem.eql(u8, cos_after_rm[1].id, "doc-memory"), true);

    // 2. Dot product metric
    let dot_idx = VectorIndex.init(alloc, 4, .Dot)?;
    defer dot_idx.deinit();
    dot_idx.upsert("small", [1.0, 1.0, 0.0, 0.0], "s")?;
    dot_idx.upsert("large", [5.0, 2.0, 0.0, 0.0], "l")?;
    const dot_hits = dot_idx.search(alloc, [2.0, 1.0, 0.0, 0.0], 2, 0.0)?;
    defer alloc.free(dot_hits);
    expect_eq(dot_hits.len, 2);
    expect_eq(std.mem.eql(u8, dot_hits[0].id, "large"), true);
    expect_eq(approxF32(dot_hits[0].score, 12.0, EPS_F32), true);
    expect_eq(std.mem.eql(u8, dot_hits[1].id, "small"), true);
    expect_eq(approxF32(dot_hits[1].score, 3.0, EPS_F32), true);

    // 3. Euclidean (L2) metric -> similarity = 1.0 / (1.0 + d)
    let l2_idx = VectorIndex.init(alloc, 3, .Euclidean)?;
    defer l2_idx.deinit();
    l2_idx.upsert("exact", [1.0, 2.0, 3.0], "e")?;
    l2_idx.upsert("near", [4.0, 6.0, 3.0], "n")?; // dist = 5.0 => score = 1/6
    l2_idx.upsert("far", [10.0, 20.0, 30.0], "f")?;
    const l2_hits = l2_idx.search(alloc, [1.0, 2.0, 3.0], 3, 0.0)?;
    defer alloc.free(l2_hits);
    expect_eq(l2_hits.len, 3);
    expect_eq(std.mem.eql(u8, l2_hits[0].id, "exact"), true);
    expect_eq(approxF32(l2_hits[0].score, 1.0, EPS_F32), true);
    expect_eq(std.mem.eql(u8, l2_hits[1].id, "near"), true);
    expect_eq(approxF32(l2_hits[1].score, 1.0 / 6.0, EPS_F32), true);
    expect_eq(l2_hits[1].score > l2_hits[2].score, true);
}

fn testBm25SaturationIdfAndDeletion()! {
    const alloc = std.heap.page_allocator;

    let bm25 = Bm25Index.init(alloc, Bm25Config{ k1: 1.5, b: 0.75 });
    defer bm25.deinit();

    // Common word: "system" appears in 4 docs; rare word: "quantum" appears in 1 doc
    bm25.addDocument("d1", "The system uses fast memory and system architecture.")?;
    bm25.addDocument("d2", "System logs show system health and system uptime.")?;
    bm25.addDocument("d3", "A system with quantum vector indexing for RAG.")?;
    bm25.addDocument("d4", "System kernel scheduler handles BEAM reductions.")?;
    expect_eq(bm25.docCount(), 4);

    // 1. IDF discrimination: query "system quantum" must rank d3 #1 because "quantum" has much higher IDF
    const idf_hits = bm25.search(alloc, "SYSTEM, quantum!", 4)?;
    defer alloc.free(idf_hits);
    expect_eq(idf_hits.len, 4);
    expect_eq(std.mem.eql(u8, idf_hits[0].id, "d3"), true);
    expect_eq(idf_hits[0].score > idf_hits[1].score, true);

    // 2. Term frequency saturation: 10x repetition vs 1x repetition is bounded (< (k1 + 1) = 2.5x)
    let sat = Bm25Index.init(alloc);
    defer sat.deinit();
    sat.addDocument("once", "vector alpha beta gamma")?;
    sat.addDocument("ten", "vector vector vector vector vector vector vector vector vector vector alpha beta gamma")?;
    sat.addDocument("other", "delta epsilon zeta")?;
    const sat_hits = sat.search(alloc, "vector", 3)?;
    defer alloc.free(sat_hits);
    expect_eq(sat_hits.len, 2);
    expect_eq(std.mem.eql(u8, sat_hits[0].id, "ten"), true);
    expect_eq(std.mem.eql(u8, sat_hits[1].id, "once"), true);
    // Due to BM25 TF saturation, 10x TF cannot yield 10x score
    expect_eq(sat_hits[0].score < sat_hits[1].score * 3.0, true);

    // 3. Query deduplication: repeating "quantum" in query should not double score vs single "quantum"
    const q1 = bm25.search(alloc, "quantum", 2)?;
    defer alloc.free(q1);
    const q2 = bm25.search(alloc, "quantum Quantum QUANTUM", 2)?;
    defer alloc.free(q2);
    expect_eq(q1.len, 1);
    expect_eq(q2.len, 1);
    expect_eq(approxF32(q1[0].score, q2[0].score, EPS_F32), true);

    // 4. Document deletion updates docCount and removes postings cleanly
    expect_eq(bm25.removeDocument("d3"), true);
    expect_eq(bm25.removeDocument("d3"), false);
    expect_eq(bm25.docCount(), 3);

    const after_rm = bm25.search(alloc, "quantum", 5)?;
    defer alloc.free(after_rm);
    expect_eq(after_rm.len, 0);

    // 5. Upsert/replace existing doc ID in BM25
    bm25.addDocument("d4", "Freshly replaced document with photon index")?;
    expect_eq(bm25.docCount(), 3);
    const old_kw = bm25.search(alloc, "reductions", 5)?;
    defer alloc.free(old_kw);
    expect_eq(old_kw.len, 0);
    const new_kw = bm25.search(alloc, "photon", 5)?;
    defer alloc.free(new_kw);
    expect_eq(new_kw.len, 1);
    expect_eq(std.mem.eql(u8, new_kw[0].id, "d4"), true);
}

fn testRrfFusionDualBoostAndDedup()! {
    const alloc = std.heap.page_allocator;

    // Vector hits: rank1=docA, rank2=docB, rank3=docC (also duplicate docA to test per-list dedup)
    const vec_hits: [4]SearchHit = [
        SearchHit{ id: "docA", score: 0.95, meta: "meta-A" },
        SearchHit{ id: "docB", score: 0.88, meta: "meta-B" },
        SearchHit{ id: "docC", score: 0.72, meta: "" },
        SearchHit{ id: "docA", score: 0.10, meta: "dup-ignored" },
    ];

    // BM25 hits: rank1=docD, rank2=docB, rank3=docC
    // With k=60:
    //   docB = 1/(60+2) + 1/(60+2) = 2/62 = 0.032258... (dual hit boosted to #1!)
    //   docC = 1/(60+3) + 1/(60+3) = 2/63 = 0.031746... (dual hit boosted to #2!)
    //   docA = 1/(60+1) = 1/61 = 0.016393... (tied with docD, 'docA' < 'docD' lexicographically)
    //   docD = 1/(60+1) = 1/61 = 0.016393...
    const text_hits: [3]SearchHit = [
        SearchHit{ id: "docD", score: 4.5, meta: "bm25-D" },
        SearchHit{ id: "docB", score: 3.2, meta: "" },
        SearchHit{ id: "docC", score: 2.1, meta: "bm25-C" },
    ];

    const fused = RrfFusion.fuse(alloc, &vec_hits, &text_hits, 60.0, 4)?;
    defer alloc.free(fused);

    expect_eq(fused.len, 4);
    expect_eq(std.mem.eql(u8, fused[0].id, "docB"), true);
    expect_eq(approxF32(fused[0].score, 2.0 / 62.0, EPS_F32), true);
    expect_eq(std.mem.eql(u8, fused[0].meta, "meta-B"), true);

    expect_eq(std.mem.eql(u8, fused[1].id, "docC"), true);
    expect_eq(approxF32(fused[1].score, 2.0 / 63.0, EPS_F32), true);
    expect_eq(std.mem.eql(u8, fused[1].meta, "bm25-C"), true);

    expect_eq(std.mem.eql(u8, fused[2].id, "docA"), true);
    expect_eq(approxF32(fused[2].score, 1.0 / 61.0, EPS_F32), true);
    expect_eq(std.mem.eql(u8, fused[2].meta, "meta-A"), true);

    expect_eq(std.mem.eql(u8, fused[3].id, "docD"), true);
    expect_eq(approxF32(fused[3].score, 1.0 / 61.0, EPS_F32), true);

    // Top-k truncation
    const top2 = RrfFusion.fuse(alloc, &vec_hits, &text_hits, 60.0, 2)?;
    defer alloc.free(top2);
    expect_eq(top2.len, 2);
    expect_eq(std.mem.eql(u8, top2[0].id, "docB"), true);
    expect_eq(std.mem.eql(u8, top2[1].id, "docC"), true);
}

fn testEmptyAndBoundarySafety()! {
    const alloc = std.heap.page_allocator;

    // 1. InvalidDimension (dim == 0)
    let got_invalid_dim = false;
    * = VectorIndex.init(alloc, 0, .Cosine) catch e => {
        if (e == VectorError.InvalidDimension) got_invalid_dim = true;
    };
    expect_eq(got_invalid_dim, true);

    // 2. Empty VectorIndex & top_k = 0
    let v_idx = VectorIndex.init(alloc, 3, .Cosine)?;
    defer v_idx.deinit();

    const empty_v_res = v_idx.search(alloc, [1.0, 0.0, 0.0], 5, 0.0)?;
    defer alloc.free(empty_v_res);
    expect_eq(empty_v_res.len, 0);

    v_idx.upsert("only", [1.0, 0.0, 0.0], "m")?;
    const zero_k_v = v_idx.search(alloc, [1.0, 0.0, 0.0], 0, 0.0)?;
    defer alloc.free(zero_k_v);
    expect_eq(zero_k_v.len, 0);

    // 3. Empty Bm25Index, empty query, stopword-only query, top_k = 0
    let b_idx = Bm25Index.init(alloc);
    defer b_idx.deinit();

    const empty_b_res = b_idx.search(alloc, "hello", 5)?;
    defer alloc.free(empty_b_res);
    expect_eq(empty_b_res.len, 0);

    b_idx.addDocument("d1", "the and of to in")?;
    expect_eq(b_idx.docCount(), 1);

    const q_empty = b_idx.search(alloc, "", 5)?;
    defer alloc.free(q_empty);
    expect_eq(q_empty.len, 0);

    const q_stop = b_idx.search(alloc, "the and or is in", 5)?;
    defer alloc.free(q_stop);
    expect_eq(q_stop.len, 0);

    const q_zero_k = b_idx.search(alloc, "hello", 0)?;
    defer alloc.free(q_zero_k);
    expect_eq(q_zero_k.len, 0);

    // 4. RrfFusion empty inputs & top_k = 0
    const empty_hits: []const SearchHit = [];
    const rrf_empty = RrfFusion.fuse(alloc, empty_hits, empty_hits, 60.0, 5)?;
    defer alloc.free(rrf_empty);
    expect_eq(rrf_empty.len, 0);

    const one_hit: [1]SearchHit = [SearchHit{ id: "x", score: 1.0, meta: "" }];
    const rrf_zero_k = RrfFusion.fuse(alloc, &one_hit, empty_hits, 60.0, 0)?;
    defer alloc.free(rrf_zero_k);
    expect_eq(rrf_zero_k.len, 0);
}

fn testCapVectorDynamicMount()! {
    const alloc = std.heap.page_allocator;

    let vec_idx = cap.vector.VectorIndex.init(alloc, 3, cap.vector.DistanceMetric.Cosine)?;
    defer vec_idx.deinit();
    vec_idx.upsert("mem1", [1.0, 0.0, 0.0], "{\"kind\":\"episodic\"}")?;
    vec_idx.upsert("mem2", [0.7, 0.7, 0.0], "{\"kind\":\"semantic\"}")?;
    vec_idx.upsert("mem3", [0.0, 0.0, 1.0], "{\"kind\":\"procedural\"}")?;

    let bm25 = cap.vector.Bm25Index.init(alloc, cap.vector.Bm25Config{ k1: 1.5, b: 0.75 });
    defer bm25.deinit();
    bm25.addDocument("mem1", "agent planning loop")?;
    bm25.addDocument("mem2", "hybrid rag semantic vector memory")?;
    bm25.addDocument("mem3", "rag memory persistence")?;

    const v_hits = vec_idx.search(alloc, [0.8, 0.6, 0.0], 3, 0.0)?;
    defer alloc.free(v_hits);

    const t_hits = bm25.search(alloc, "semantic rag memory", 3)?;
    defer alloc.free(t_hits);

    const fused = cap.vector.RrfFusion.fuse(alloc, v_hits, t_hits, 60.0, 3)?;
    defer alloc.free(fused);

    expect_eq(fused.len, 3);
    // mem2 ranks #1 in vector (closest to [0.8, 0.6, 0]) AND #1 in BM25 (matches all 3 query terms)
    expect_eq(std.mem.eql(u8, fused[0].id, "mem2"), true);
    expect_eq(std.mem.eql(u8, fused[0].meta, "{\"kind\":\"semantic\"}"), true);
    expect_eq(approxF32(fused[0].score, 2.0 / 61.0, EPS_F32), true);
}

fn main() {
    test("1. VectorIndex Cosine/Dot/Euclidean Search, Upsert Overwrite, Remove & DimensionMismatch", () => {
        testVectorIndexMetricsUpsertRemoveAndErrors()?;
    });
    test("2. Bm25Index TF Saturation, IDF Discrimination, Deletion & Exact Ranking", () => {
        testBm25SaturationIdfAndDeletion()?;
    });
    test("3. RrfFusion.fuse Dual-Hit Boost, Deduplication & Top-K", () => {
        testRrfFusionDualBoostAndDedup()?;
    });
    test("4. Empty Query, Empty Index, Stopword-Only & top_k=0 Boundary Safety", () => {
        testEmptyAndBoundarySafety()?;
    });
    test("5. import cap from \"cap\"; cap.vector Automatic Dynamic Lazy Mounting", () => {
        testCapVectorDynamicMount()?;
    });
}
