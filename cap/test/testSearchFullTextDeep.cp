// Deep Production Test Suite for lib/vector/fullTextIndex.cp (Wave 27 Round 1)
// Verifies: multilingual tokenizer (ASCII + CJK unigram/2-gram + stopwords + Porter-lite stemmer),
// exact multi-field BM25F formula parity (< 1e-6), recursive-descent boolean queries,
// positional phrase & slop proximity, prefix & Damerau-Levenshtein fuzzy search,
// soft-deletion tombstones, dense-window <mark> snippet extraction,
// CRC32 snapshot round-trip + corruption rejection, and >200-doc scale beyond legacy 128 limit.

import { test, expect_eq, expect_true } from "cap:test";
import {
    FT_SNAPSHOT_MAGIC,
    FT_SNAPSHOT_VERSION,
    FullTextError,
    FullTextFieldSpec,
    FullTextConfig,
    FtDocMeta,
    FtTermEntry,
    FtPosting,
    FtHit,
    FtStats,
    FtTokenSpan,
    FullTextBuffers,
    FullTextScratch,
    ftDocFieldLensMin,
    ftTermHashTableMin,
    isCjkCodepoint,
    isEnglishStopword,
    porterLiteStemInPlace,
    tokenizeText,
    damerauLevenshteinBounded,
    computeBm25fIdf,
    computeBm25fTermScore,
    FullTextIndex,
    extractBestSnippet,
} from "../../lib/vector/index.cp";

const std = @import("std");

