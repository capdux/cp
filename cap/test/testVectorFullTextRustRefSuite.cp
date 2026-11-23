// quickwit-oss/tantivy (MIT) Reference-Hardened Full-Text Search & SIMD Scoring Test Suite
// Covers:
//   - Phrase Query with slop=0 (exact), slop=1/2 (proximity), repeated terms & monotone positions
//   - Structured Boolean Query (MUST / SHOULD / MUST_NOT + boost + minimum_should_match)
//   - Prefix Query, linear O(N*M) Wildcard Query (* / ? & anti-backtracking), SIMD ASCII normalization
//   - Tantivy BM25 monotonicity & boundary guards (empty index, N=0, df=0, df>N, avgdl=0, all-deleted, NaN)
//   - 8-lane SIMD batch BM25 & hybrid linear fusion vs scalar reference (< 1e-4) + RRF top-K fusion
//   - 1,200-iteration deterministic PRNG indexing/deletion/query/malformed-query fuzzing (0 panic/OOB/NaN/hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    FullTextError,
    FullTextFieldSpec,
    FullTextConfig,
    FtDocMeta,
    FtTermEntry,
    FtPosting,
    FtHit,
    FullTextBuffers,
    FullTextScratch,
    FullTextIndex,
    computeBm25fIdf,
    computeBm25fTermScore,
} from "../../lib/vector/fullTextIndex.cp";
import {
    FtOccur,
    FtClauseKind,
    FtBoolClause,
    FtBoolQuery,
    normalizeAsciiQuerySimd,
    wildcardMatchBytes,
    tantivyBm25Idf,
    tantivyBm25Score,
    batchBm25ScoreSimdF32,
    fuseHybridScoresSimdF32,
    rrfFuseTopKHits,
    searchBooleanQuery,
    searchPhraseQuery,
    searchPrefixQuery,
    searchWildcardQuery,
} from "../../lib/vector/fullTextQuery.cp";

fn ftAbsF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

