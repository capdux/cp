// Origin: Pure-CP Stdlib Test Suite (lib/vector/rag_store.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Heap Local RAG Store Facade:
//   1. Upsert, in-place update, remove tombstone & automatic tombstone slot reuse + stats
//   2. Namespace & 64-bit tag bitmask (including high bit 63) pre-filtering
//   3. SIMD @dot cosine Top-K ranking with L2 normalization, min_score cutoff & tie-breaking
//   4. Token-budgeted LLM context packing ([doc:<id>]) & buffer overflow protection
//   5. Full store, dimension mismatch, zero-vector & NaN-vector guards
//   6. Invalid init & searchFiltered parameter guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RagDocMeta,
    RagSearchHit,
    RagStoreStats,
    InMemoryRagStore,
} from "cap:vector";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: Upsert, In-Place Update, Remove & Tombstone Slot Reuse
// ============================================================================

fn testUpsertUpdateRemoveAndReuse()! {
    let metas: [2]RagDocMeta = [RagDocMeta{}; 2];
    let vecs: [8]f32 = [0.0; 8];
    let metas_s: []RagDocMeta = metas[0..];
    let vecs_s: []f32 = vecs[0..];

    let store = cap.vector.InMemoryRagStore.init(metas_s, vecs_s, 4) ?? return error.InitFailed;
    expect_eq(store.len(), 0);
    expect_eq(store.capacity(), 2);
    expect_eq(store.embeddingDim(), 4);

    const v1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v2: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    const v3: [4]f32 = [0.0, 0.0, 1.0, 0.0];

    expect_eq(store.upsert(10, 1, 0b01, 20, 1000, "doc 10 v1", v1[0..]), true);
    expect_eq(store.upsert(20, 1, 0b10, 25, 1001, "doc 20", v2[0..]), true);
    expect_eq(store.len(), 2);

    // Store is full: new doc_id 30 must be rejected
    expect_eq(store.upsert(30, 1, 0b11, 30, 1002, "overflow", v3[0..]), false);

    // In-place update on existing doc_id 10 succeeds even when full
    expect_eq(store.upsert(10, 1, 0b11, 22, 1005, "doc 10 v2", v2[0..]), true);
    expect_eq(store.len(), 2);
    const meta10 = store.get(10) ?? return error.MissingDoc;
    expect_eq(meta10.token_count, 22);
    expect_eq(std.mem.eql(u8, meta10.text, "doc 10 v2"), true);

    // Remove doc 20 creates tombstone, then doc 30 reuses the tombstone slot
    expect_eq(store.remove(20), true);
    expect_eq(store.remove(20), false); // already removed
    expect_eq(store.get(20) == null, true);
    expect_eq(store.len(), 1);

    expect_eq(store.upsert(30, 1, 0b01, 30, 1010, "doc 30 reused", v3[0..]), true);
    expect_eq(store.len(), 2);

    const st: RagStoreStats = store.stats();
    expect_eq(st.active_docs, 2);
    expect_eq(st.deleted_slots, 0);
    expect_eq(st.free_slots, 0);
    expect_eq(st.total_upserts, 4);
    expect_eq(st.total_updates, 1);
    expect_eq(st.total_removals, 1);
}

// ============================================================================
// Test 2: Namespace & 64-Bit Tag Bitmask Pre-Filtering
// ============================================================================

fn testNamespaceAndTagBitmaskFilter()! {
    let metas: [4]RagDocMeta = [RagDocMeta{}; 4];
    let vecs: [16]f32 = [0.0; 16];
    let metas_s: []RagDocMeta = metas[0..];
    let vecs_s: []f32 = vecs[0..];
    let store = InMemoryRagStore.init(metas_s, vecs_s, 4) ?? return error.InitFailed;

    const tag_code: u64 = 1;
    const tag_prod: u64 = 1 << 63; // High bit 63
    const v: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    expect_eq(store.upsert(1, 100, tag_code, 10, 100, "ns100 code", v[0..]), true);
    expect_eq(store.upsert(2, 100, tag_code | tag_prod, 10, 101, "ns100 code+prod", v[0..]), true);
    expect_eq(store.upsert(3, 200, tag_code | tag_prod, 10, 102, "ns200 code+prod", v[0..]), true);
    expect_eq(store.upsert(4, 100, tag_prod, 10, 103, "ns100 prod only", v[0..]), true);

    let hits: [4]RagSearchHit = [RagSearchHit{}; 4];
    let hits_s: []RagSearchHit = hits[0..];

    // Namespace 100 + both tag_code & tag_prod -> only doc 2
    const n1 = store.searchFiltered(v[0..], 100, tag_code | tag_prod, 0.5, 4, hits_s) ?? return error.SearchFailed;
    expect_eq(n1, 1);
    expect_eq(hits[0].doc_id, 2);

    // Global namespace (0) + tag_code | tag_prod -> docs 2 and 3 (sorted by doc_id asc on tie)
    const n2 = store.searchFiltered(v[0..], 0, tag_code | tag_prod, 0.5, 4, hits_s) ?? return error.SearchFailed;
    expect_eq(n2, 2);
    expect_eq(hits[0].doc_id, 2);
    expect_eq(hits[1].doc_id, 3);
}

