// Source: cp/cap/test/test_fuzzy_production_deep.cp
// Deep Production Verification Suite for cap:fuzzy & cap.fuzzy
// Covers:
//   1. levenshtein & damerauLevenshtein (empty, equal, "kitten"->"sitting"=3, "ca"->"ac"=1, "ab"->"ba"=1)
//   2. jaro & jaroWinkler standard vectors ("MARTHA"/"MARHTA", "DWAYNE"/"DUANE") & InvalidPrefixScale
//   3. fuzzyScore word-boundary/camelCase/consecutive bonuses vs scattered match & exact positions
//   4. rankCandidates batch path/command ranking & empty query boundary
//   5. `import cap from "cap"; cap.fuzzy` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FuzzyError,
    FuzzyMatch,
    levenshtein,
    damerauLevenshtein,
    jaro,
    jaroWinkler,
    fuzzyScore,
    rankCandidates,
    freeMatches,
} from "cap:ds";

fn approxEq(a: f64, b: f64, tol: f64): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= tol;
}

test("cap:fuzzy - levenshtein and damerauLevenshtein edit distances") {
    const alloc = std.testing.allocator;

    // Empty & equal strings
    expect_eq(levenshtein(alloc, "", "") catch unreachable, 0);
    expect_eq(levenshtein(alloc, "abc", "") catch unreachable, 3);
    expect_eq(levenshtein(alloc, "", "hello") catch unreachable, 5);
    expect_eq(levenshtein(alloc, "rust", "rust") catch unreachable, 0);

    // Classic "kitten" -> "sitting" = 3
    expect_eq(levenshtein(alloc, "kitten", "sitting") catch unreachable, 3);
    expect_eq(damerauLevenshtein(alloc, "kitten", "sitting") catch unreachable, 3);

    // Adjacent transpositions: Levenshtein = 2 vs Damerau-Levenshtein = 1
    expect_eq(levenshtein(alloc, "ca", "ac") catch unreachable, 2);
    expect_eq(damerauLevenshtein(alloc, "ca", "ac") catch unreachable, 1);
    expect_eq(damerauLevenshtein(alloc, "ab", "ba") catch unreachable, 1);
    expect_eq(damerauLevenshtein(alloc, "a_cat", "an_act") catch unreachable, 2);

    // Empty & equal for Damerau-Levenshtein
    expect_eq(damerauLevenshtein(alloc, "", "") catch unreachable, 0);
    expect_eq(damerauLevenshtein(alloc, "xyz", "") catch unreachable, 3);
    expect_eq(damerauLevenshtein(alloc, "same", "same") catch unreachable, 0);
}

test("cap:fuzzy - jaro and jaroWinkler standard vectors and prefix_scale validation") {
    const alloc = std.testing.allocator;

    // Boundary cases
    expect_eq(approxEq(jaro(alloc, "", "") catch unreachable, 1.0, 1e-9), true);
    expect_eq(approxEq(jaro(alloc, "abc", "") catch unreachable, 0.0, 1e-9), true);
    expect_eq(approxEq(jaro(alloc, "exact", "exact") catch unreachable, 1.0, 1e-9), true);

    // "MARTHA" vs "MARHTA": Jaro = 17/18 (~0.944444), JW(0.1) = 0.961111
    const j_martha = jaro(alloc, "MARTHA", "MARHTA") catch unreachable;
    expect_eq(approxEq(j_martha, 17.0 / 18.0, 1e-6), true);

    const jw_martha = jaroWinkler(alloc, "MARTHA", "MARHTA", 0.1) catch unreachable;
    expect_eq(approxEq(jw_martha, 0.9611111111, 1e-6), true);

    // "DWAYNE" vs "DUANE": Jaro = 0.822222..., JW(0.1) = 0.840000
    const j_dwayne = jaro(alloc, "DWAYNE", "DUANE") catch unreachable;
    expect_eq(approxEq(j_dwayne, 0.8222222222, 1e-6), true);

    const jw_dwayne = jaroWinkler(alloc, "DWAYNE", "DUANE", 0.1) catch unreachable;
    expect_eq(approxEq(jw_dwayne, 0.84, 1e-6), true);

    // Invalid prefix_scale > 0.25 or < 0.0 must return FuzzyError.InvalidPrefixScale
    let got_high_err = false;
    * = jaroWinkler(alloc, "abc", "abd", 0.26) catch |e| {
        if (e == FuzzyError.InvalidPrefixScale) {
            got_high_err = true;
        }
    };
    expect_eq(got_high_err, true);

    let got_neg_err = false;
    * = jaroWinkler(alloc, "abc", "abd", -0.01) catch |e| {
        if (e == FuzzyError.InvalidPrefixScale) {
            got_neg_err = true;
        }
    };
    expect_eq(got_neg_err, true);
}