fn absDiffF64(a: f64, b: f64): f64 {
    return if (a > b) a - b else b - a;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        let ok: bool = true;
        let j: usize = 0;
        while (j < needle.len) : (j += 1) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Multilingual Tokenizer, Stopwords, Porter-Lite Stemmer & CJK 2-grams
// ============================================================================
test("FullTextIndex: multilingual tokenizer, Porter-lite stemming & CJK bigrams") {
    // Verify stopword & CJK classification
    expect_true(isEnglishStopword("the"));
    expect_true(isEnglishStopword("with"));
    expect_true(!isEnglishStopword("engine"));
    expect_true(isCjkCodepoint(0x641C)); // '搜'
    expect_true(!isCjkCodepoint('A'));

    // Verify Porter-lite stemmer reductions
    let w1: [16]u8 = [0; 16];
    @memcpy(w1[0..7], "running");
    let w1_s: []u8 = w1[0..];
    const l1: usize = porterLiteStemInPlace(*w1_s, 7);
    expect_true(strEq(w1[0..l1], "run"));

    let w2: [16]u8 = [0; 16];
    @memcpy(w2[0..9], "searching");
    let w2_s: []u8 = w2[0..];
    const l2: usize = porterLiteStemInPlace(*w2_s, 9);
    expect_true(strEq(w2[0..l2], "search"));

    let w3: [16]u8 = [0; 16];
    @memcpy(w3[0..7], "studies");
    let w3_s: []u8 = w3[0..];
    const l3: usize = porterLiteStemInPlace(*w3_s, 7);
    expect_true(strEq(w3[0..l3], "studi"));

    let w4: [16]u8 = [0; 16];
    @memcpy(w4[0..7], "indexed");
    let w4_s: []u8 = w4[0..];
    const l4: usize = porterLiteStemInPlace(*w4_s, 7);
    expect_true(strEq(w4[0..l4], "index"));

    // Mixed English + CJK + punctuation tokenization
    const sample: []const u8 = "The Engines are searching: 搜索引擎!";
    let spans: [64]FtTokenSpan = [FtTokenSpan{}; 64];
    let spans_s: []FtTokenSpan = spans[0..];
    const cnt: usize = tokenizeText(sample, true, true, true, *spans_s);

    // Expect: "engin", "search", then CJK unigrams & 2-grams for 搜/索/引/擎 (4 unigrams + 3 bigrams = 7) => total 9
    expect_eq(cnt, 9);
    expect_true(strEq(spans[0].text(), "engin"));
    expect_true(strEq(spans[1].text(), "search"));
    expect_true(strEq(spans[2].text(), "搜"));
    expect_true(!spans[2].is_bigram);
    expect_true(strEq(spans[3].text(), "索"));
    expect_true(strEq(spans[4].text(), "搜索"));
    expect_true(spans[4].is_bigram);
    expect_true(strEq(spans[8].text(), "引擎"));
    expect_true(spans[8].is_bigram);
}

// ============================================================================
// Test 2: Exact Multi-Field BM25F Formula Verification (error < 1e-6)
// ============================================================================
test("FullTextIndex: exact multi-field BM25F formula parity (< 1e-6)") {
    let field_specs: [3]FullTextFieldSpec = [
        FullTextFieldSpec{ weight: 3.0, b: 0.50 }, // title
        FullTextFieldSpec{ weight: 1.0, b: 0.75 }, // body
        FullTextFieldSpec{ weight: 2.0, b: 0.40 }, // tags
    ];
    let field_sums: [3]u64 = [0; 3];
    let docs: [8]FtDocMeta = [FtDocMeta{}; 8];
    let doc_flens: [24]u32 = [0; 24];
    let terms: [128]FtTermEntry = [FtTermEntry{}; 128];
    let hash_tbl: [256]u32 = [0; 256];
    let term_bytes: [2048]u8 = [0; 2048];
    let postings: [256]FtPosting = [FtPosting{}; 256];
    let positions: [512]u32 = [0; 512];

    const bufs: FullTextBuffers = FullTextBuffers{
        field_specs: field_specs[0..],
        field_len_sums: field_sums[0..],
        docs: docs[0..],
        doc_field_lens: doc_flens[0..],
        terms: terms[0..],
        term_hash_table: hash_tbl[0..],
        term_bytes: term_bytes[0..],
        postings: postings[0..],
        positions: positions[0..],
    };

    const cfg: FullTextConfig = FullTextConfig{
        k1: 1.5,
        field_count: 3,
        enable_stemming: false,
        enable_stopwords: false,
        enable_cjk_bigrams: false,
    };
    let idx: FullTextIndex = FullTextIndex.init(cfg, bufs) catch {
        expect_true(false);
        return;
    };

    let sc_tids: [128]u32 = [0; 128];
    let sc_pos: [128]u32 = [0; 128];
    let sc_scores: [8]f64 = [0.0; 8];
    let sc_mask: [8]u8 = [0; 8];
    let sc_aux: [64]u8 = [0; 64];
    let sc_cnt: [8]u32 = [0; 8];
    let scratch: FullTextScratch = FullTextScratch{
        term_ids: sc_tids[0..],
        token_positions: sc_pos[0..],
        scores: sc_scores[0..],
        doc_mask: sc_mask[0..],
        doc_mask_aux: sc_aux[0..],
        matched_terms_cnt: sc_cnt[0..],
    };

    // 4 docs with known exact field lengths:
    // Doc 10: title="alpha beta" (2), body="alpha gamma delta epsilon" (4), tags="alpha" (1)
    // Doc 20: title="gamma delta" (2), body="alpha alpha alpha beta" (4), tags="beta" (1)
    // Doc 30: title="epsilon zeta eta theta" (4), body="iota kappa" (2), tags="lambda mu" (2)
    // Doc 40: title="nu xi" (2), body="omicron pi" (2), tags="rho sigma tau upsilon" (4)
    const d1: [3][]const u8 = ["alpha beta", "alpha gamma delta epsilon", "alpha"];
    const d2: [3][]const u8 = ["gamma delta", "alpha alpha alpha beta", "beta"];
    const d3: [3][]const u8 = ["epsilon zeta eta theta", "iota kappa", "lambda mu"];
    const d4: [3][]const u8 = ["nu xi", "omicron pi", "rho sigma tau upsilon"];
    * = idx.addDocument(10, d1[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(20, d2[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(30, d3[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(40, d4[0..], *scratch) catch { expect_true(false); return; };

    // Hand-computed reference for query "alpha":
    // N = 4, df("alpha") = 2 (docs 10 and 20)
    // avgdl: title = (2+2+4+2)/4 = 2.5, body = (4+4+2+2)/4 = 3.0, tags = (1+1+2+4)/4 = 2.0
    const expected_idf: f64 = std.math.log(f64, std.math.e, 1.0 + (4.0 - 2.0 + 0.5) / (2.0 + 0.5));
    expect_true(absDiffF64(computeBm25fIdf(4, 2), expected_idf) < 1.0e-12);

    // Doc 10: title tf=1, dl=2 (norm = 0.5 + 0.5*(2/2.5)=0.9), body tf=1, dl=4 (norm = 0.25 + 0.75*(4/3)=1.25), tags tf=1, dl=1 (norm = 0.6 + 0.4*(1/2)=0.8)
    const ptf_10: f64 = 3.0 * (1.0 / 0.9) + 1.0 * (1.0 / 1.25) + 2.0 * (1.0 / 0.8);
    const expected_s10: f64 = expected_idf * (ptf_10 * 2.5) / (ptf_10 + 1.5);

    // Doc 20: title tf=0, body tf=3, dl=4 (norm=1.25), tags tf=0
    const ptf_20: f64 = 1.0 * (3.0 / 1.25);
    const expected_s20: f64 = expected_idf * (ptf_20 * 2.5) / (ptf_20 + 1.5);

    let hits: [4]FtHit = [FtHit{}; 4];
    let hits_s: []FtHit = hits[0..];
    const n: usize = idx.searchQuery("alpha", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n, 2);
    expect_eq(hits[0].doc_id, 10);
    expect_eq(hits[1].doc_id, 20);
    expect_true(absDiffF64(hits[0].score, expected_s10) < 1.0e-6);
    expect_true(absDiffF64(hits[1].score, expected_s20) < 1.0e-6);
    expect_true(hits[0].score > hits[1].score);
}

// ============================================================================
// Test 3: Boolean Queries (AND / OR / NOT / Parentheses Precedence / Errors)
// ============================================================================
test("FullTextIndex: boolean AND/OR/NOT, parentheses precedence & malformed query guard") {
    let field_specs: [2]FullTextFieldSpec = [
        FullTextFieldSpec{ weight: 2.0, b: 0.75 },
        FullTextFieldSpec{ weight: 1.0, b: 0.75 },
    ];
    let field_sums: [2]u64 = [0; 2];
    let docs: [16]FtDocMeta = [FtDocMeta{}; 16];
    let doc_flens: [32]u32 = [0; 32];
    let terms: [256]FtTermEntry = [FtTermEntry{}; 256];
    let hash_tbl: [512]u32 = [0; 512];
    let term_bytes: [4096]u8 = [0; 4096];
    let postings: [512]FtPosting = [FtPosting{}; 512];
    let positions: [1024]u32 = [0; 1024];

    const bufs: FullTextBuffers = FullTextBuffers{
        field_specs: field_specs[0..],
        field_len_sums: field_sums[0..],
        docs: docs[0..],
        doc_field_lens: doc_flens[0..],
        terms: terms[0..],
        term_hash_table: hash_tbl[0..],
        term_bytes: term_bytes[0..],
        postings: postings[0..],
        positions: positions[0..],
    };

    let idx: FullTextIndex = FullTextIndex.init(FullTextConfig{ k1: 1.2, field_count: 2 }, bufs) catch {
        expect_true(false);
        return;
    };

    let sc_tids: [128]u32 = [0; 128];
    let sc_pos: [128]u32 = [0; 128];
    let sc_scores: [16]f64 = [0.0; 16];
    let sc_mask: [16]u8 = [0; 16];
    let sc_aux: [256]u8 = [0; 256];
    let sc_cnt: [16]u32 = [0; 16];
    let scratch: FullTextScratch = FullTextScratch{
        term_ids: sc_tids[0..],
        token_positions: sc_pos[0..],
        scores: sc_scores[0..],
        doc_mask: sc_mask[0..],
        doc_mask_aux: sc_aux[0..],
        matched_terms_cnt: sc_cnt[0..],
    };

    const doc1: [2][]const u8 = ["Rust Systems", "rust memory safety native"];
    const doc2: [2][]const u8 = ["Zig Compiler", "zig comptime native legacy"];
    const doc3: [2][]const u8 = ["Python Runtime", "python scripting legacy"];
    const doc4: [2][]const u8 = ["Rust and Zig", "rust zig beam actor"];
    const doc5: [2][]const u8 = ["Go Server", "go concurrency network"];
    * = idx.addDocument(1, doc1[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(2, doc2[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(3, doc3[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(4, doc4[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(5, doc5[0..], *scratch) catch { expect_true(false); return; };

    let hits: [8]FtHit = [FtHit{}; 8];
    let hits_s: []FtHit = hits[0..];

    // (rust OR zig) AND NOT (python OR legacy) -> docs 1 and 4 (doc 2 excluded by legacy, doc 3 by python/legacy)
    const n1: usize = idx.searchQuery("(rust OR zig) AND NOT (python OR legacy)", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n1, 2);
    expect_true((hits[0].doc_id == 1 && hits[1].doc_id == 4) || (hits[0].doc_id == 4 && hits[1].doc_id == 1));

    // Precedence check: `go OR rust AND zig` parses as `go OR (rust AND zig)` -> docs 4 and 5 (NOT doc 1)
    const n2: usize = idx.searchQuery("go OR rust AND zig", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n2, 2);
    expect_true((hits[0].doc_id == 4 && hits[1].doc_id == 5) || (hits[0].doc_id == 5 && hits[1].doc_id == 4));

    // Unary + and - syntax: `+native -legacy` -> only doc 1
    const n3: usize = idx.searchQuery("+native -legacy", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n3, 1);
    expect_eq(hits[0].doc_id, 1);

    // Malformed queries (unclosed parenthesis / unclosed quote) must return InvalidQuery
    let got_bad_paren: bool = false;
    * = idx.searchQuery("(rust AND zig", *scratch, *hits_s) catch |e| {
        if (e == FullTextError.InvalidQuery) got_bad_paren = true;
    };
    expect_true(got_bad_paren);

    let got_bad_quote: bool = false;
    * = idx.searchQuery("\"unterminated phrase", *scratch, *hits_s) catch |e| {
        if (e == FullTextError.InvalidQuery) got_bad_quote = true;
    };
    expect_true(got_bad_quote);
}

// ============================================================================
// Test 4: Exact Phrase & Proximity Slop Sliding Window ("quick brown fox"~slop)
// ============================================================================
test("FullTextIndex: positional phrase & slop proximity matching") {
    let field_specs: [2]FullTextFieldSpec = [
        FullTextFieldSpec{ weight: 2.0, b: 0.75 },
        FullTextFieldSpec{ weight: 1.0, b: 0.75 },
    ];
    let field_sums: [2]u64 = [0; 2];
    let docs: [16]FtDocMeta = [FtDocMeta{}; 16];
    let doc_flens: [32]u32 = [0; 32];
    let terms: [256]FtTermEntry = [FtTermEntry{}; 256];
    let hash_tbl: [512]u32 = [0; 512];
    let term_bytes: [4096]u8 = [0; 4096];
    let postings: [512]FtPosting = [FtPosting{}; 512];
    let positions: [1024]u32 = [0; 1024];

    const bufs: FullTextBuffers = FullTextBuffers{
        field_specs: field_specs[0..],
        field_len_sums: field_sums[0..],
        docs: docs[0..],
        doc_field_lens: doc_flens[0..],
        terms: terms[0..],
        term_hash_table: hash_tbl[0..],
        term_bytes: term_bytes[0..],
        postings: postings[0..],
        positions: positions[0..],
    };

    let idx: FullTextIndex = FullTextIndex.init(FullTextConfig{ k1: 1.2, field_count: 2 }, bufs) catch {
        expect_true(false);
        return;
    };

    let sc_tids: [128]u32 = [0; 128];
    let sc_pos: [128]u32 = [0; 128];
    let sc_scores: [16]f64 = [0.0; 16];
    let sc_mask: [16]u8 = [0; 16];
    let sc_aux: [256]u8 = [0; 256];
    let sc_cnt: [16]u32 = [0; 16];
    let scratch: FullTextScratch = FullTextScratch{
        term_ids: sc_tids[0..],
        token_positions: sc_pos[0..],
        scores: sc_scores[0..],
        doc_mask: sc_mask[0..],
        doc_mask_aux: sc_aux[0..],
        matched_terms_cnt: sc_cnt[0..],
    };

    // Doc 101: exact phrase "quick brown fox" (slop 0)
    const d101: [2][]const u8 = ["Story", "quick brown fox jumps high"];
    // Doc 102: 1 word between quick and brown (slop 1)
    const d102: [2][]const u8 = ["Story", "quick red brown fox jumps"];
    // Doc 103: 2 words gap (slop 2)
    const d103: [2][]const u8 = ["Story", "quick big red brown fox"];
    // Doc 104: split across title & body -> must NOT match phrase in same field
    const d104: [2][]const u8 = ["quick brown", "fox runs away"];
    // Doc 105: swapped adjacent words "brown quick" (requires slop >= 2 for transposition)
    const d105: [2][]const u8 = ["Story", "brown quick fox runs"];

    * = idx.addDocument(101, d101[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(102, d102[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(103, d103[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(104, d104[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(105, d105[0..], *scratch) catch { expect_true(false); return; };

    let hits: [8]FtHit = [FtHit{}; 8];
    let hits_s: []FtHit = hits[0..];

    // Exact phrase slop=0: only Doc 101
    const n0: usize = idx.searchQuery("\"quick brown fox\"", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n0, 1);
    expect_eq(hits[0].doc_id, 101);

    // Slop=1: Docs 101 and 102
    const n1: usize = idx.searchQuery("\"quick brown fox\"~1", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n1, 2);

    // Slop=2: Docs 101, 102, 103, 105 (still excludes cross-field Doc 104)
    const n2: usize = idx.searchQuery("\"quick brown fox\"~2", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n2, 4);
    let i: usize = 0;
    while (i < n2) : (i += 1) {
        expect_true(hits[i].doc_id != 104);
    }
}

// ============================================================================
// Test 5: Prefix Matching & Damerau-Levenshtein Fuzzy Correction
// ============================================================================
test("FullTextIndex: prefix wildcard & Damerau-Levenshtein fuzzy correction") {
    // Direct edit distance unit checks
    expect_eq(damerauLevenshteinBounded("algorithm", "algorithm", 2), 0);
    expect_eq(damerauLevenshteinBounded("algoritmh", "algorithm", 2), 1); // adjacent transposition
    expect_eq(damerauLevenshteinBounded("algorith", "algorithm", 2), 1);  // deletion
    expect_eq(damerauLevenshteinBounded("algrithm", "algorithm", 2), 1);  // omission
    expect_eq(damerauLevenshteinBounded("xyz", "algorithm", 2), 3);       // bounded early exit > 2

    let field_specs: [2]FullTextFieldSpec = [
        FullTextFieldSpec{ weight: 2.0, b: 0.75 },
        FullTextFieldSpec{ weight: 1.0, b: 0.75 },
    ];
    let field_sums: [2]u64 = [0; 2];
    let docs: [16]FtDocMeta = [FtDocMeta{}; 16];
    let doc_flens: [32]u32 = [0; 32];
    let terms: [256]FtTermEntry = [FtTermEntry{}; 256];
    let hash_tbl: [512]u32 = [0; 512];
    let term_bytes: [4096]u8 = [0; 4096];
    let postings: [512]FtPosting = [FtPosting{}; 512];
    let positions: [1024]u32 = [0; 1024];

    const bufs: FullTextBuffers = FullTextBuffers{
        field_specs: field_specs[0..],
        field_len_sums: field_sums[0..],
        docs: docs[0..],
        doc_field_lens: doc_flens[0..],
        terms: terms[0..],
        term_hash_table: hash_tbl[0..],
        term_bytes: term_bytes[0..],
        postings: postings[0..],
        positions: positions[0..],
    };

    let idx: FullTextIndex = FullTextIndex.init(FullTextConfig{ k1: 1.2, field_count: 2 }, bufs) catch {
        expect_true(false);
        return;
    };

    let sc_tids: [128]u32 = [0; 128];
    let sc_pos: [128]u32 = [0; 128];
    let sc_scores: [16]f64 = [0.0; 16];
    let sc_mask: [16]u8 = [0; 16];
    let sc_aux: [256]u8 = [0; 256];
    let sc_cnt: [16]u32 = [0; 16];
    let scratch: FullTextScratch = FullTextScratch{
        term_ids: sc_tids[0..],
        token_positions: sc_pos[0..],
        scores: sc_scores[0..],
        doc_mask: sc_mask[0..],
        doc_mask_aux: sc_aux[0..],
        matched_terms_cnt: sc_cnt[0..],
    };

    const d1: [2][]const u8 = ["Compiler Design", "quantum algorithm optimization"];
    const d2: [2][]const u8 = ["Compression", "lz4 zstd compression ratio"];
    const d3: [2][]const u8 = ["Compute Shader", "gpu compute pipeline"];
    const d4: [2][]const u8 = ["Database", "btree storage layout"];

    * = idx.addDocument(1, d1[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(2, d2[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(3, d3[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(4, d4[0..], *scratch) catch { expect_true(false); return; };

    let hits: [8]FtHit = [FtHit{}; 8];
    let hits_s: []FtHit = hits[0..];

    // Prefix query `comp*` matches docs 1, 2, 3 (excludes doc 4)
    const np: usize = idx.searchQuery("comp*", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(np, 3);
    let i: usize = 0;
    while (i < np) : (i += 1) {
        expect_true(hits[i].doc_id != 4);
    }

    // Fuzzy query with adjacent transposition typo `algoritmh~1` -> matches doc 1
    const nf1: usize = idx.searchQuery("algoritmh~1", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(nf1, 1);
    expect_eq(hits[0].doc_id, 1);

    // Fuzzy query with 2 edits `qautnum~2` -> matches "quantum" in doc 1
    const nf2: usize = idx.searchFuzzy("qautnum", 2, *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(nf2, 1);
    expect_eq(hits[0].doc_id, 1);
}

// ============================================================================
// Test 6: Soft-Deletion Tombstone & Dense-Window <mark> Snippet Highlighter
// ============================================================================
test("FullTextIndex: tombstone soft deletion & dense-window snippet highlighting") {
    let field_specs: [2]FullTextFieldSpec = [
        FullTextFieldSpec{ weight: 2.0, b: 0.75 },
        FullTextFieldSpec{ weight: 1.0, b: 0.75 },
    ];
    let field_sums: [2]u64 = [0; 2];
    let docs: [8]FtDocMeta = [FtDocMeta{}; 8];
    let doc_flens: [16]u32 = [0; 16];
    let terms: [128]FtTermEntry = [FtTermEntry{}; 128];
    let hash_tbl: [256]u32 = [0; 256];
    let term_bytes: [2048]u8 = [0; 2048];
    let postings: [256]FtPosting = [FtPosting{}; 256];
    let positions: [512]u32 = [0; 512];

    const bufs: FullTextBuffers = FullTextBuffers{
        field_specs: field_specs[0..],
        field_len_sums: field_sums[0..],
        docs: docs[0..],
        doc_field_lens: doc_flens[0..],
        terms: terms[0..],
        term_hash_table: hash_tbl[0..],
        term_bytes: term_bytes[0..],
        postings: postings[0..],
        positions: positions[0..],
    };

    let idx: FullTextIndex = FullTextIndex.init(FullTextConfig{ k1: 1.2, field_count: 2 }, bufs) catch {
        expect_true(false);
        return;
    };

    let sc_tids: [128]u32 = [0; 128];
    let sc_pos: [128]u32 = [0; 128];
    let sc_scores: [8]f64 = [0.0; 8];
    let sc_mask: [8]u8 = [0; 8];
    let sc_aux: [128]u8 = [0; 128];
    let sc_cnt: [8]u32 = [0; 8];
    let scratch: FullTextScratch = FullTextScratch{
        term_ids: sc_tids[0..],
        token_positions: sc_pos[0..],
        scores: sc_scores[0..],
        doc_mask: sc_mask[0..],
        doc_mask_aux: sc_aux[0..],
        matched_terms_cnt: sc_cnt[0..],
    };

    const d1: [2][]const u8 = ["Vector Search", "hybrid bm25f retrieval engine"];
    const d2: [2][]const u8 = ["Vector Index", "zero heap bm25f search"];
    * = idx.addDocument(501, d1[0..], *scratch) catch { expect_true(false); return; };
    * = idx.addDocument(502, d2[0..], *scratch) catch { expect_true(false); return; };

    expect_true(idx.markDeleted(501));
    expect_true(idx.isDeleted(501));
    expect_true(!idx.isDeleted(502));
    expect_true(!idx.markDeleted(501)); // idempotent second delete returns false

    const st: FtStats = idx.stats();
    expect_eq(st.doc_count, 2);
    expect_eq(st.active_doc_count, 1);
    expect_eq(st.deleted_doc_count, 1);

    let hits: [4]FtHit = [FtHit{}; 4];
    let hits_s: []FtHit = hits[0..];
    const n: usize = idx.searchQuery("bm25f", *scratch, *hits_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n, 1);
    expect_eq(hits[0].doc_id, 502);

    // Dense-window snippet extraction on long English text
    const long_doc: []const u8 =
        "Introduction to distributed systems and storage engines across cloud clusters. " ++
        "In chapter four, our zero-heap BM25F engine performs exact phrase proximity and fuzzy correction " ++
        "with minimal memory overhead for real-time agent pipelines.";
    const qterms: [3][]const u8 = ["bm25f", "phrase", "fuzzy"];
    let snip_buf: [256]u8 = [0; 256];
    let snip_s: []u8 = snip_buf[0..];
    const slen: usize = extractBestSnippet(long_doc, qterms[0..], 80, "<mark>", "</mark>", true, *snip_s) catch {
        expect_true(false);
        return;
    };
    const snip: []const u8 = snip_buf[0..slen];
    expect_true(containsSubstr(snip, "<mark>BM25F</mark>"));
    expect_true(containsSubstr(snip, "<mark>phrase</mark>"));
    expect_true(containsSubstr(snip, "..."));

    // CJK snippet extraction with merged overlapping unigram+bigram highlights (no nested <mark>)
    const cjk_doc: []const u8 = "现代系统编程需要高性能的全文搜索引擎来支撑大规模知识库检索。";
    const cjk_q: [2][]const u8 = ["搜索", "引擎"];
    let cjk_buf: [256]u8 = [0; 256];
    let cjk_s: []u8 = cjk_buf[0..];
    const clen: usize = extractBestSnippet(cjk_doc, cjk_q[0..], 64, "<mark>", "</mark>", false, *cjk_s) catch {
        expect_true(false);
        return;
    };
    const cjk_snip: []const u8 = cjk_buf[0..clen];
    expect_true(containsSubstr(cjk_snip, "<mark>搜索引擎</mark>"));
    expect_true(!containsSubstr(cjk_snip, "<mark><mark>"));
}

// ============================================================================
// Test 7: >200-Doc Scale, CJK+EN Corpus & CRC32 Snapshot Round-Trip / Tamper Guard
// ============================================================================
test("FullTextIndex: 220-doc scale, CJK+EN retrieval & CRC32 snapshot round-trip") {
    const N_DOCS: usize = 220;
    const N_FIELDS: usize = 2;
    const N_TERMS: usize = 512;
    const N_HASH: usize = 1024;

    expect_eq(ftDocFieldLensMin(N_DOCS, N_FIELDS), 440);
    expect_eq(ftTermHashTableMin(N_TERMS), 1024);

    let fspecs1: [2]FullTextFieldSpec = [
        FullTextFieldSpec{ weight: 3.0, b: 0.70 },
        FullTextFieldSpec{ weight: 1.0, b: 0.75 },
    ];
    let fsums1: [2]u64 = [0; 2];
    let docs1: [220]FtDocMeta = [FtDocMeta{}; 220];
    let flens1: [440]u32 = [0; 440];
    let terms1: [512]FtTermEntry = [FtTermEntry{}; 512];
    let hash1: [1024]u32 = [0; 1024];
    let tbytes1: [8192]u8 = [0; 8192];
    let posts1: [4096]FtPosting = [FtPosting{}; 4096];
    let pos1: [8192]u32 = [0; 8192];

    const bufs1: FullTextBuffers = FullTextBuffers{
        field_specs: fspecs1[0..],
        field_len_sums: fsums1[0..],
        docs: docs1[0..],
        doc_field_lens: flens1[0..],
        terms: terms1[0..],
        term_hash_table: hash1[0..],
        term_bytes: tbytes1[0..],
        postings: posts1[0..],
        positions: pos1[0..],
    };

    let idx: FullTextIndex = FullTextIndex.init(FullTextConfig{ k1: 1.2, field_count: 2 }, bufs1) catch {
        expect_true(false);
        return;
    };

    let sc_tids: [128]u32 = [0; 128];
    let sc_pos: [128]u32 = [0; 128];
    let sc_scores: [220]f64 = [0.0; 220];
    let sc_mask: [220]u8 = [0; 220];
    let sc_aux: [1760]u8 = [0; 1760];
    let sc_cnt: [220]u32 = [0; 220];
    let scratch: FullTextScratch = FullTextScratch{
        term_ids: sc_tids[0..],
        token_positions: sc_pos[0..],
        scores: sc_scores[0..],
        doc_mask: sc_mask[0..],
        doc_mask_aux: sc_aux[0..],
        matched_terms_cnt: sc_cnt[0..],
    };

    // Index 220 documents (exceeding legacy 128 fixed cap) with English + CJK mixed corpus
    let d: usize = 0;
    while (d < N_DOCS) : (d += 1) {
        if (d == 177) {
            const special: [2][]const u8 = ["全文搜索引擎 Architecture", "zero heap bm25f 搜索引擎 snapshot"];
            * = idx.addDocument(1000 + @as(u64, @intCast(d)), special[0..], *scratch) catch {
                expect_true(false);
                return;
            };
        } else if ((d % 3) == 0) {
            const f: [2][]const u8 = ["Compiler Notes", "zig native state machine runtime"];
            * = idx.addDocument(1000 + @as(u64, @intCast(d)), f[0..], *scratch) catch {
                expect_true(false);
                return;
            };
        } else if ((d % 3) == 1) {
            const f: [2][]const u8 = ["分布式检索", "向量数据库与倒排索引混合架构"];
            * = idx.addDocument(1000 + @as(u64, @intCast(d)), f[0..], *scratch) catch {
                expect_true(false);
                return;
            };
        } else {
            const f: [2][]const u8 = ["Storage Engine", "wal compaction checksum recovery"];
            * = idx.addDocument(1000 + @as(u64, @intCast(d)), f[0..], *scratch) catch {
                expect_true(false);
                return;
            };
        }
    }

    // Soft-delete one doc before snapshot
    expect_true(idx.markDeleted(1000));
    expect_eq(idx.stats().doc_count, 220);
    expect_eq(idx.stats().active_doc_count, 219);

    // Query before snapshot
    let hits_before: [4]FtHit = [FtHit{}; 4];
    let hb_s: []FtHit = hits_before[0..];
    const nb: usize = idx.searchQuery("搜索引擎 AND bm25f", *scratch, *hb_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(nb, 1);
    expect_eq(hits_before[0].doc_id, 1177);

    // Serialize CRC32 binary snapshot
    const need_bytes: usize = idx.snapshotByteSize();
    let snap_buf: [131072]u8 = [0; 131072];
    expect_true(need_bytes <= snap_buf.len);
    let snap_s: []u8 = snap_buf[0..];
    const written: usize = idx.serializeSnapshot(*snap_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(written, need_bytes);

    // Deserialize into completely fresh external buffers
    let fspecs2: [2]FullTextFieldSpec = [FullTextFieldSpec{}; 2];
    let fsums2: [2]u64 = [0; 2];
    let docs2: [220]FtDocMeta = [FtDocMeta{}; 220];
    let flens2: [440]u32 = [0; 440];
    let terms2: [512]FtTermEntry = [FtTermEntry{}; 512];
    let hash2: [1024]u32 = [0; 1024];
    let tbytes2: [8192]u8 = [0; 8192];
    let posts2: [4096]FtPosting = [FtPosting{}; 4096];
    let pos2: [8192]u32 = [0; 8192];

    const bufs2: FullTextBuffers = FullTextBuffers{
        field_specs: fspecs2[0..],
        field_len_sums: fsums2[0..],
        docs: docs2[0..],
        doc_field_lens: flens2[0..],
        terms: terms2[0..],
        term_hash_table: hash2[0..],
        term_bytes: tbytes2[0..],
        postings: posts2[0..],
        positions: pos2[0..],
    };

    const reloaded: FullTextIndex = FullTextIndex.deserializeSnapshot(snap_buf[0..written], bufs2) catch {
        expect_true(false);
        return;
    };

    expect_eq(reloaded.stats().doc_count, 220);
    expect_eq(reloaded.stats().active_doc_count, 219);
    expect_eq(reloaded.stats().term_count, idx.stats().term_count);
    expect_true(reloaded.isDeleted(1000));

    let hits_after: [4]FtHit = [FtHit{}; 4];
    let ha_s: []FtHit = hits_after[0..];
    const na: usize = reloaded.searchQuery("搜索引擎 AND bm25f", *scratch, *ha_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(na, 1);
    expect_eq(hits_after[0].doc_id, 1177);
    expect_true(absDiffF64(hits_after[0].score, hits_before[0].score) < 1.0e-12);

    // Single-bit corruption guard -> ChecksumMismatch
    snap_buf[written / 2] ^= 0x01;
    let got_crc_err: bool = false;
    * = FullTextIndex.deserializeSnapshot(snap_buf[0..written], bufs2) catch |e| {
        if (e == FullTextError.ChecksumMismatch) got_crc_err = true;
    };
    expect_true(got_crc_err);

    // Truncated snapshot guard -> InvalidSnapshot
    let got_trunc_err: bool = false;
    * = FullTextIndex.deserializeSnapshot(snap_buf[0..32], bufs2) catch |e| {
        if (e == FullTextError.InvalidSnapshot) got_trunc_err = true;
    };
    expect_true(got_trunc_err);

    * = FT_SNAPSHOT_MAGIC;
    * = FT_SNAPSHOT_VERSION;
}
