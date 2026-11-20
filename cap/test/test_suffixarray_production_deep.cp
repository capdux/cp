// Source: cp/cap/test/test_suffixarray_production_deep.cp
// Deep Production Verification Suite for cap:suffixarray & cap.suffixarray
// Covers:
//   1. Classic "banana" SA [5,3,1,0,4,2] and Kasai LCP [0,1,3,0,0,2] exact verification
//   2. findAll / count / contains (including overlapping "ana" in "banana" at [1, 3]) & ascending offsets
//   3. longestRepeatedSubstring & distinctSubstringCount formula vs brute-force set cross-check
//   4. Empty text, single byte, all-identical "aaaaa", binary \x00 payload, and EmptyPattern error interception
//   5. `import cap from "cap"; cap.suffixarray` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SuffixArrayError,
    SuffixArray,
} from "cap:suffixarray";

fn expectSliceEqUsize(actual: []const usize, expected: []const usize) {
    expect_eq(actual.len, expected.len);
    let i: usize = 0;
    while (i < actual.len) : (i += 1) {
        expect_eq(actual[i], expected[i]);
    }
}

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(actual.len, expected.len);
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn bruteForceDistinctSubstrings(alloc: std.mem.Allocator, text: []const u8): u64 {
    let seen: std.StringHashMap(void) = .init(alloc);
    defer seen.deinit();

    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        let j: usize = i + 1;
        while (j <= text.len) : (j += 1) {
            seen.put(text[i..j], {}) catch unreachable;
        }
    }
    return seen.count() as u64;
}

test("cap:suffixarray - classic banana SA and Kasai LCP exact verification") {
    const alloc = std.testing.allocator;
    let sa = SuffixArray.build(alloc, "banana") catch unreachable;
    defer sa.deinit();

    expect_eq(sa.len(), 6);

    const expected_sa: [6]usize = [ 5, 3, 1, 0, 4, 2 ];
    const expected_lcp: [6]usize = [ 0, 1, 3, 0, 0, 2 ];

    expectSliceEqUsize(sa.saSlice(), &expected_sa);
    expectSliceEqUsize(sa.lcpSlice(), &expected_lcp);
}

test("cap:suffixarray - findAll, count, contains with overlapping occurrences and ascending offsets") {
    const alloc = std.testing.allocator;
    let sa = SuffixArray.build(alloc, "banana") catch unreachable;
    defer sa.deinit();

    // Overlapping pattern "ana" at [1, 3]
    expect_eq(sa.contains("ana") catch unreachable, true);
    expect_eq(sa.count("ana") catch unreachable, 2);

    const ana_hits = sa.findAll(alloc, "ana") catch unreachable;
    defer alloc.free(ana_hits);
    const expected_ana: [2]usize = [ 1, 3 ];
    expectSliceEqUsize(ana_hits, &expected_ana);

    // Single char "a" at [1, 3, 5]
    expect_eq(sa.count("a") catch unreachable, 3);
    const a_hits = sa.findAll(alloc, "a") catch unreachable;
    defer alloc.free(a_hits);
    const expected_a: [3]usize = [ 1, 3, 5 ];
    expectSliceEqUsize(a_hits, &expected_a);

    // Missing patterns
    expect_eq(sa.contains("apple") catch unreachable, false);
    expect_eq(sa.contains("bananas") catch unreachable, false);
    expect_eq(sa.count("xyz") catch unreachable, 0);
    const none_hits = sa.findAll(alloc, "xyz") catch unreachable;
    defer alloc.free(none_hits);
    expect_eq(none_hits.len, 0);
}

test("cap:suffixarray - longestRepeatedSubstring and distinctSubstringCount vs brute-force") {
    const alloc = std.testing.allocator;

    // Case 1: "banana" -> LRS = "ana", distinct = 21 - (0+1+3+0+0+2) = 15
    let sa1 = SuffixArray.build(alloc, "banana") catch unreachable;
    defer sa1.deinit();
    expectBytesEq(sa1.longestRepeatedSubstring(), "ana");
    expect_eq(sa1.distinctSubstringCount(), 15);
    expect_eq(sa1.distinctSubstringCount(), bruteForceDistinctSubstrings(alloc, "banana"));

    // Case 2: "mississippi" -> LRS = "issi"
    const s2 = "mississippi";
    let sa2 = SuffixArray.build(alloc, s2) catch unreachable;
    defer sa2.deinit();
    expectBytesEq(sa2.longestRepeatedSubstring(), "issi");
    expect_eq(sa2.distinctSubstringCount(), bruteForceDistinctSubstrings(alloc, s2));

    // Case 3: all unique "abcdef" -> LRS = "", distinct = 6*7/2 = 21
    const s3 = "abcdef";
    let sa3 = SuffixArray.build(alloc, s3) catch unreachable;
    defer sa3.deinit();
    expectBytesEq(sa3.longestRepeatedSubstring(), "");
    expect_eq(sa3.distinctSubstringCount(), 21);
    expect_eq(sa3.distinctSubstringCount(), bruteForceDistinctSubstrings(alloc, s3));
}

