// cap/test/testTestRunnerExpectSeq.cp
// Production-grade deep tests for cap:test_runner expectSeq.cp (toBeEmpty, toBeArrayOfSize, multiset & order matchers)

import { test, assert, assertEq } from "cap:test";
import {
    toBeEmptyBytes,
    toBeEmptyStrSlice,
    toBeEmptyI64Slice,
    toHaveLengthBytes,
    toBeArrayOfSizeStr,
    toBeArrayOfSizeI64,
    toContainStr,
    toContainI64,
    toIncludeAllMembersStr,
    toIncludeAllMembersI64,
    toIncludeAnyMembersStr,
    toIncludeAnyMembersI64,
    toIncludeSameOrderStr,
    toIncludeSameOrderI64,
    toBeSortedAscI64,
    toBeSortedDescI64,
    toBeUniqueI64,
    toBeUniqueStr,
    ExpectSeqMatcher,
} from "cap:test_runner";

test("ExpectSeq: toBeEmpty & toHaveLength / toBeArrayOfSize", () => {
    const empty_s: [0][]const u8 = [];
    const empty_n: [0]i64 = [];
    const tags: [3][]const u8 = ["a", "b", "c"];
    const nums: [2]i64 = [10, 20];

    assert(toBeEmptyBytes(""));
    assert(!toBeEmptyBytes("x"));
    assert(toBeEmptyStrSlice(empty_s[0..]));
    assert(!toBeEmptyStrSlice(tags[0..]));
    assert(toBeEmptyI64Slice(empty_n[0..]));
    assert(!toBeEmptyI64Slice(nums[0..]));

    assert(toHaveLengthBytes("hello", 5));
    assert(!toHaveLengthBytes("hello", 4));
    assert(toBeArrayOfSizeStr(tags[0..], 3));
    assert(!toBeArrayOfSizeStr(tags[0..], 2));
    assert(toBeArrayOfSizeI64(nums[0..], 2));
    assert(!toBeArrayOfSizeI64(nums[0..], 0));
});

test("ExpectSeq: toContain & toIncludeAnyMembers", () => {
    const tags: [3][]const u8 = ["alpha", "beta", "gamma"];
    const cands_hit: [2][]const u8 = ["delta", "beta"];
    const cands_miss: [2][]const u8 = ["delta", "omega"];

    assert(toContainStr(tags[0..], "gamma"));
    assert(!toContainStr(tags[0..], "Gamma"));
    assert(toIncludeAnyMembersStr(tags[0..], cands_hit[0..]));
    assert(!toIncludeAnyMembersStr(tags[0..], cands_miss[0..]));

    const nums: [3]i64 = [-5, 0, 99];
    const n_hit: [2]i64 = [1, 99];
    const n_miss: [2]i64 = [1, 2];
    assert(toContainI64(nums[0..], -5));
    assert(!toContainI64(nums[0..], 5));
    assert(toIncludeAnyMembersI64(nums[0..], n_hit[0..]));
    assert(!toIncludeAnyMembersI64(nums[0..], n_miss[0..]));
});

test("ExpectSeq: toIncludeAllMembers multiset multiplicity accuracy", () => {
    const a: [3][]const u8 = ["x", "y", "x"];
    const b: [3][]const u8 = ["x", "x", "y"];
    const c: [3][]const u8 = ["x", "y", "y"];
    const d: [2][]const u8 = ["x", "y"];

    assert(toIncludeAllMembersStr(a[0..], b[0..]));
    assert(!toIncludeAllMembersStr(a[0..], c[0..]));
    assert(!toIncludeAllMembersStr(a[0..], d[0..]));

    const na: [4]i64 = [1, 2, 1, 3];
    const nb: [4]i64 = [3, 1, 2, 1];
    const nc: [4]i64 = [1, 2, 2, 3];
    assert(toIncludeAllMembersI64(na[0..], nb[0..]));
    assert(!toIncludeAllMembersI64(na[0..], nc[0..]));
});

test("ExpectSeq: toIncludeSameOrder strict positional equality", () => {
    const s1: [3][]const u8 = ["a", "b", "c"];
    const s2: [3][]const u8 = ["a", "b", "c"];
    const s3: [3][]const u8 = ["a", "c", "b"];
    assert(toIncludeSameOrderStr(s1[0..], s2[0..]));
    assert(!toIncludeSameOrderStr(s1[0..], s3[0..]));

    const n1: [3]i64 = [1, 2, 3];
    const n2: [3]i64 = [1, 2, 3];
    const n3: [3]i64 = [1, 3, 2];
    assert(toIncludeSameOrderI64(n1[0..], n2[0..]));
    assert(!toIncludeSameOrderI64(n1[0..], n3[0..]));
});

test("ExpectSeq: toBeSortedAscI64 / toBeSortedDescI64 / toBeUnique", () => {
    const empty_n: [0]i64 = [];
    const one_n: [1]i64 = [42];
    const asc: [4]i64 = [-10, 0, 0, 5];
    const desc: [4]i64 = [9, 5, 5, -1];
    const unsorted: [3]i64 = [1, 3, 2];

    assert(toBeSortedAscI64(empty_n[0..]));
    assert(toBeSortedAscI64(one_n[0..]));
    assert(toBeSortedAscI64(asc[0..]));
    assert(!toBeSortedAscI64(unsorted[0..]));

    assert(toBeSortedDescI64(empty_n[0..]));
    assert(toBeSortedDescI64(desc[0..]));
    assert(!toBeSortedDescI64(unsorted[0..]));

    const uniq_n: [3]i64 = [1, 2, 3];
    const dup_n: [3]i64 = [1, 2, 1];
    assert(toBeUniqueI64(uniq_n[0..]));
    assert(!toBeUniqueI64(dup_n[0..]));

    const uniq_s: [3][]const u8 = ["a", "b", "c"];
    const dup_s: [3][]const u8 = ["a", "b", "a"];
    assert(toBeUniqueStr(uniq_s[0..]));
    assert(!toBeUniqueStr(dup_s[0..]));
});

test("ExpectSeq: ExpectSeqMatcher stateful .not negation & counters", () => {
    let m = ExpectSeqMatcher.init();
    const s1: [2][]const u8 = ["k1", "k2"];
    const s2: [2][]const u8 = ["k2", "k1"];
    const n1: [3]i64 = [1, 2, 3];

    assert(m.checkEmptyBytes(""));
    assert(m.checkArrayOfSizeStr(s1[0..], 2));
    assert(m.checkArrayOfSizeI64(n1[0..], 3));
    assert(m.checkContainStr(s1[0..], "k2"));
    assert(m.checkContainI64(n1[0..], 2));
    assert(m.checkIncludeAllMembersStr(s1[0..], s2[0..]));
    assert(m.checkIncludeAllMembersI64(n1[0..], n1[0..]));
    assert(m.checkIncludeAnyMembersStr(s1[0..], s2[0..]));
    assert(m.checkIncludeAnyMembersI64(n1[0..], n1[0..]));
    assert(m.checkSortedAscI64(n1[0..]));
    assert(m.checkUniqueStr(s1[0..]));

    // Single-shot .not negation
    m.negate();
    assert(m.checkEmptyStr(s1[0..]));
    // Auto-reset after one assertion
    assert(!m.checkEmptyI64(n1[0..]));

    assertEq(m.assertions, @as(usize, 13));
    assertEq(m.passed, @as(usize, 12));
    assertEq(m.failed, @as(usize, 1));
});
