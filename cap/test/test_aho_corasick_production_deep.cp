// Source: cp/cap/test/test_aho_corasick_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { Match, AhoCorasick } from "cap:ds";

test "aho_corasick: classic ushers overlapping multi-pattern match" {
    const alloc = std.testing.allocator;
    const pats: [4][]const u8 = [ "he", "she", "his", "hers" ];
    let ac = try AhoCorasick.init(alloc, &pats);
    defer ac.deinit(alloc);

    const matches = try ac.findAllOverlapping(alloc, "ushers");
    defer alloc.free(matches);

    // Matches: "she" (1..4), "he" (2..4), "hers" (2..6)
    try expect_eq(matches.len, 3);
    try expect_eq(matches[0].pattern_id, 1);
    try expect_eq(matches[0].start, 1);
    try expect_eq(matches[0].end, 4);
    try expect_eq(matches[1].pattern_id, 0);
    try expect_eq(matches[2].pattern_id, 3);
    try expect_eq(matches[2].start, 2);
    try expect_eq(matches[2].end, 6);
}

test "aho_corasick: nested suffix output links (a, ab, bab, bc, bca, c, caa)" {
    const alloc = std.testing.allocator;
    const pats: [7][]const u8 = [ "a", "ab", "bab", "bc", "bca", "c", "caa" ];
    let ac = try AhoCorasick.init(alloc, &pats);
    defer ac.deinit(alloc);

    const matches = try ac.findAllOverlapping(alloc, "abccab");
    defer alloc.free(matches);
    // "a"(0..1), "ab"(0..2), "bc"(1..3), "c"(2..3), "c"(3..4), "a"(4..5), "ab"(4..6)
    try expect_eq(matches.len, 7);
}

test "aho_corasick: containsAny fast zero-alloc check" {
    const alloc = std.testing.allocator;
    const pats: [2][]const u8 = [ "error", "panic" ];
    let ac = try AhoCorasick.init(alloc, &pats);
    defer ac.deinit(alloc);

    try expect_eq(ac.containsAny("all systems nominal"), false);
    try expect_eq(ac.containsAny("kernel panic detected"), true);
    try expect_eq(ac.containsAny(""), false);
}

test "aho_corasick: empty pattern list and empty pattern ignored" {
    const alloc = std.testing.allocator;
    const pats: [2][]const u8 = [ "", "xyz" ];
    let ac = try AhoCorasick.init(alloc, &pats);
    defer ac.deinit(alloc);

    const m = try ac.findAllOverlapping(alloc, "abcxyz");
    defer alloc.free(m);
    try expect_eq(m.len, 1);
    try expect_eq(m[0].pattern_id, 1);
    try expect_eq(m[0].start, 3);
}

test "aho_corasick: cap.aho_corasick dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    const pats: [2][]const u8 = [ "foo", "bar" ];
    let ac = try cap.ds.AhoCorasick.init(alloc, &pats);
    defer ac.deinit(alloc);
    try expect_eq(ac.containsAny("foobar"), true);
}
