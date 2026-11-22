// Origin: Pure-CP Stdlib Test Suite (lib/vector/simhash_dedup.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc 64-bit Charikar SimHash & 4-Band LSH Dedup Index:
//   1. hammingDistanceU64 boundary values & hardware @popCount correctness
//   2. Identical & case/punctuation-perturbed texts produce 0 Hamming distance
//   3. Slightly edited near-duplicate long RAG documents detected & rejected
//   4. Distinct-topic documents produce high Hamming distance and ingest cleanly
//   5. 4-band Pigeonhole pre-filter (max_hamming_dist <= 3) vs exact full scan (> 3) 100% parity & tie-break
//   6. remove_doc logical eviction, re-ingestion of same text & slot reuse
//   7. Boundary guards: empty/punctuation-only text, duplicate doc_id, invalid config & 256-slot capacity limit

import { test, expect_eq } from "cap:test";
import {
    SIMHASH_MAX_DOCS,
    SIMHASH_NUM_BANDS,
    SimHashDedupConfig,
    SimHashMatch,
    SimHashDedupStats,
    hammingDistanceU64,
    computeSimHash64,
    SimHashDedupIndex,
} from "cap:vector";

// ============================================================================
// Test 1: hammingDistanceU64 Boundary & Hardware @popCount Verification
// ============================================================================
test("simhash_dedup: hammingDistanceU64 boundaries & @popCount accuracy", () => {
    expect_eq(SIMHASH_MAX_DOCS, 256);
    expect_eq(SIMHASH_NUM_BANDS, 4);

    expect_eq(hammingDistanceU64(0, 0), 0);
    expect_eq(hammingDistanceU64(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF), 0);
    expect_eq(hammingDistanceU64(0, 0xFFFFFFFFFFFFFFFF), 64);
    expect_eq(hammingDistanceU64(0xAAAAAAAAAAAAAAAA, 0x5555555555555555), 64);
    expect_eq(hammingDistanceU64(0x0000000000000001, 0x0000000000000000), 1);
    // 1 bit flipped in each of the 4 16-bit bands -> distance 4
    const four_bands: u64 = (1 as u64) | ((1 as u64) << 16) | ((1 as u64) << 32) | ((1 as u64) << 48);
    expect_eq(hammingDistanceU64(0, four_bands), 4);
    expect_eq(hammingDistanceU64(0xF0F0F0F00F0F0F0F, 0x0000000000000000), 32);
});

// ============================================================================
// Test 2: Identical & Case/Punctuation-Perturbed Texts -> 0 Hamming Distance
// ============================================================================
test("simhash_dedup: case & punctuation normalization yields 0 Hamming distance", () => {
    const t1: []const u8 = "Retrieval-Augmented Generation improves factual accuracy in LLM systems.";
    const t2: []const u8 = "retrieval augmented generation, IMPROVES factual accuracy in llm systems!!!";

    const fp1: u64 = computeSimHash64(t1, 2);
    const fp2: u64 = computeSimHash64(t2, 2);
    expect_eq(fp1 != 0, true);
    expect_eq(fp1, fp2);
    expect_eq(hammingDistanceU64(fp1, fp2), 0);

    let idx = SimHashDedupIndex.init(SimHashDedupConfig{}) ?? return;
    const m1: SimHashMatch = idx.check_and_insert(10, t1);
    expect_eq(m1.inserted, true);
    expect_eq(m1.is_duplicate, false);
    expect_eq(m1.fingerprint, fp1);

    const m2: SimHashMatch = idx.check_and_insert(20, t2);
    expect_eq(m2.inserted, false);
    expect_eq(m2.is_duplicate, true);
    expect_eq(m2.matched_doc_id, 10);
    expect_eq(m2.hamming_dist, 0);

    const st: SimHashDedupStats = idx.stats();
    expect_eq(st.active_docs, 1);
    expect_eq(st.total_checked, 2);
    expect_eq(st.unique_accepted, 1);
    expect_eq(st.duplicates_rejected, 1);
});

// ============================================================================
// Test 3: Slightly Edited Near-Duplicate Long Document Detection & Rejection
// ============================================================================
test("simhash_dedup: slightly paraphrased long RAG doc detected as near-duplicate", () => {
    const orig: []const u8 = "Modern distributed inference engines combine continuous batching with paged attention memory blocks so that thousands of concurrent language model requests share prefix caches without fragmentation overhead across high throughput GPU clusters in production deployments.";
    const edited: []const u8 = "Modern distributed inference engines combine continuous batching with paged attention memory blocks so that thousands of concurrent language model requests share prefix caches without fragmentation overhead across high throughput GPU clusters in enterprise deployments.";

    const fp_orig: u64 = computeSimHash64(orig, 2);
    const fp_edit: u64 = computeSimHash64(edited, 2);
    const dist: u32 = hammingDistanceU64(fp_orig, fp_edit);
    expect_eq(dist <= 6, true);

    // Use max_hamming_dist = 6 to verify near-duplicate rejection on real edited prose
    let idx = SimHashDedupIndex.init(SimHashDedupConfig{
        max_hamming_dist: 6,
        min_token_len: 2,
    }) ?? return;

    const r1 = idx.check_and_insert(1001, orig);
    expect_eq(r1.inserted, true);

    const r2 = idx.check_and_insert(1002, edited);
    expect_eq(r2.inserted, false);
    expect_eq(r2.is_duplicate, true);
    expect_eq(r2.matched_doc_id, 1001);
    expect_eq(r2.hamming_dist, dist);

    // query_nearest also detects without mutating stats
    const q = idx.query_nearest(edited);
    expect_eq(q.is_duplicate, true);
    expect_eq(q.matched_doc_id, 1001);
    expect_eq(idx.stats().total_checked, 2);
});

