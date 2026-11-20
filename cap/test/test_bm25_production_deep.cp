// Source: cp/cap/test/test_bm25_production_deep.cp (Deep Production Tests for cap:bm25)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TokenizerConfig,
    defaultStopwords,
    tokenizeTerms,
    freeTerms,
    Bm25Config,
    Bm25Index,
    RankedDoc,
    rrfFuse,
} from "cap:bm25";

test("tokenizeTerms case folding, stopwords, max_len, and UTF-8 preservation") {
    const alloc = std.testing.allocator;
    const sw = defaultStopwords();
    try std.testing.expect(sw.len >= 20);

    const cfg = TokenizerConfig{
        .lowercase = true,
        .filter_stopwords = true,
        .min_len = 2,
        .max_len = 16,
    };
    const text = "The Rust & Zig systems! 搜索引擎 supercalifragilisticexpialidocious x";
    const terms = try tokenizeTerms(alloc, text, cfg);
    defer freeTerms(alloc, terms);

    expect_eq(terms.len, 4);
    try std.testing.expect(std.mem.eql(u8, terms[0], "rust"));
    try std.testing.expect(std.mem.eql(u8, terms[1], "zig"));
    try std.testing.expect(std.mem.eql(u8, terms[2], "systems"));
    try std.testing.expect(std.mem.eql(u8, terms[3], "搜索引擎"));
}

test("Bm25Index Okapi BM25 ranking with IDF rarity and document length normalization") {
    const alloc = std.testing.allocator;
    let idx = try Bm25Index.init(alloc, Bm25Config{});
    defer idx.deinit();

    // Doc 1: short doc containing rare term "quantum" and common term "engine"
    try idx.addDocument(1, "quantum engine");
    // Doc 2: long doc containing "quantum" once and many other words
    try idx.addDocument(2, "quantum engine alpha beta gamma delta epsilon zeta theta iota kappa");
    // Doc 3: common term "engine" only
    try idx.addDocument(3, "engine runtime compiler");
    // Doc 4: common term "engine" only
    try idx.addDocument(4, "engine scheduler actor");

    expect_eq(idx.docCount(), 4);
    try std.testing.expect(idx.avgDocLen() > 3.0);

    const hits = try idx.search(alloc, "quantum engine", 3);
    defer alloc.free(hits);

    expect_eq(hits.len, 3);
    // Doc 1 should outrank Doc 2 due to shorter length, and both outrank Doc 3/4 due to rare "quantum" IDF
    expect_eq(hits[0].doc_id, 1);
    expect_eq(hits[1].doc_id, 2);
    try std.testing.expect(hits[0].score > hits[1].score);
    try std.testing.expect(hits[1].score > hits[2].score);
}

test("Bm25Index removeDocument and re-indexing update") {
    const alloc = std.testing.allocator;
    let idx = try Bm25Index.init(alloc, Bm25Config{});
    defer idx.deinit();

    try idx.addDocument(10, "apollo mission lunar module");
    try idx.addDocument(20, "voyager probe interstellar space");
    expect_eq(idx.docCount(), 2);

    // Re-index doc 10 with completely different content
    try idx.addDocument(10, "artemis rocket launch");
    expect_eq(idx.docCount(), 2);

    const old_hits = try idx.search(alloc, "lunar", 5);
    defer alloc.free(old_hits);
    expect_eq(old_hits.len, 0);

    const new_hits = try idx.search(alloc, "artemis", 5);
    defer alloc.free(new_hits);
    expect_eq(new_hits.len, 1);
    expect_eq(new_hits[0].doc_id, 10);

    // Remove doc 20 and verify no hits remain
    const removed = idx.removeDocument(20);
    try std.testing.expect(removed);
    try std.testing.expect(!idx.removeDocument(999));
    expect_eq(idx.docCount(), 1);

    const gone_hits = try idx.search(alloc, "voyager", 5);
    defer alloc.free(gone_hits);
    expect_eq(gone_hits.len, 0);
}

test("Bm25Index searchPhrase strict adjacent position matching") {
    const alloc = std.testing.allocator;
    let idx = try Bm25Index.init(alloc, Bm25Config{});
    defer idx.deinit();

    // Doc 1 has "zero copy" adjacent
    try idx.addDocument(1, "high performance zero copy network buffer");
    // Doc 2 has "zero" and "copy" separated by another token
    try idx.addDocument(2, "zero overhead memory copy routine");
    // Doc 3 has reversed order "copy zero"
    try idx.addDocument(3, "copy zero bytes");

    const phrase: [2][]const u8 = ["Zero", "Copy"];
    const hits = try idx.searchPhrase(alloc, &phrase, 5);
    defer alloc.free(hits);

    expect_eq(hits.len, 1);
    expect_eq(hits[0].doc_id, 1);
}

test("rrfFuse hybrid ranking, cap.bm25 mount, and zero memory leaks") {
    const alloc = std.testing.allocator;
    let idx = try cap.bm25.Bm25Index.init(alloc, cap.bm25.Bm25Config{});
    defer idx.deinit();
    try idx.addDocument(1, "hybrid search fusion");
    expect_eq(idx.docCount(), 1);

    const bm25_list: [3]RankedDoc = [
        RankedDoc{ .doc_id = 101, .rank = 1 },
        RankedDoc{ .doc_id = 102, .rank = 2 },
        RankedDoc{ .doc_id = 103, .rank = 3 },
    ];
    const vec_list: [3]RankedDoc = [
        RankedDoc{ .doc_id = 102, .rank = 1 },
        RankedDoc{ .doc_id = 104, .rank = 2 },
        RankedDoc{ .doc_id = 101, .rank = 3 },
    ];
    const lists: [2][]const RankedDoc = [&bm25_list, &vec_list];
    const weights: [2]f64 = [1.0, 1.0];

    const fused = try rrfFuse(alloc, &lists, &weights, 60.0, 3);
    defer alloc.free(fused);

    expect_eq(fused.len, 3);
    // Doc 102: 1/62 + 1/61; Doc 101: 1/61 + 1/63 -> 102 > 101 > 104
    expect_eq(fused[0].doc_id, 102);
    expect_eq(fused[1].doc_id, 101);
    expect_eq(fused[2].doc_id, 104);
    try std.testing.expect(fused[0].score > fused[1].score);
    try std.testing.expect(fused[1].score > fused[2].score);
}
