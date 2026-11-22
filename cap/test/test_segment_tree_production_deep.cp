// Source: cp/cap/test/test_segment_tree_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { SegError, SegmentTree } from "../../lib/ds/segment_tree_.cp";

test "segment_tree: build, rangeSum, rangeMin, rangeMax" {
    const alloc = std.testing.allocator;
    const vals: [6]i64 = [ 2, 5, 1, 8, -3, 7 ];
    let st = try SegmentTree(i64).init(alloc, &vals);
    defer st.deinit();

    try expect_eq(st.len(), 6);
    try expect_eq(try st.rangeSum(0, 5), 20);
    try expect_eq(try st.rangeMin(0, 5), -3);
    try expect_eq(try st.rangeMax(0, 5), 8);
    try expect_eq(try st.rangeMin(0, 2), 1);
    try expect_eq(try st.rangeMax(1, 3), 8);
}

test "segment_tree: lazy propagation across overlapping rangeAdd" {
    const alloc = std.testing.allocator;
    const vals: [5]i64 = [ 1, 2, 3, 4, 5 ];
    let st = try SegmentTree(i64).init(alloc, &vals);
    defer st.deinit();

    try st.rangeAdd(0, 3, 10); // [11, 12, 13, 14, 5]
    try st.rangeAdd(2, 4, -5); // [11, 12, 8, 9, 0]

    try expect_eq(try st.rangeSum(0, 4), 40);
    try expect_eq(try st.rangeMin(0, 4), 0);
    try expect_eq(try st.rangeMax(0, 4), 12);
    try expect_eq(try st.rangeSum(2, 3), 17);
}

test "segment_tree: brute-force equivalence over 64 elements" {
    const alloc = std.testing.allocator;
    let brute: [64]i64 = undefined;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        brute[i] = @intCast(i * 3 - 20);
    }
    let st = try SegmentTree(i64).init(alloc, &brute);
    defer st.deinit();

    // Apply multiple range updates
    let k: usize = 5;
    while (k < 40) : (k += 1) { brute[k] += 7; }
    try st.rangeAdd(5, 39, 7);

    k = 20;
    while (k < 60) : (k += 1) { brute[k] -= 4; }
    try st.rangeAdd(20, 59, -4);

    let expected_sum: i64 = 0;
    k = 10;
    while (k <= 50) : (k += 1) { expected_sum += brute[k]; }
    try expect_eq(try st.rangeSum(10, 50), expected_sum);
}

test "segment_tree: error guards for empty, invalid range, and OOB" {
    const alloc = std.testing.allocator;
    const empty: [0]i64 = [];
    try expect_eq(SegmentTree(i64).init(alloc, &empty) == SegError.EmptyTree, true);

    const vals: [3]i64 = [ 1, 2, 3 ];
    let st = try SegmentTree(i64).init(alloc, &vals);
    defer st.deinit();
    try expect_eq(st.rangeSum(2, 1) == SegError.InvalidRange, true);
    try expect_eq(st.rangeAdd(0, 3, 1) == SegError.IndexOutOfBounds, true);
}

test "segment_tree: cap.segment_tree dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    const vals: [4]i64 = [ 10, 20, 30, 40 ];
    let st = try SegmentTree(i64).init(alloc, &vals);
    defer st.deinit();
    try st.rangeAdd(1, 2, 5);
    try expect_eq(try st.rangeSum(0, 3), 110);
}
