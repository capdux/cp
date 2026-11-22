// Source: cp/cap/test/test_search_production_deep.cp
// Deep Production Verification Suite for cap:search & cap.search
// Covers:
//   1. PrefixTrie insertion, weight aggregation, contains, termCount & Top-K suggest ordering
//   2. SearchIndex multi-field BM25F title/body weighting, DuplicateDocId, upsert & removeDocument
//   3. Boolean queries (AND / +must / -exclude / NOT) & exact phrase matching ("...") + InvalidQuery
//   4. highlightSnippet dense-window extraction & <mark>...</mark> multi-keyword highlighting
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.search`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SearchError,
    PrefixSuggestion,
    PrefixTrie,
    SearchHit,
    SearchIndex,
} from "cap:db";

fn containsSubstr(haystack: string, needle: string): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

fn testPrefixTrieInsertAndSuggest()! {
    const alloc = std.heap.page_allocator;

    let trie = PrefixTrie.init(alloc);
    defer trie.deinit();

    trie.insert("Search", 2)?;
    trie.insert("search", 3)?;   // total freq = 5
    trie.insert("Server", 4)?;   // freq = 4
    trie.insert("Session", 4)?;  // tie with "server", lexicographically after "server"
    trie.insert("Serial", 1)?;   // freq = 1
    trie.insert("Alpha", 10)?;

    expect_eq(trie.termCount(), 5);
    expect_eq(trie.contains("SEARCH"), true);
    expect_eq(trie.contains("server"), true);
    expect_eq(trie.contains("missing"), false);

    // Top-3 for prefix "Se" -> search (5), server (4), session (4)
    const sugs = trie.suggest(alloc, "Se", 3)?;
    defer alloc.free(sugs);
    expect_eq(sugs.len, 3);
    expect_eq(std.mem.eql(u8, sugs[0].term, "search"), true);
    expect_eq(sugs[0].freq, 5);
    expect_eq(std.mem.eql(u8, sugs[1].term, "server"), true);
    expect_eq(sugs[1].freq, 4);
    expect_eq(std.mem.eql(u8, sugs[2].term, "session"), true);
    expect_eq(sugs[2].freq, 4);

    // Empty prefix / non-matching prefix
    const empty_sugs = trie.suggest(alloc, "xyz", 5)?;
    defer alloc.free(empty_sugs);
    expect_eq(empty_sugs.len, 0);
}

fn testSearchIndexFieldWeightingUpsertAndRemove()! {
    const alloc = std.heap.page_allocator;

    let idx = SearchIndex.init(alloc);
    defer idx.deinit();

    idx.setFieldWeights(4.0, 1.0);

    // doc_body has "quantum" in body; doc_title has "quantum" in title
    idx.addDocument("doc_body", "General Systems Overview", "This article briefly discusses quantum indexing.")?;
    idx.addDocument("doc_title", "Quantum Indexing Architecture", "A detailed guide to fast retrieval.")?;
    expect_eq(idx.docCount(), 2);

    // DuplicateDocId error check
    let got_dup = false;
    idx.addDocument("doc_title", "Dup", "Dup") catch e => {
        if (e == SearchError.DuplicateDocId) got_dup = true;
    };
    expect_eq(got_dup, true);

    const hits = idx.search(alloc, "quantum", 5)?;
    defer SearchIndex.freeHits(alloc, hits);
    expect_eq(hits.len, 2);
    expect_eq(std.mem.eql(u8, hits[0].doc_id, "doc_title"), true);
    expect_eq(std.mem.eql(u8, hits[1].doc_id, "doc_body"), true);
    expect_eq(hits[0].score > hits[1].score, true);

    // Autocomplete integrated with indexed documents
    const ac = idx.autocomplete(alloc, "quan", 5)?;
    defer alloc.free(ac);
    expect_eq(ac.len, 1);
    expect_eq(std.mem.eql(u8, ac[0].term, "quantum"), true);
    expect_eq(ac[0].freq, 2);

    // Upsert replaces content cleanly
    idx.upsertDocument("doc_body", "Photon Retrieval", "Updated body with photon engine.")?;
    expect_eq(idx.docCount(), 2);

    const hits_after_upsert = idx.search(alloc, "quantum", 5)?;
    defer SearchIndex.freeHits(alloc, hits_after_upsert);
    expect_eq(hits_after_upsert.len, 1);
    expect_eq(std.mem.eql(u8, hits_after_upsert[0].doc_id, "doc_title"), true);

    const photon_hits = idx.search(alloc, "photon", 5)?;
    defer SearchIndex.freeHits(alloc, photon_hits);
    expect_eq(photon_hits.len, 1);
    expect_eq(std.mem.eql(u8, photon_hits[0].doc_id, "doc_body"), true);

    // Remove document
    expect_eq(idx.removeDocument("doc_title"), true);
    expect_eq(idx.removeDocument("doc_title"), false);
    expect_eq(idx.docCount(), 1);

    const after_rm = idx.search(alloc, "quantum", 5)?;
    defer SearchIndex.freeHits(alloc, after_rm);
    expect_eq(after_rm.len, 0);
}

fn testBooleanAndExactPhraseQueries()! {
    const alloc = std.heap.page_allocator;

    let idx = SearchIndex.init(alloc);
    defer idx.deinit();

    idx.addDocument("d1", "Memory Safety", "Zero copy memory safety in CP actor runtime.")?;
    idx.addDocument("d2", "Copy Semantics", "Safety of memory allocation and copy elision.")?;
    idx.addDocument("d3", "Legacy Runtime", "Zero copy memory safety with legacy gc pauses.")?;
    idx.addDocument("d4", "Actor Guide", "Actor mailbox and supervision trees.")?;

    // 1. Exact phrase + exclusion: "zero copy" AND actor -legacy
    const q1 = idx.search(alloc, "\"zero copy\" AND actor -legacy", 10)?;
    defer SearchIndex.freeHits(alloc, q1);
    expect_eq(q1.len, 1);
    expect_eq(std.mem.eql(u8, q1[0].doc_id, "d1"), true);

    // 2. Exact phrase order check: "memory copy" does NOT match "copy memory" or separated words
    const q2 = idx.search(alloc, "\"memory copy\"", 10)?;
    defer SearchIndex.freeHits(alloc, q2);
    expect_eq(q2.len, 0);

    // 3. +must and NOT operator
    const q3 = idx.search(alloc, "+memory NOT legacy", 10)?;
    defer SearchIndex.freeHits(alloc, q3);
    expect_eq(q3.len, 2);

    // 4. Malformed query (unclosed quote) -> InvalidQuery
    let got_invalid = false;
    * = idx.search(alloc, "\"unclosed phrase", 5) catch e => {
        if (e == SearchError.InvalidQuery) got_invalid = true;
    };
    expect_eq(got_invalid, true);
}

fn testHighlightSnippetDenseWindow()! {
    const alloc = std.heap.page_allocator;

    let idx = SearchIndex.init(alloc);
    defer idx.deinit();

    const long_text = "Introduction to distributed systems and storage engines. " ++
        "Many architectures exist for data processing. " ++
        "In modern RAG pipelines, BM25F ranking and Trie autocomplete accelerate local search queries significantly. " ++
        "Concluding remarks about deployment and monitoring.";

    const terms: [2]string = ["bm25f", "autocomplete"];
    const snippet = idx.highlightSnippet(alloc, long_text, &terms, 95, "<mark>", "</mark>")?;
    defer alloc.free(snippet);

    expect_eq(containsSubstr(snippet, "<mark>BM25F</mark>"), true);
    expect_eq(containsSubstr(snippet, "<mark>autocomplete</mark>"), true);
    expect_eq(containsSubstr(snippet, "..."), true);
}

fn testDynamicCapNamespaceMounting()! {
    const alloc = std.heap.page_allocator;

    let idx = cap.db.SearchIndex.init(alloc);
    defer idx.deinit();

    idx.addDocument("cap1", "Cap Search Module", "Dynamic namespace mounting for cap.search works cleanly.")?;
    const hits = idx.search(alloc, "namespace", 3)?;
    defer cap.db.SearchIndex.freeHits(alloc, hits);

    expect_eq(hits.len, 1);
    expect_eq(std.mem.eql(u8, hits[0].doc_id, "cap1"), true);
    expect_eq(containsSubstr(hits[0].snippet, "<mark>namespace</mark>"), true);
}

test("cap:search PrefixTrie insert, weights and Top-K suggest") {
    testPrefixTrieInsertAndSuggest() catch unreachable;
}

test("cap:search SearchIndex BM25F field weighting, upsert and remove") {
    testSearchIndexFieldWeightingUpsertAndRemove() catch unreachable;
}

test("cap:search boolean operators and exact phrase matching") {
    testBooleanAndExactPhraseQueries() catch unreachable;
}

test("cap:search highlightSnippet dense window and mark tags") {
    testHighlightSnippetDenseWindow() catch unreachable;
}

test("cap:search dynamic cap.search namespace mounting") {
    testDynamicCapNamespaceMounting() catch unreachable;
}