// ============================================================================
// Test 3: SIMD Cosine Top-K Ranking, L2 Normalization & min_score Cutoff
// ============================================================================

fn testSimdCosineTopKAndNormalization()! {
    let metas: [4]RagDocMeta = [RagDocMeta{}; 4];
    let vecs: [16]f32 = [0.0; 16];
    let metas_s: []RagDocMeta = metas[0..];
    let vecs_s: []f32 = vecs[0..];
    let store = InMemoryRagStore.init(metas_s, vecs_s, 4) ?? return error.InitFailed;

    // Unnormalized vectors: upsert normalizes automatically!
    const e1: [4]f32 = [10.0, 0.0, 0.0, 0.0];       // cos = 1.0
    const e2: [4]f32 = [3.0, 4.0, 0.0, 0.0];        // cos = 0.6
    const e3: [4]f32 = [8.0, 6.0, 0.0, 0.0];        // cos = 0.8
    const e4: [4]f32 = [-5.0, 0.0, 0.0, 0.0];       // cos = -1.0 (filtered out by min_score)

    expect_eq(store.upsert(1, 0, 0, 10, 1, "exact", e1[0..]), true);
    expect_eq(store.upsert(2, 0, 0, 10, 2, "mid", e2[0..]), true);
    expect_eq(store.upsert(3, 0, 0, 10, 3, "high", e3[0..]), true);
    expect_eq(store.upsert(4, 0, 0, 10, 4, "opposite", e4[0..]), true);

    const q: [4]f32 = [2.0, 0.0, 0.0, 0.0]; // Unnormalized query
    let hits: [2]RagSearchHit = [RagSearchHit{}; 2];
    let hits_s: []RagSearchHit = hits[0..];

    // Top-2 with min_score=0.5 -> [doc 1 (1.0), doc 3 (0.8)]
    const n = store.searchFiltered(q[0..], 0, 0, 0.5, 2, hits_s) ?? return error.SearchFailed;
    expect_eq(n, 2);
    expect_eq(hits[0].doc_id, 1);
    expect_eq(approxEqF32(hits[0].score, 1.0, 0.0001), true);
    expect_eq(hits[1].doc_id, 3);
    expect_eq(approxEqF32(hits[1].score, 0.8, 0.0001), true);
}

// ============================================================================
// Test 4: Token-Budgeted LLM Context Packing & Buffer Overflow Protection
// ============================================================================

fn testPackSearchContextAndBounds()! {
    let metas: [3]RagDocMeta = [RagDocMeta{}; 3];
    let vecs: [12]f32 = [0.0; 12];
    let metas_s: []RagDocMeta = metas[0..];
    let vecs_s: []f32 = vecs[0..];
    let store = InMemoryRagStore.init(metas_s, vecs_s, 4) ?? return error.InitFailed;

    const hits: [3]RagSearchHit = [
        RagSearchHit{ doc_id: 10, score: 0.95, token_count: 30, text: "alpha chunk" },
        RagSearchHit{ doc_id: 20, score: 0.90, token_count: 50, text: "oversized chunk" },
        RagSearchHit{ doc_id: 30, score: 0.85, token_count: 25, text: "gamma chunk" },
    ];

    // Budget = 60 tokens: fits doc 10 (30) + skips doc 20 (50) + fits doc 30 (25) = 55 tokens
    let ctx_buf: [256]u8 = [0; 256];
    let ctx_s: []u8 = ctx_buf[0..];
    const written = store.packSearchContext(hits[0..], 60, ctx_s) ?? return error.PackFailed;
    const text = ctx_s[0..written];
    expect_eq(std.mem.indexOf(u8, text, "[doc:10]\nalpha chunk\n\n") != null, true);
    expect_eq(std.mem.indexOf(u8, text, "[doc:20]") == null, true);
    expect_eq(std.mem.indexOf(u8, text, "[doc:30]\ngamma chunk\n\n") != null, true);

    // Zero budget -> 0 bytes written
    expect_eq(store.packSearchContext(hits[0..], 0, ctx_s) ?? 99, 0);

    // Tiny output buffer -> null without overflow
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(store.packSearchContext(hits[0..], 60, tiny_s) == null, true);
}