// ============================================================================
// Test 4: Distinct-Topic Documents Have High Hamming Distance & Ingest Cleanly
// ============================================================================
test("simhash_dedup: unrelated topic documents have high distance & all ingest", () => {
    const d1: []const u8 = "B-tree database storage engines optimize disk page locality and write-ahead logging for ACID transactions.";
    const d2: []const u8 = "Photosynthesis converts solar photons into chemical energy inside chloroplast thylakoid membranes of green plants.";
    const d3: []const u8 = "Baroque harpsichord concertos feature contrapuntal fugues and basso continuo harmonic accompaniment.";

    const fp1 = computeSimHash64(d1, 2);
    const fp2 = computeSimHash64(d2, 2);
    const fp3 = computeSimHash64(d3, 2);
    expect_eq(hammingDistanceU64(fp1, fp2) > 10, true);
    expect_eq(hammingDistanceU64(fp1, fp3) > 10, true);
    expect_eq(hammingDistanceU64(fp2, fp3) > 10, true);

    let idx = SimHashDedupIndex.init(SimHashDedupConfig{
        max_hamming_dist: 3,
        min_token_len: 2,
    }) ?? return;

    expect_eq(idx.check_and_insert(1, d1).inserted, true);
    expect_eq(idx.check_and_insert(2, d2).inserted, true);
    expect_eq(idx.check_and_insert(3, d3).inserted, true);

    const st = idx.stats();
    expect_eq(st.active_docs, 3);
    expect_eq(st.unique_accepted, 3);
    expect_eq(st.duplicates_rejected, 0);
});

// ============================================================================
// Test 5: 4-Band Pigeonhole Pre-Filter vs Exact Full Scan 100% Parity & Tie-Break
// ============================================================================
test("simhash_dedup: 4-band Pigeonhole filter matches exact scan 100% & breaks ties by doc_id", () => {
    let idx_band = SimHashDedupIndex.init(SimHashDedupConfig{
        max_hamming_dist: 3,
        min_token_len: 2,
    }) ?? return;
    let idx_scan = SimHashDedupIndex.init(SimHashDedupConfig{
        max_hamming_dist: 5,
        min_token_len: 2,
    }) ?? return;

    const base_text: []const u8 = "Zero allocation systems programming with deterministic destruction and compile time safety.";
    const base_fp: u64 = computeSimHash64(base_text, 2);
    expect_eq(idx_band.check_and_insert(50, base_text).inserted, true);
    expect_eq(idx_scan.check_and_insert(50, base_text).inserted, true);

    // Worst-case Pigeonhole test: flip 1 bit in each of bands 0, 1, 2 (distance = 3, only band 3 matches)
    const flip_3_bands: u64 = ((1 as u64) << 2) | ((1 as u64) << 19) | ((1 as u64) << 37);
    const q3: u64 = base_fp ^ flip_3_bands;
    const mb3 = idx_band.query_nearest_fp(q3);
    const ms3 = idx_scan.query_nearest_fp(q3);
    expect_eq(mb3.is_duplicate, true);
    expect_eq(mb3.matched_doc_id, 50);
    expect_eq(mb3.hamming_dist, 3);
    expect_eq(ms3.is_duplicate, true);
    expect_eq(ms3.matched_doc_id, 50);
    expect_eq(ms3.hamming_dist, 3);

    // Flip 1 bit in all 4 bands (distance = 4): band index (max=3) rejects, full scan (max=5) accepts
    const flip_4_bands: u64 = flip_3_bands | ((1 as u64) << 55);
    const q4: u64 = base_fp ^ flip_4_bands;
    const mb4 = idx_band.query_nearest_fp(q4);
    const ms4 = idx_scan.query_nearest_fp(q4);
    expect_eq(mb4.is_duplicate, false);
    expect_eq(ms4.is_duplicate, true);
    expect_eq(ms4.hamming_dist, 4);

    // Tie-breaking: two entries equidistant (dist = 1) from target_fp -> lower doc_id (12 < 99) wins
    let idx_tie = SimHashDedupIndex.init(SimHashDedupConfig{
        max_hamming_dist: 3,
        min_token_len: 2,
    }) ?? return;
    // Populate two entries directly with controlled fingerprints at distance 1 from target 0x123456789ABCDEF0
    const target_fp: u64 = 0x123456789ABCDEF0;
    idx_tie.entries[0] = idx_tie.entries[0];
    idx_tie.entries[0].doc_id = 99;
    idx_tie.entries[0].fingerprint = target_fp ^ (1 as u64);
    idx_tie.entries[0].active = true;
    idx_tie.entries[1].doc_id = 12;
    idx_tie.entries[1].fingerprint = target_fp ^ ((1 as u64) << 20);
    idx_tie.entries[1].active = true;
    idx_tie.active_docs = 2;

    const tie_hit = idx_tie.query_nearest_fp(target_fp);
    expect_eq(tie_hit.is_duplicate, true);
    expect_eq(tie_hit.hamming_dist, 1);
    expect_eq(tie_hit.matched_doc_id, 12);
});

