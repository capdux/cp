// Source: cp/cap/test/test_spell_production_deep.cp
// Deep Production Verification Suite for cap:spell & cap.spell
// Covers:
//   1. damerauLevenshtein OSA distance (transposition, insertion, deletion, substitution) & Soundex phonetic codes
//   2. SymSpellIndex dictionary loading, frequency-first ranking & Top/Closest/All verbosity modes
//   3. BkTree triangle-inequality pruned search vs SymSpellIndex consistency across dist = 0, 1, 2
//   4. Edge error interception (InvalidMaxDistance, EmptyWord) & std.testing.allocator zero-leak
//   5. `import cap from "cap"; cap.spell` dynamic mounting verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SpellError,
    SuggestVerbosity,
    SpellSuggestion,
    damerauLevenshtein,
    soundex,
    SymSpellIndex,
    BkTree,
} from "cap:spell";

test("cap:spell - damerauLevenshtein OSA transposition & soundex phonetic encoding") {
    const alloc = std.testing.allocator;

    // Adjacent transposition must be distance 1 (not 2).
    expect_eq(damerauLevenshtein(alloc, "teh", "the") catch unreachable, 1);
    expect_eq(damerauLevenshtein(alloc, "form", "from") catch unreachable, 1);
    expect_eq(damerauLevenshtein(alloc, "ca", "ac") catch unreachable, 1);

    // Classic edit operations: exact, insertion, deletion, substitution, empty.
    expect_eq(damerauLevenshtein(alloc, "spell", "spell") catch unreachable, 0);
    expect_eq(damerauLevenshtein(alloc, "", "abc") catch unreachable, 3);
    expect_eq(damerauLevenshtein(alloc, "abc", "") catch unreachable, 3);
    expect_eq(damerauLevenshtein(alloc, "cat", "cats") catch unreachable, 1);
    expect_eq(damerauLevenshtein(alloc, "cats", "cat") catch unreachable, 1);
    expect_eq(damerauLevenshtein(alloc, "cat", "cut") catch unreachable, 1);
    expect_eq(damerauLevenshtein(alloc, "kitten", "sitting") catch unreachable, 3);

    // Classic American Soundex phonetic codes.
    expect_eq(std.mem.eql(u8, soundex("Robert")[0..], "R163"), true);
    expect_eq(std.mem.eql(u8, soundex("Rupert")[0..], "R163"), true);
    expect_eq(std.mem.eql(u8, soundex("Rubin")[0..], "R150"), true);
    expect_eq(std.mem.eql(u8, soundex("Ashcraft")[0..], "A261"), true);
    expect_eq(std.mem.eql(u8, soundex("Ashcroft")[0..], "A261"), true);
    expect_eq(std.mem.eql(u8, soundex("Tymczak")[0..], "T522"), true);
    expect_eq(std.mem.eql(u8, soundex("Pfister")[0..], "P236"), true);
    expect_eq(std.mem.eql(u8, soundex("")[0..], "0000"), true);
    expect_eq(std.mem.eql(u8, soundex("1234")[0..], "0000"), true);
}