// ============================================================================
// Test 5: Dimension Mismatch, Zero-Vector & NaN-Vector Guards
// ============================================================================

fn testEmbeddingValidationGuards()! {
    let metas: [2]RagDocMeta = [RagDocMeta{}; 2];
    let vecs: [8]f32 = [0.0; 8];
    let metas_s: []RagDocMeta = metas[0..];
    let vecs_s: []f32 = vecs[0..];
    let store = InMemoryRagStore.init(metas_s, vecs_s, 4) ?? return error.InitFailed;

    const wrong_dim: [3]f32 = [1.0, 0.0, 0.0];
    const zero_vec: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    const nan_vec: [4]f32 = [1.0, std.math.nan(f32), 0.0, 0.0];
    const ok_vec: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    expect_eq(store.upsert(0, 0, 0, 10, 1, "bad id", ok_vec[0..]), false);
    expect_eq(store.upsert(1, 0, 0, 10, 1, "bad dim", wrong_dim[0..]), false);
    expect_eq(store.upsert(1, 0, 0, 10, 1, "zero", zero_vec[0..]), false);
    expect_eq(store.upsert(1, 0, 0, 10, 1, "nan", nan_vec[0..]), false);
    expect_eq(store.upsert(1, 0, 0, 10, 1, "ok", ok_vec[0..]), true);

    let hits: [2]RagSearchHit = [RagSearchHit{}; 2];
    let hits_s: []RagSearchHit = hits[0..];
    expect_eq(store.searchFiltered(wrong_dim[0..], 0, 0, 0.0, 1, hits_s) == null, true);
    expect_eq(store.searchFiltered(zero_vec[0..], 0, 0, 0.0, 1, hits_s) == null, true);
    expect_eq(store.searchFiltered(nan_vec[0..], 0, 0, 0.0, 1, hits_s) == null, true);
}

// ============================================================================
// Test 6: Invalid Init & Search Parameter Guards
// ============================================================================

fn testInitAndSearchParamGuards()! {
    let metas: [2]RagDocMeta = [RagDocMeta{}; 2];
    let vecs: [8]f32 = [0.0; 8];
    let empty_m: []RagDocMeta = metas[0..0];
    let metas_s: []RagDocMeta = metas[0..];
    let vecs_s: []f32 = vecs[0..];
    let small_v: []f32 = vecs[0..7]; // needs 2 * 4 = 8

    expect_eq(InMemoryRagStore.init(empty_m, vecs_s, 4) == null, true);
    expect_eq(InMemoryRagStore.init(metas_s, vecs_s, 0) == null, true);
    expect_eq(InMemoryRagStore.init(metas_s, small_v, 4) == null, true);

    let store = InMemoryRagStore.init(metas_s, vecs_s, 4) ?? return error.InitFailed;
    const q: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    let hits: [1]RagSearchHit = [RagSearchHit{}; 1];
    let hits_s: []RagSearchHit = hits[0..];

    expect_eq(store.searchFiltered(q[0..], 0, 0, 0.0, 0, hits_s) == null, true);
    expect_eq(store.searchFiltered(q[0..], 0, 0, 0.0, 2, hits_s) == null, true);
    expect_eq(store.searchFiltered(q[0..], 0, 0, std.math.nan(f32), 1, hits_s) == null, true);
}

test("vector rag_store: upsert, in-place update, remove & tombstone slot reuse", testUpsertUpdateRemoveAndReuse);
test("vector rag_store: namespace & 64-bit tag bitmask pre-filtering", testNamespaceAndTagBitmaskFilter);
test("vector rag_store: SIMD cosine Top-K ranking, L2 norm & min_score cutoff", testSimdCosineTopKAndNormalization);
test("vector rag_store: token-budgeted context packing & buffer overflow protection", testPackSearchContextAndBounds);
test("vector rag_store: dimension mismatch, zero-vector & NaN-vector guards", testEmbeddingValidationGuards);
test("vector rag_store: invalid init & search parameter guards", testInitAndSearchParamGuards);