test("cap:suffixarray - empty text, single byte, all-identical, binary NUL payload, and EmptyPattern") {
    const alloc = std.testing.allocator;

    // Empty text
    let sa_empty = SuffixArray.build(alloc, "") catch unreachable;
    defer sa_empty.deinit();
    expect_eq(sa_empty.len(), 0);
    expect_eq(sa_empty.saSlice().len, 0);
    expect_eq(sa_empty.lcpSlice().len, 0);
    expect_eq(sa_empty.contains("a") catch unreachable, false);
    expect_eq(sa_empty.count("a") catch unreachable, 0);
    const empty_hits = sa_empty.findAll(alloc, "a") catch unreachable;
    defer alloc.free(empty_hits);
    expect_eq(empty_hits.len, 0);
    expectBytesEq(sa_empty.longestRepeatedSubstring(), "");
    expect_eq(sa_empty.distinctSubstringCount(), 0);

    // Single byte
    let sa_one = SuffixArray.build(alloc, "z") catch unreachable;
    defer sa_one.deinit();
    expect_eq(sa_one.len(), 1);
    expect_eq(sa_one.contains("z") catch unreachable, true);
    expect_eq(sa_one.contains("a") catch unreachable, false);
    expectBytesEq(sa_one.longestRepeatedSubstring(), "");
    expect_eq(sa_one.distinctSubstringCount(), 1);

    // All-identical "aaaaa" -> SA=[4,3,2,1,0], LCP=[0,1,2,3,4], LRS="aaaa", distinct=5
    let sa_rep = SuffixArray.build(alloc, "aaaaa") catch unreachable;
    defer sa_rep.deinit();
    const expected_rep_sa: [5]usize = [ 4, 3, 2, 1, 0 ];
    const expected_rep_lcp: [5]usize = [ 0, 1, 2, 3, 4 ];
    expectSliceEqUsize(sa_rep.saSlice(), &expected_rep_sa);
    expectSliceEqUsize(sa_rep.lcpSlice(), &expected_rep_lcp);
    expectBytesEq(sa_rep.longestRepeatedSubstring(), "aaaa");
    expect_eq(sa_rep.distinctSubstringCount(), 5);

    // Binary NUL byte payload
    const bin_text = "\x00ab\x00ab\x00";
    let sa_bin = SuffixArray.build(alloc, bin_text) catch unreachable;
    defer sa_bin.deinit();
    expect_eq(sa_bin.count("\x00ab") catch unreachable, 2);
    const bin_hits = sa_bin.findAll(alloc, "\x00ab") catch unreachable;
    defer alloc.free(bin_hits);
    const expected_bin: [2]usize = [ 0, 3 ];
    expectSliceEqUsize(bin_hits, &expected_bin);
    expectBytesEq(sa_bin.longestRepeatedSubstring(), "\x00ab\x00");

    // EmptyPattern error interception
    let err_contains = false;
    * = sa_bin.contains("") catch |e| {
        if (e == SuffixArrayError.EmptyPattern) {
            err_contains = true;
        }
    };
    expect_eq(err_contains, true);

    let err_count = false;
    * = sa_bin.count("") catch |e| {
        if (e == SuffixArrayError.EmptyPattern) {
            err_count = true;
        }
    };
    expect_eq(err_count, true);

    let err_find = false;
    * = sa_bin.findAll(alloc, "") catch |e| {
        if (e == SuffixArrayError.EmptyPattern) {
            err_find = true;
        }
    };
    expect_eq(err_find, true);
}

test("cap.suffixarray - dynamic namespace mount and zero-leak check") {
    const alloc = std.testing.allocator;
    let sa = cap.suffixarray.SuffixArray.build(alloc, "abracadabra") catch unreachable;
    defer sa.deinit();

    expect_eq(sa.contains("abra") catch unreachable, true);
    expect_eq(sa.count("abra") catch unreachable, 2);
    expectBytesEq(sa.longestRepeatedSubstring(), "abra");

    const hits = sa.findAll(alloc, "abra") catch unreachable;
    defer alloc.free(hits);
    const expected: [2]usize = [ 0, 7 ];
    expectSliceEqUsize(hits, &expected);
    expect_eq(sa.distinctSubstringCount(), bruteForceDistinctSubstrings(alloc, "abracadabra"));
}
