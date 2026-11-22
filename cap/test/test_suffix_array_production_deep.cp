// Source: native-cp-stdlib (Suffix Array & Kasai LCP)
import std from "std";
import { SuffixArray } from "../../lib/ds/suffix_array_.cp";
import { test, expect_eq } from "cap:test";

test "SuffixArray: banana SA and Kasai LCP exact match" {
    const alloc = std.testing.allocator;
    let sa = try SuffixArray.init(alloc, "banana");
    defer sa.deinit();

    expect_eq(sa.len(), @as(usize, 6));

    const sa_slice = sa.saSlice();
    expect_eq(sa_slice.len, @as(usize, 6));
    expect_eq(sa_slice[0], @as(usize, 5));
    expect_eq(sa_slice[1], @as(usize, 3));
    expect_eq(sa_slice[2], @as(usize, 1));
    expect_eq(sa_slice[3], @as(usize, 0));
    expect_eq(sa_slice[4], @as(usize, 4));
    expect_eq(sa_slice[5], @as(usize, 2));

    const lcp_slice = sa.lcpSlice();
    expect_eq(lcp_slice.len, @as(usize, 6));
    expect_eq(lcp_slice[0], @as(usize, 0));
    expect_eq(lcp_slice[1], @as(usize, 1));
    expect_eq(lcp_slice[2], @as(usize, 3));
    expect_eq(lcp_slice[3], @as(usize, 0));
    expect_eq(lcp_slice[4], @as(usize, 0));
    expect_eq(lcp_slice[5], @as(usize, 2));
}

test "SuffixArray: searchAll binary search all occurrences" {
    const alloc = std.testing.allocator;
    let sa = try SuffixArray.init(alloc, "abracadabra");
    defer sa.deinit();

    const pos_a = try sa.searchAll(alloc, "a");
    defer alloc.free(pos_a);
    // Suffixes starting with "a": 10("a"), 7("abra"), 0("abracadabra"), 3("acadabra"), 5("adabra")
    expect_eq(pos_a.len, @as(usize, 5));
    expect_eq(pos_a[0], @as(usize, 10));
    expect_eq(pos_a[1], @as(usize, 7));
    expect_eq(pos_a[2], @as(usize, 0));
    expect_eq(pos_a[3], @as(usize, 3));
    expect_eq(pos_a[4], @as(usize, 5));

    const pos_abra = try sa.searchAll(alloc, "abra");
    defer alloc.free(pos_abra);
    expect_eq(pos_abra.len, @as(usize, 2));
    expect_eq(pos_abra[0], @as(usize, 7));
    expect_eq(pos_abra[1], @as(usize, 0));

    const pos_none = try sa.searchAll(alloc, "xyz");
    defer alloc.free(pos_none);
    expect_eq(pos_none.len, @as(usize, 0));
}

test "SuffixArray: contains and countOccurrences" {
    const alloc = std.testing.allocator;
    let sa = try SuffixArray.init(alloc, "mississippi");
    defer sa.deinit();

    expect_eq(sa.contains("iss"), true);
    expect_eq(sa.contains("ppi"), true);
    expect_eq(sa.contains("mississippi"), true);
    expect_eq(sa.contains("mississippis"), false);
    expect_eq(sa.contains("abc"), false);
    expect_eq(sa.contains(""), false);

    expect_eq(sa.countOccurrences("i"), @as(usize, 4));
    expect_eq(sa.countOccurrences("issi"), @as(usize, 2));
    expect_eq(sa.countOccurrences("ss"), @as(usize, 2));
    expect_eq(sa.countOccurrences("p"), @as(usize, 2));
    expect_eq(sa.countOccurrences("z"), @as(usize, 0));
    expect_eq(sa.countOccurrences(""), @as(usize, 0));
}

test "SuffixArray: longestRepeatedSubstring" {
    const alloc = std.testing.allocator;

    let sa1 = try SuffixArray.init(alloc, "banana");
    defer sa1.deinit();
    expect_eq(std.mem.eql(u8, sa1.longestRepeatedSubstring(), "ana"), true);

    let sa2 = try SuffixArray.init(alloc, "mississippi");
    defer sa2.deinit();
    expect_eq(std.mem.eql(u8, sa2.longestRepeatedSubstring(), "issi"), true);

    let sa3 = try SuffixArray.init(alloc, "abcdefg");
    defer sa3.deinit();
    expect_eq(sa3.longestRepeatedSubstring().len, @as(usize, 0));
}

test "SuffixArray: empty, single char, identical chars, and cap.suffix_array dynamic mount" {
    const alloc = std.testing.allocator;

    // Empty string boundary
    let sa_empty = try SuffixArray.init(alloc, "");
    defer sa_empty.deinit();
    expect_eq(sa_empty.len(), @as(usize, 0));
    expect_eq(sa_empty.saSlice().len, @as(usize, 0));
    expect_eq(sa_empty.lcpSlice().len, @as(usize, 0));
    expect_eq(sa_empty.contains("a"), false);
    expect_eq(sa_empty.countOccurrences("a"), @as(usize, 0));
    expect_eq(sa_empty.longestRepeatedSubstring().len, @as(usize, 0));

    // Single character boundary
    let sa_one = try SuffixArray.init(alloc, "x");
    defer sa_one.deinit();
    expect_eq(sa_one.len(), @as(usize, 1));
    expect_eq(sa_one.saSlice()[0], @as(usize, 0));
    expect_eq(sa_one.lcpSlice()[0], @as(usize, 0));
    expect_eq(sa_one.contains("x"), true);
    expect_eq(sa_one.longestRepeatedSubstring().len, @as(usize, 0));

    // Identical characters boundary ("aaaaa")
    let sa_same = try SuffixArray.init(alloc, "aaaaa");
    defer sa_same.deinit();
    const same_sa = sa_same.saSlice();
    const same_lcp = sa_same.lcpSlice();
    expect_eq(same_sa[0], @as(usize, 4));
    expect_eq(same_sa[1], @as(usize, 3));
    expect_eq(same_sa[2], @as(usize, 2));
    expect_eq(same_sa[3], @as(usize, 1));
    expect_eq(same_sa[4], @as(usize, 0));
    expect_eq(same_lcp[0], @as(usize, 0));
    expect_eq(same_lcp[1], @as(usize, 1));
    expect_eq(same_lcp[2], @as(usize, 2));
    expect_eq(same_lcp[3], @as(usize, 3));
    expect_eq(same_lcp[4], @as(usize, 4));
    expect_eq(sa_same.countOccurrences("aa"), @as(usize, 4));
    expect_eq(std.mem.eql(u8, sa_same.longestRepeatedSubstring(), "aaaa"), true);
}