test("cap:fuzzy - fuzzyScore boundary, camelCase, consecutive bonuses and exact positions") {
    const alloc = std.testing.allocator;

    // Word-boundary match ("foo_bar" -> 0, 4) beats non-boundary ("foobar" -> 0, 3) & scattered ("a_x_f_y_b")
    let m_boundary = (fuzzyScore(alloc, "fb", "foo_bar", false) catch unreachable) orelse unreachable;
    defer m_boundary.deinit(alloc);

    let m_plain = (fuzzyScore(alloc, "fb", "foobar", false) catch unreachable) orelse unreachable;
    defer m_plain.deinit(alloc);

    let m_scattered = (fuzzyScore(alloc, "fb", "a_x_f_y_b", false) catch unreachable) orelse unreachable;
    defer m_scattered.deinit(alloc);

    expect_eq(m_boundary.score > m_plain.score, true);
    expect_eq(m_plain.score > m_scattered.score, true);
    expect_eq(m_boundary.positions.len, 2);
    expect_eq(m_boundary.positions[0], 0);
    expect_eq(m_boundary.positions[1], 4);

    // CamelCase boundary ("getUserName" with "gun" -> positions 0, 3, 7)
    let m_camel = (fuzzyScore(alloc, "gun", "getUserName", false) catch unreachable) orelse unreachable;
    defer m_camel.deinit(alloc);
    expect_eq(m_camel.positions.len, 3);
    expect_eq(m_camel.positions[0], 0);
    expect_eq(m_camel.positions[1], 3);
    expect_eq(m_camel.positions[2], 7);

    // Consecutive path segment match ("lib/fuzzy/index.cp" with "fuzzy" -> 4,5,6,7,8)
    let m_path = (fuzzyScore(alloc, "fuzzy", "lib/fuzzy/index.cp", false) catch unreachable) orelse unreachable;
    defer m_path.deinit(alloc);
    expect_eq(m_path.positions.len, 5);
    expect_eq(m_path.positions[0], 4);
    expect_eq(m_path.positions[1], 5);
    expect_eq(m_path.positions[2], 6);
    expect_eq(m_path.positions[3], 7);
    expect_eq(m_path.positions[4], 8);

    // Case-sensitive miss vs case-insensitive hit
    const miss_cs = fuzzyScore(alloc, "fb", "FooBar", true) catch unreachable;
    expect_eq(miss_cs == null, true);

    let hit_ci = (fuzzyScore(alloc, "fb", "FooBar", false) catch unreachable) orelse unreachable;
    defer hit_ci.deinit(alloc);
    expect_eq(hit_ci.positions[0], 0);
    expect_eq(hit_ci.positions[1], 3);
}

test("cap:fuzzy - rankCandidates batch ranking and empty query boundary") {
    const alloc = std.testing.allocator;

    const candidates: [5][]const u8 = [
        "src/utils/x_f_z.cp",
        "lib/fuzzy/fuzzy.cp",
        "unrelated/file.txt",
        "lib/fz/main.cp",
        "fuzzy_search.cp",
    ];

    const ranked = rankCandidates(alloc, "fz", &candidates, false) catch unreachable;
    defer freeMatches(alloc, ranked);

    // "unrelated/file.txt" does not contain 'z' after 'f' -> filtered out (4 matches remain)
    expect_eq(ranked.len, 4);

    // Scores must be strictly non-increasing; tie-broken by original index ascending
    let i: usize = 1;
    while (i < ranked.len) : (i += 1) {
        expect_eq(ranked[i - 1].score >= ranked[i].score, true);
        if (ranked[i - 1].score == ranked[i].score) {
            expect_eq(ranked[i - 1].index < ranked[i].index, true);
        }
    }

    // "fuzzy_search.cp" (first char + consecutive/boundary) or "lib/fz/main.cp" must rank above scattered "src/utils/x_f_z.cp"
    expect_eq(ranked[ranked.len - 1].index, 0);

    // Empty query matches all candidates with score=0, positions.len=0, preserving original index order
    const empty_ranked = rankCandidates(alloc, "", &candidates, false) catch unreachable;
    defer freeMatches(alloc, empty_ranked);
    expect_eq(empty_ranked.len, 5);
    expect_eq(empty_ranked[0].index, 0);
    expect_eq(empty_ranked[0].score, 0);
    expect_eq(empty_ranked[0].positions.len, 0);
    expect_eq(empty_ranked[4].index, 4);
}

test("cap.fuzzy - dynamic namespace mounting and zero-leak verification") {
    const alloc = std.testing.allocator;

    expect_eq(cap.ds.levenshtein(alloc, "abc", "axc") catch unreachable, 1);
    expect_eq(cap.ds.damerauLevenshtein(alloc, "ab", "ba") catch unreachable, 1);

    const sim = cap.ds.jaroWinkler(alloc, "MARTHA", "MARHTA", 0.1) catch unreachable;
    expect_eq(approxEq(sim, 0.9611111111, 1e-6), true);

    const items: [2][]const u8 = [ "cap/test/fuzzy.cp", "cap/src/other.cp" ];
    const res = cap.ds.rankCandidates(alloc, "fuzzy", &items, false) catch unreachable;
    defer cap.ds.freeMatches(alloc, res);
    expect_eq(res.len, 1);
    expect_eq(res[0].index, 0);
    expect_eq(res[0].positions.len, 5);
}