test("cap:spell - SymSpellIndex dictionary loading, frequency ranking & verbosity modes") {
    const alloc = std.testing.allocator;
    let idx = SymSpellIndex.init(alloc, 2, 7) catch unreachable;
    defer idx.deinit();

    // Load a diverse corpus including words longer than prefix_length (7).
    idx.addWord("The", 10000) catch unreachable;
    idx.addWord("there", 3000) catch unreachable;
    idx.addWord("there", 2000) catch unreachable; // accumulates to 5000
    idx.addWord("their", 2500) catch unreachable;
    idx.addWord("theme", 800) catch unreachable;
    idx.addWord("these", 1200) catch unreachable;
    idx.addWord("three", 900) catch unreachable;
    idx.addWord("algorithm", 4000) catch unreachable;
    idx.addWord("logarithm", 1500) catch unreachable;
    idx.addWord("international", 6000) catch unreachable;
    idx.addWord("interpretation", 2200) catch unreachable;
    idx.addWord("configuration", 3500) catch unreachable;
    idx.addWord("application", 4500) catch unreachable;

    expect_eq(idx.wordCount(), 12);

    // Transposition "teh" -> "the" (dist=1)
    const top_teh = idx.lookup(alloc, "TEH", .Top, 2) catch unreachable;
    defer alloc.free(top_teh);
    expect_eq(top_teh.len, 1);
    expect_eq(std.mem.eql(u8, top_teh[0].term, "the"), true);
    expect_eq(top_teh[0].distance, 1);
    expect_eq(top_teh[0].frequency, 10000);

    // Long word typo beyond prefix_length=7: "internationla" -> "international" (dist=1)
    const long_hit = idx.lookup(alloc, "internationla", .Top, 2) catch unreachable;
    defer alloc.free(long_hit);
    expect_eq(long_hit.len, 1);
    expect_eq(std.mem.eql(u8, long_hit[0].term, "international"), true);
    expect_eq(long_hit[0].distance, 1);

    // Query "thier": "their" (dist=1, freq=2500) beats dist=2 candidates.
    // In .Closest mode, only dist=1 matches are returned; in .All mode, dist=2 matches also appear.
    const closest_thier = idx.lookup(alloc, "thier", .Closest, 2) catch unreachable;
    defer alloc.free(closest_thier);
    expect_eq(closest_thier.len, 1);
    expect_eq(std.mem.eql(u8, closest_thier[0].term, "their"), true);
    expect_eq(closest_thier[0].distance, 1);

    const all_thier = idx.lookup(alloc, "thier", .All, 2) catch unreachable;
    defer alloc.free(all_thier);
    expect_eq(all_thier.len > 1, true);
    expect_eq(std.mem.eql(u8, all_thier[0].term, "their"), true);

    // Same-distance tie-breaking by frequency DESC: query "ther" -> "there" (5000, dist=1) before "the" (10000, dist=1)?
    // Wait: "the" has dist=1 (delete 'r'), freq=10000; "there" has dist=1 (insert 'e'), freq=5000.
    // So "the" (freq=10000) must rank before "there" (freq=5000)!
    const closest_ther = idx.lookup(alloc, "ther", .Closest, 2) catch unreachable;
    defer alloc.free(closest_ther);
    expect_eq(closest_ther.len >= 2, true);
    expect_eq(closest_ther[0].distance, 1);
    expect_eq(closest_ther[1].distance, 1);
    expect_eq(std.mem.eql(u8, closest_ther[0].term, "the"), true);
    expect_eq(std.mem.eql(u8, closest_ther[1].term, "there"), true);
}

test("cap:spell - BkTree triangle inequality search matches SymSpellIndex across dist 0..2") {
    const alloc = std.testing.allocator;

    let sym = SymSpellIndex.init(alloc, 2, 7) catch unreachable;
    defer sym.deinit();
    let bk = BkTree.init(alloc);
    defer bk.deinit();

    const entries = [
        "apple", "apply", "maple", "ample", "banana", "bandana",
        "orange", "grape", "search", "server", "service", "spell",
        "spelling", "the", "there", "their", "them", "then",
    ];

    let f: u64 = 100;
    for (w of entries) {
        sym.addWord(w, f) catch unreachable;
        bk.insert(w, f) catch unreachable;
        f += 50;
    }

    expect_eq(bk.wordCount(), entries.len);
    expect_eq(sym.wordCount(), entries.len);

    // Verify exact parity for dist = 0, 1, 2 (including transposition "teh").
    const queries = ["apple", "aple", "teh", "serch", "banan"];
    const dists: [3]u32 = [0, 1, 2];

    for (q of queries) {
        for (d of dists) {
            const s_res = sym.lookup(alloc, q, .All, d) catch unreachable;
            defer alloc.free(s_res);
            const b_res = bk.search(alloc, q, d) catch unreachable;
            defer alloc.free(b_res);

            expect_eq(b_res.len, s_res.len);
            let i: usize = 0;
            while (i < s_res.len) : (i += 1) {
                expect_eq(std.mem.eql(u8, b_res[i].term, s_res[i].term), true);
                expect_eq(b_res[i].distance, s_res[i].distance);
                expect_eq(b_res[i].frequency, s_res[i].frequency);
            }
        }
    }
}

