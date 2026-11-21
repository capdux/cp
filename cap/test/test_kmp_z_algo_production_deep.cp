// Source: cp/cap/test/test_kmp_z_algo_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { computeLps, kmpFindFirst, kmpFindAll, zArray, longestPalindrome } from "cap:kmp_z_algo";

test "kmp_z_algo: KMP LPS table & overlapping matches" {
    const alloc = std.testing.allocator;
    const lps = try computeLps(alloc, "ABABCABAB");
    defer alloc.free(lps);
    const expected_lps: [9]usize = [ 0, 0, 1, 2, 0, 1, 2, 3, 4 ];
    try expect_eq(std.mem.eql(usize, lps, &expected_lps), true);

    const hits = try kmpFindAll(alloc, "ababa", "aba");
    defer alloc.free(hits);
    try expect_eq(hits.len, 2);
    try expect_eq(hits[0], 0);
    try expect_eq(hits[1], 2);
}

test "kmp_z_algo: kmpFindFirst hit, miss, and empty pattern" {
    const alloc = std.testing.allocator;
    try expect_eq((try kmpFindFirst(alloc, "hello world", "world")).?, 6);
    try expect_eq((try kmpFindFirst(alloc, "hello world", "xyz")) == null, true);
    try expect_eq((try kmpFindFirst(alloc, "abc", "")).?, 0);
}

test "kmp_z_algo: Z-algorithm exact array verification" {
    const alloc = std.testing.allocator;
    const z = try zArray(alloc, "aabxaabxcaabxaabxay");
    defer alloc.free(z);
    // z[0]=19, z[1]=1, z[4]=4, z[9]=8
    try expect_eq(z[0], 19);
    try expect_eq(z[1], 1);
    try expect_eq(z[4], 4);
    try expect_eq(z[9], 8);
}

test "kmp_z_algo: Manacher longestPalindrome odd, even, and single char" {
    const alloc = std.testing.allocator;
    const odd_str = "forgeeksskeegfor";
    const s1 = try longestPalindrome(alloc, odd_str);
    try expect_eq(std.mem.eql(u8, odd_str[s1.start..s1.end], "geeksskeeg"), true);

    const s2 = try longestPalindrome(alloc, "racecar");
    try expect_eq(s2.start, 0);
    try expect_eq(s2.end, 7);

    const s3 = try longestPalindrome(alloc, "x");
    try expect_eq(s3.end - s3.start, 1);
}

test "kmp_z_algo: cap.kmp_z_algo dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    const hits = try cap.kmp_z_algo.kmpFindAll(alloc, "aaaa", "aa");
    defer alloc.free(hits);
    try expect_eq(hits.len, 3);
}