// ============================================================================
// Test 6: remove_doc Logical Eviction, Re-Ingestion & Slot Reuse
// ============================================================================
test("simhash_dedup: remove_doc allows re-ingestion & reuses freed slot", () => {
    let idx = SimHashDedupIndex.init(SimHashDedupConfig{}) ?? return;
    const doc: []const u8 = "Vector quantization reduces embedding memory footprint by eight to thirty two times.";

    expect_eq(idx.check_and_insert(77, doc).inserted, true);
    expect_eq(idx.stats().active_docs, 1);

    // Duplicate rejected while doc 77 is active
    expect_eq(idx.check_and_insert(78, doc).is_duplicate, true);

    // Removing non-existent doc returns false
    expect_eq(idx.remove_doc(999), false);

    // Remove doc 77 -> active_docs drops to 0, same text can now be re-ingested under doc 78
    expect_eq(idx.remove_doc(77), true);
    expect_eq(idx.stats().active_docs, 0);

    const re = idx.check_and_insert(78, doc);
    expect_eq(re.inserted, true);
    expect_eq(re.is_duplicate, false);
    expect_eq(idx.stats().active_docs, 1);
    expect_eq(idx.stats().unique_accepted, 2);
});

// ============================================================================
// Test 7: Boundary Guards (Invalid Config, Empty Text, Duplicate doc_id & 256-Slot Cap)
// ============================================================================
test("simhash_dedup: invalid config, empty text, duplicate doc_id & 256-slot cap guards", () => {
    // Invalid configs return null
    expect_eq(SimHashDedupIndex.init(SimHashDedupConfig{ max_hamming_dist: 17, min_token_len: 2 }) == null, true);
    expect_eq(SimHashDedupIndex.init(SimHashDedupConfig{ max_hamming_dist: 3, min_token_len: 0 }) == null, true);

    let idx = SimHashDedupIndex.init(SimHashDedupConfig{
        max_hamming_dist: 0,
        min_token_len: 2,
    }) ?? return;

    // Empty and punctuation-only texts produce fp == 0 and are safely not inserted
    const empty: []const u8 = "";
    const punct: []const u8 = "   !!! --- ... a b c ";
    expect_eq(computeSimHash64(empty, 2), 0);
    expect_eq(computeSimHash64(punct, 2), 0);
    expect_eq(idx.check_and_insert(1, empty).inserted, false);
    expect_eq(idx.check_and_insert(2, punct).inserted, false);
    expect_eq(idx.stats().active_docs, 0);

    // Duplicate active doc_id guard (even with completely different text!)
    const txt_a: []const u8 = "Alpha compiler lexer parser semantic analyzer codegen.";
    const txt_b: []const u8 = "Oceanographic coral reef biodiversity marine biology survey.";
    expect_eq(idx.check_and_insert(500, txt_a).inserted, true);
    const dup_id_res = idx.check_and_insert(500, txt_b);
    expect_eq(dup_id_res.inserted, false);
    expect_eq(dup_id_res.is_duplicate, true);
    expect_eq(dup_id_res.matched_doc_id, 500);

    // Fill remaining 255 slots with unique synthetic fingerprints to test 256-slot capacity limit
    let slot: usize = 1;
    while (slot < SIMHASH_MAX_DOCS) : (slot += 1) {
        idx.entries[slot].doc_id = 1000 + (slot as u64);
        idx.entries[slot].fingerprint = (slot as u64) *% 0x9e3779b97f4a7c15;
        idx.entries[slot].active = true;
    }
    idx.active_docs = SIMHASH_MAX_DOCS;

    // Inserting a brand-new unique text when full must safely return inserted = false
    const overflow_txt: []const u8 = "Astronomical supernova remnant spectroscopy x-ray observatory telescope.";
    const full_res = idx.check_and_insert(9999, overflow_txt);
    expect_eq(full_res.inserted, false);
    expect_eq(full_res.is_duplicate, false);
    expect_eq(idx.stats().active_docs, SIMHASH_MAX_DOCS);

    // Removing 1 doc frees a slot -> overflow_txt now succeeds!
    expect_eq(idx.remove_doc(1005), true);
    const after_free = idx.check_and_insert(9999, overflow_txt);
    expect_eq(after_free.inserted, true);
    expect_eq(idx.stats().active_docs, SIMHASH_MAX_DOCS);
});