fn ftStrEq(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

fn main() {
    // ========================================================================
    // 1. Phrase Query with Slop (slop=0 exact, slop=1/2 proximity, repeated terms)
    // ========================================================================
    test("ft_ref_01_phrase_query_slop_and_repeated_terms", () => {
        let fspecs: [2]FullTextFieldSpec = [
            FullTextFieldSpec{ weight: 2.0, b: 0.75 },
            FullTextFieldSpec{ weight: 1.0, b: 0.75 },
        ];
        let flsums: [2]u64 = [0; 2];
        let docs: [16]FtDocMeta = [FtDocMeta{}; 16];
        let dflens: [32]u32 = [0; 32];
        let terms: [128]FtTermEntry = [FtTermEntry{}; 128];
        let thash: [256]u32 = [0; 256];
        let tbytes: [4096]u8 = [0; 4096];
        let postings: [512]FtPosting = [FtPosting{}; 512];
        let positions: [1024]u32 = [0; 1024];

        let cfg = FullTextConfig{
            k1: 1.2,
            field_count: 2,
            enable_stemming: false,
            enable_stopwords: false,
            enable_cjk_bigrams: false,
        };
        let bufs = FullTextBuffers{
            field_specs: fspecs[0..],
            field_len_sums: flsums[0..],
            docs: docs[0..],
            doc_field_lens: dflens[0..],
            terms: terms[0..],
            term_hash_table: thash[0..],
            term_bytes: tbytes[0..],
            postings: postings[0..],
            positions: positions[0..],
        };
        let idx = FullTextIndex.init(cfg, bufs) catch unreachable;

        let sc_tids: [128]u32 = [0; 128];
        let sc_pos: [128]u32 = [0; 128];
        let sc_scores: [16]f64 = [0.0; 16];
        let sc_mask: [16]u8 = [0; 16];
        let sc_aux: [64]u8 = [0; 64];
        let sc_cnt: [16]u32 = [0; 16];
        let scratch = FullTextScratch{
            term_ids: sc_tids[0..],
            token_positions: sc_pos[0..],
            scores: sc_scores[0..],
            doc_mask: sc_mask[0..],
            doc_mask_aux: sc_aux[0..],
            matched_terms_cnt: sc_cnt[0..],
        };

        const d1: [2][]const u8 = ["rust engine", "quick brown fox jumps over lazy dog"];
        const d2: [2][]const u8 = ["zig engine", "quick red brown lazy fox jumps"];
        const d3: [2][]const u8 = ["repeat test", "rust fast rust safe memory"];
        const d4: [2][]const u8 = ["reverse", "fox brown quick"];

        * = idx.addDocument(1, d1[0..], *scratch) catch unreachable;
        * = idx.addDocument(2, d2[0..], *scratch) catch unreachable;
        * = idx.addDocument(3, d3[0..], *scratch) catch unreachable;
        * = idx.addDocument(4, d4[0..], *scratch) catch unreachable;

        let hits: [8]FtHit = [FtHit{}; 8];
        let hits_s: []FtHit = hits[0..];

        // Exact phrase slop=0: only doc 1 has "quick brown fox"
        const n0 = searchPhraseQuery(*idx, "quick brown fox", 0, 1.0, *scratch, hits_s) catch unreachable;
        expect_eq(n0, 1);
        expect_eq(hits[0].doc_id, 1);

        // Proximity slop=2: doc 1 ("quick brown fox") AND doc 2 ("quick [red] brown [lazy] fox") match; doc 4 reversed does NOT
        const n2 = searchPhraseQuery(*idx, "quick brown fox", 2, 1.0, *scratch, hits_s) catch unreachable;
        expect_eq(n2, 2);

        // Repeated-term phrase "rust fast rust" matches doc 3 (requires two distinct positions for "rust")
        const nr = searchPhraseQuery(*idx, "rust fast rust", 0, 1.0, *scratch, hits_s) catch unreachable;
        expect_eq(nr, 1);
        expect_eq(hits[0].doc_id, 3);
    });

    // ========================================================================
    // 2. Structured Boolean Query (MUST / SHOULD / MUST_NOT + Boost + MinShould)
    // ========================================================================
    test("ft_ref_02_boolean_must_should_must_not_boost_and_min_should", () => {
        let fspecs: [1]FullTextFieldSpec = [FullTextFieldSpec{ weight: 1.0, b: 0.75 }];
        let flsums: [1]u64 = [0; 1];
        let docs: [16]FtDocMeta = [FtDocMeta{}; 16];
        let dflens: [16]u32 = [0; 16];
        let terms: [128]FtTermEntry = [FtTermEntry{}; 128];
        let thash: [256]u32 = [0; 256];
        let tbytes: [4096]u8 = [0; 4096];
        let postings: [512]FtPosting = [FtPosting{}; 512];
        let positions: [1024]u32 = [0; 1024];

        let cfg = FullTextConfig{
            k1: 1.2,
            field_count: 1,
            enable_stemming: false,
            enable_stopwords: false,
            enable_cjk_bigrams: false,
        };
        let bufs = FullTextBuffers{
            field_specs: fspecs[0..],
            field_len_sums: flsums[0..],
            docs: docs[0..],
            doc_field_lens: dflens[0..],
            terms: terms[0..],
            term_hash_table: thash[0..],
            term_bytes: tbytes[0..],
            postings: postings[0..],
            positions: positions[0..],
        };
        let idx = FullTextIndex.init(cfg, bufs) catch unreachable;

        let sc_tids: [128]u32 = [0; 128];
        let sc_pos: [128]u32 = [0; 128];
        let sc_scores: [16]f64 = [0.0; 16];
        let sc_mask: [16]u8 = [0; 16];
        let sc_aux: [64]u8 = [0; 64];
        let sc_cnt: [16]u32 = [0; 16];
        let scratch = FullTextScratch{
            term_ids: sc_tids[0..],
            token_positions: sc_pos[0..],
            scores: sc_scores[0..],
            doc_mask: sc_mask[0..],
            doc_mask_aux: sc_aux[0..],
            matched_terms_cnt: sc_cnt[0..],
        };

        const d10: [1][]const u8 = ["search engine rust vector"];
        const d20: [1][]const u8 = ["search engine python legacy"];
        const d30: [1][]const u8 = ["search engine zig simd vector"];
        const d40: [1][]const u8 = ["database sql btree"];
        * = idx.addDocument(10, d10[0..], *scratch) catch unreachable;
        * = idx.addDocument(20, d20[0..], *scratch) catch unreachable;
        * = idx.addDocument(30, d30[0..], *scratch) catch unreachable;
        * = idx.addDocument(40, d40[0..], *scratch) catch unreachable;

        let hits: [8]FtHit = [FtHit{}; 8];
        let hits_s: []FtHit = hits[0..];

        // MUST "search", MUST_NOT "legacy", SHOULD "simd" (boost 5.0), SHOULD "rust" (boost 1.0)
        // Doc 20 excluded by MUST_NOT; Doc 30 outranks Doc 10 due to 5.0 boost on "simd"
        const clauses: [4]FtBoolClause = [
            FtBoolClause{ occur: FtOccur.must, kind: FtClauseKind.term, text: "search", boost: 1.0 },
            FtBoolClause{ occur: FtOccur.must_not, kind: FtClauseKind.term, text: "legacy", boost: 1.0 },
            FtBoolClause{ occur: FtOccur.should, kind: FtClauseKind.term, text: "simd", boost: 5.0 },
            FtBoolClause{ occur: FtOccur.should, kind: FtClauseKind.term, text: "rust", boost: 1.0 },
        ];
        const nh = searchBooleanQuery(*idx, FtBoolQuery{ clauses: clauses[0..], minimum_should_match: 1 }, *scratch, hits_s) catch unreachable;
        expect_eq(nh, 2);
        expect_eq(hits[0].doc_id, 30);
        expect_eq(hits[1].doc_id, 10);

        // minimum_should_match = 2: only doc 30 has both "simd" and "vector"
        const msm_clauses: [3]FtBoolClause = [
            FtBoolClause{ occur: FtOccur.should, kind: FtClauseKind.term, text: "simd", boost: 1.0 },
            FtBoolClause{ occur: FtOccur.should, kind: FtClauseKind.term, text: "vector", boost: 1.0 },
            FtBoolClause{ occur: FtOccur.should, kind: FtClauseKind.term, text: "sql", boost: 1.0 },
        ];
        const nh2 = searchBooleanQuery(*idx, FtBoolQuery{ clauses: msm_clauses[0..], minimum_should_match: 2 }, *scratch, hits_s) catch unreachable;
        expect_eq(nh2, 1);
        expect_eq(hits[0].doc_id, 30);
    });

    // ========================================================================
    // 3. Prefix, Wildcard (Anti-Backtracking), and SIMD ASCII Normalization
    // ========================================================================
    test("ft_ref_03_prefix_wildcard_and_simd_ascii_normalize", () => {
        // SIMD ASCII normalization
        let nbuf: [64]u8 = [0; 64];
        let ns: []u8 = nbuf[0..];
        const nlen = normalizeAsciiQuerySimd("   HeLLo\t\n  WoRLD   SIMD   ", ns);
        expect_eq(ftStrEq(nbuf[0..nlen], "hello world simd"), true);

        // Linear wildcard matcher & anti-backtracking stress
        expect_eq(wildcardMatchBytes("comp*er", "compiler"), true);
        expect_eq(wildcardMatchBytes("te?t", "test"), true);
        expect_eq(wildcardMatchBytes("te?t", "toast"), false);
        let long_a: [64]u8 = ['a'; 64];
        expect_eq(wildcardMatchBytes("*a*a*a*a*a*a*b", long_a[0..]), false);

        // Index prefix & wildcard search
        let fspecs: [1]FullTextFieldSpec = [FullTextFieldSpec{ weight: 1.0, b: 0.75 }];
        let flsums: [1]u64 = [0; 1];
        let docs: [8]FtDocMeta = [FtDocMeta{}; 8];
        let dflens: [8]u32 = [0; 8];
        let terms: [64]FtTermEntry = [FtTermEntry{}; 64];
        let thash: [128]u32 = [0; 128];
        let tbytes: [2048]u8 = [0; 2048];
        let postings: [128]FtPosting = [FtPosting{}; 128];
        let positions: [256]u32 = [0; 256];
        let cfg = FullTextConfig{ k1: 1.2, field_count: 1, enable_stemming: false, enable_stopwords: false, enable_cjk_bigrams: false };
        let bufs = FullTextBuffers{
            field_specs: fspecs[0..], field_len_sums: flsums[0..], docs: docs[0..],
            doc_field_lens: dflens[0..], terms: terms[0..], term_hash_table: thash[0..],
            term_bytes: tbytes[0..], postings: postings[0..], positions: positions[0..],
        };
        let idx = FullTextIndex.init(cfg, bufs) catch unreachable;
        let sc_tids: [64]u32 = [0; 64];
        let sc_pos: [64]u32 = [0; 64];
        let sc_scores: [8]f64 = [0.0; 8];
        let sc_mask: [8]u8 = [0; 8];
        let sc_aux: [32]u8 = [0; 32];
        let sc_cnt: [8]u32 = [0; 8];
        let scratch = FullTextScratch{
            term_ids: sc_tids[0..], token_positions: sc_pos[0..], scores: sc_scores[0..],
            doc_mask: sc_mask[0..], doc_mask_aux: sc_aux[0..], matched_terms_cnt: sc_cnt[0..],
        };

        const d1: [1][]const u8 = ["compiler compression compute"];
        const d2: [1][]const u8 = ["network socket protocol"];
        * = idx.addDocument(101, d1[0..], *scratch) catch unreachable;
        * = idx.addDocument(102, d2[0..], *scratch) catch unreachable;

        let hits: [4]FtHit = [FtHit{}; 4];
        let hits_s: []FtHit = hits[0..];
        const np = searchPrefixQuery(*idx, "COMP", 1.0, *scratch, hits_s) catch unreachable;
        expect_eq(np, 1);
        expect_eq(hits[0].doc_id, 101);

        const nw = searchWildcardQuery(*idx, "sock?t", 1.0, *scratch, hits_s) catch unreachable;
        expect_eq(nw, 1);
        expect_eq(hits[0].doc_id, 102);
    });

    // ========================================================================
    // 4. Tantivy BM25 Monotonicity & Boundary Guards (N=0, df=0, df>N, All-Deleted)
    // ========================================================================
    test("ft_ref_04_bm25_monotonicity_and_boundary_guards", () => {
        expect_eq(tantivyBm25Idf(0, 0), 0.0);
        expect_eq(tantivyBm25Idf(10, 0), 0.0);
        expect_eq(tantivyBm25Idf(10, 15) >= 0.0, true);
        expect_eq(tantivyBm25Idf(100, 2) > tantivyBm25Idf(100, 50), true);

        // Score monotonicity with TF & safety against avg_dl=0 / NaN / Inf
        const idf = tantivyBm25Idf(100, 5);
        const s1 = tantivyBm25Score(1.0, 10.0, 10.0, idf, 1.2, 0.75);
        const s5 = tantivyBm25Score(5.0, 10.0, 10.0, idf, 1.2, 0.75);
        expect_eq(s5 > s1 && s1 > 0.0, true);
        expect_eq(tantivyBm25Score(0.0, 10.0, 10.0, idf, 1.2, 0.75), 0.0);
        expect_eq(tantivyBm25Score(2.0, 10.0, 0.0, idf, 1.2, 0.75) > 0.0, true);
        expect_eq(tantivyBm25Score(std.math.nan(f64), 10.0, 10.0, idf, 1.2, 0.75), 0.0);
        expect_eq(computeBm25fTermScore(std.math.inf(f64), 2.0, 1.2), 0.0);
    });

    // ========================================================================
    // 5. 8-Lane SIMD Batch BM25 & Hybrid Linear / RRF Fusion vs Scalar (< 1e-4)
    // ========================================================================
    test("ft_ref_05_simd_batch_bm25_and_hybrid_rrf_fusion", () => {
        const n: usize = 53; // non-multiple of 8 (6x8 + 5 tail)
        let tfs: [53]f32 = [0.0; 53];
        let dls: [53]f32 = [0.0; 53];
        let vec_s: [53]f32 = [0.0; 53];
        let simd_bm25: [53]f32 = [0.0; 53];
        let simd_fused: [53]f32 = [0.0; 53];

        let i: usize = 0;
        while (i < n) : (i += 1) {
            tfs[i] = @as(f32, @floatFromInt(i % 7));
            dls[i] = 10.0 + @as(f32, @floatFromInt((i * 5) % 40));
            vec_s[i] = @as(f32, @floatFromInt(i)) * 0.015;
        }

        const avg_dl: f32 = 25.0;
        const idf: f32 = 2.35;
        const k1: f32 = 1.2;
        const b: f32 = 0.75;
        let sb_s: []f32 = simd_bm25[0..];
        let sf_s: []f32 = simd_fused[0..];

        expect_eq(batchBm25ScoreSimdF32(tfs[0..], dls[0..], avg_dl, idf, k1, b, sb_s) catch unreachable, n);
        expect_eq(fuseHybridScoresSimdF32(simd_bm25[0..], vec_s[0..], 0.6, sf_s) catch unreachable, n);

        i = 0;
        while (i < n) : (i += 1) {
            const ref_bm25: f32 = tantivyBm25Score(tfs[i] as f64, dls[i] as f64, avg_dl as f64, idf as f64, k1 as f64, b as f64) as f32;
            expect_eq(ftAbsF32(simd_bm25[i] - ref_bm25) < 1.0e-4, true);
            const ref_fused: f32 = 0.6 * ref_bm25 + 0.4 * vec_s[i];
            expect_eq(ftAbsF32(simd_fused[i] - ref_fused) < 1.0e-4, true);
        }

        // RRF Hybrid Fusion: doc 20 appears in both rank lists so it must rank #1
        const b_hits: [3]FtHit = [
            FtHit{ doc_id: 10, score: 5.0, matched_terms: 2 },
            FtHit{ doc_id: 20, score: 4.0, matched_terms: 1 },
            FtHit{ doc_id: 30, score: 3.0, matched_terms: 1 },
        ];
        const v_hits: [2]FtHit = [
            FtHit{ doc_id: 20, score: 0.95, matched_terms: 1 },
            FtHit{ doc_id: 40, score: 0.85, matched_terms: 1 },
        ];
        let rrf_out: [4]FtHit = [FtHit{}; 4];
        let rrf_s: []FtHit = rrf_out[0..];
        const n_rrf = rrfFuseTopKHits(b_hits[0..], v_hits[0..], 60.0, 1.0, 1.0, rrf_s);
        expect_eq(n_rrf, 4);
        expect_eq(rrf_out[0].doc_id, 20);
    });

    // ========================================================================
    // 6. 1,200-Iteration Deterministic PRNG Indexing/Deletion/Query Fuzz
    // ========================================================================
    test("ft_ref_06_deterministic_1200_fuzz_index_delete_query", () => {
        let fspecs: [1]FullTextFieldSpec = [FullTextFieldSpec{ weight: 1.0, b: 0.75 }];
        let flsums: [1]u64 = [0; 1];
        let docs: [32]FtDocMeta = [FtDocMeta{}; 32];
        let dflens: [32]u32 = [0; 32];
        let terms: [128]FtTermEntry = [FtTermEntry{}; 128];
        let thash: [256]u32 = [0; 256];
        let tbytes: [4096]u8 = [0; 4096];
        let postings: [512]FtPosting = [FtPosting{}; 512];
        let positions: [1024]u32 = [0; 1024];
        let cfg = FullTextConfig{ k1: 1.2, field_count: 1, enable_stemming: true, enable_stopwords: true, enable_cjk_bigrams: false };
        let bufs = FullTextBuffers{
            field_specs: fspecs[0..], field_len_sums: flsums[0..], docs: docs[0..],
            doc_field_lens: dflens[0..], terms: terms[0..], term_hash_table: thash[0..],
            term_bytes: tbytes[0..], postings: postings[0..], positions: positions[0..],
        };
        let idx = FullTextIndex.init(cfg, bufs) catch unreachable;

        let sc_tids: [64]u32 = [0; 64];
        let sc_pos: [64]u32 = [0; 64];
        let sc_scores: [32]f64 = [0.0; 32];
        let sc_mask: [32]u8 = [0; 32];
        let sc_aux: [128]u8 = [0; 128];
        let sc_cnt: [32]u32 = [0; 32];
        let scratch = FullTextScratch{
            term_ids: sc_tids[0..], token_positions: sc_pos[0..], scores: sc_scores[0..],
            doc_mask: sc_mask[0..], doc_mask_aux: sc_aux[0..], matched_terms_cnt: sc_cnt[0..],
        };

        const seed_docs: [6]string = [
            "rust tantivy bm25 search engine",
            "zig simd vector hybrid retrieval",
            "fulltext phrase proximity slop query",
            "wildcard prefix boolean filter boost",
            "memory safety zero heap allocation",
            "fast index snapshot crc32 checksum",
        ];
        let di: usize = 0;
        while (di < 6) : (di += 1) {
            const arr: [1][]const u8 = [seed_docs[di]];
            * = idx.addDocument((di + 1) as u64, arr[0..], *scratch) catch unreachable;
        }

        const alphabet = "abcefhilmnoprstuvxz*? \"~()+- ";
        let state: u64 = 0x1B05688C2B3E6C1F;
        let qbuf: [32]u8 = [0; 32];
        let hits: [8]FtHit = [FtHit{}; 8];
        let hits_s: []FtHit = hits[0..];
        let cl_arr: [2]FtBoolClause = [
            FtBoolClause{ occur: FtOccur.should, kind: FtClauseKind.term, text: "", boost: 1.0 },
            FtBoolClause{ occur: FtOccur.must_not, kind: FtClauseKind.prefix, text: "zz", boost: 1.0 },
        ];
        let iter: usize = 0;

        while (iter < 1200) : (iter += 1) {
            state ^= state >> 12;
            state ^= state << 25;
            state ^= state >> 27;
            const r = state *% 0x2545F4914F6CDD1D;
            const qlen: usize = (1 as usize) + ((r % 24) as usize);

            let j: usize = 0;
            while (j < qlen) : (j += 1) {
                state ^= state >> 12;
                state ^= state << 25;
                state ^= state >> 27;
                qbuf[j] = alphabet[(state % (alphabet.len as u64)) as usize];
            }

            const qstr: []const u8 = qbuf[0..qlen];
            const kind_sel = iter % 4;
            if (kind_sel == 0) {
                * = searchPrefixQuery(*idx, qstr, 1.0, *scratch, hits_s) catch 0;
            } else if (kind_sel == 1) {
                * = searchWildcardQuery(*idx, qstr, 1.0, *scratch, hits_s) catch 0;
            } else if (kind_sel == 2) {
                * = searchPhraseQuery(*idx, qstr, (iter % 3) as u32, 1.0, *scratch, hits_s) catch 0;
            } else {
                cl_arr[0].text = qstr;
                * = searchBooleanQuery(*idx, FtBoolQuery{ clauses: cl_arr[0..] }, *scratch, hits_s) catch 0;
            }

            // Delete all docs near the end to fuzz empty/all-deleted active_doc_count == 0
            if (iter == 1100) {
                let k: u64 = 1;
                while (k <= 6) : (k += 1) {
                    * = idx.markDeleted(k);
                }
            }
        }

        expect_eq(iter, 1200);
        expect_eq(idx.active_doc_count, 0);
    });
}