test("cap:spell - edge error interception & std.testing.allocator zero-leak") {
    const alloc = std.testing.allocator;

    // InvalidMaxDistance on SymSpellIndex.init (> 8)
    let got_init_invalid = false;
    * = SymSpellIndex.init(alloc, 9, 7) catch e => {
        if (e == SpellError.InvalidMaxDistance) got_init_invalid = true;
    };
    expect_eq(got_init_invalid, true);

    let sym = SymSpellIndex.init(alloc, 2, 7) catch unreachable;
    defer sym.deinit();
    let bk = BkTree.init(alloc);
    defer bk.deinit();

    // Empty search on empty BkTree returns empty slice cleanly.
    const empty_bk = bk.search(alloc, "hello", 2) catch unreachable;
    defer alloc.free(empty_bk);
    expect_eq(empty_bk.len, 0);

    // EmptyWord errors
    let got_empty_add = false;
    sym.addWord("", 10) catch e => {
        if (e == SpellError.EmptyWord) got_empty_add = true;
    };
    expect_eq(got_empty_add, true);

    let got_empty_lookup = false;
    * = sym.lookup(alloc, "", .Top, 1) catch e => {
        if (e == SpellError.EmptyWord) got_empty_lookup = true;
    };
    expect_eq(got_empty_lookup, true);

    let got_empty_bk_ins = false;
    bk.insert("", 10) catch e => {
        if (e == SpellError.EmptyWord) got_empty_bk_ins = true;
    };
    expect_eq(got_empty_bk_ins, true);

    let got_empty_bk_search = false;
    * = bk.search(alloc, "", 1) catch e => {
        if (e == SpellError.EmptyWord) got_empty_bk_search = true;
    };
    expect_eq(got_empty_bk_search, true);

    // InvalidMaxDistance on lookup (> configured max_edit_distance)
    let got_lookup_invalid = false;
    * = sym.lookup(alloc, "hello", .All, 3) catch e => {
        if (e == SpellError.InvalidMaxDistance) got_lookup_invalid = true;
    };
    expect_eq(got_lookup_invalid, true);
}

test("cap.spell - dynamic mounting via import cap from \"cap\"") {
    const alloc = std.testing.allocator;

    const d = cap.spell.damerauLevenshtein(alloc, "algoritm", "algorithm") catch unreachable;
    expect_eq(d, 1);

    const sx = cap.spell.soundex("Rupert");
    expect_eq(std.mem.eql(u8, sx[0..], "R163"), true);

    let idx = cap.spell.SymSpellIndex.init(alloc, 2, 7) catch unreachable;
    defer idx.deinit();
    idx.addWord("compile", 500) catch unreachable;

    const res = idx.lookup(alloc, "compiel", cap.spell.SuggestVerbosity.Top, 2) catch unreachable;
    defer alloc.free(res);
    expect_eq(res.len, 1);
    expect_eq(std.mem.eql(u8, res[0].term, "compile"), true);
    expect_eq(res[0].distance, 1);

    let tree = cap.spell.BkTree.init(alloc);
    defer tree.deinit();
    tree.insert("compile", 500) catch unreachable;
    const bk_res = tree.search(alloc, "compiel", 1) catch unreachable;
    defer alloc.free(bk_res);
    expect_eq(bk_res.len, 1);
    expect_eq(std.mem.eql(u8, bk_res[0].term, "compile"), true);
}
