// Source: cp/cap/test/test_interval_tree_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { IntervalError, IntervalTree } from "cap:ds";

test "interval_tree: overlap queries and pruning correctness" {
    const alloc = std.testing.allocator;
    let tree = IntervalTree(i64, usize).init(alloc);
    defer tree.deinit();

    try tree.insert(15, 20, 1);
    try tree.insert(10, 30, 2);
    try tree.insert(17, 19, 3);
    try tree.insert(5, 20, 4);
    try tree.insert(12, 15, 5);
    try tree.insert(30, 40, 6);
    try expect_eq(tree.len(), 6);

    const hits = try tree.queryOverlap(alloc, 14, 16);
    defer alloc.free(hits);
    // Matches: [15,20], [10,30], [5,20], [12,15] -> 4 intervals
    try expect_eq(hits.len, 4);
}

test "interval_tree: queryPoint boundary inclusion" {
    const alloc = std.testing.allocator;
    let tree = IntervalTree(i32, []const u8).init(alloc);
    defer tree.deinit();

    try tree.insert(0, 10, "a");
    try tree.insert(10, 20, "b");
    try tree.insert(25, 30, "c");

    const at10 = try tree.queryPoint(alloc, 10);
    defer alloc.free(at10);
    try expect_eq(at10.len, 2);

    const at22 = try tree.queryPoint(alloc, 22);
    defer alloc.free(at22);
    try expect_eq(at22.len, 0);
}

test "interval_tree: hasOverlap fast boolean check" {
    const alloc = std.testing.allocator;
    let tree = IntervalTree(u64, u8).init(alloc);
    defer tree.deinit();

    try tree.insert(100, 200, 1);
    try tree.insert(300, 400, 2);

    try expect_eq(try tree.hasOverlap(201, 299), false);
    try expect_eq(try tree.hasOverlap(190, 250), true);
    try expect_eq(try tree.hasOverlap(350, 360), true);
}

test "interval_tree: InvalidInterval guard" {
    const alloc = std.testing.allocator;
    let tree = IntervalTree(i64, i64).init(alloc);
    defer tree.deinit();

    try expect_eq(tree.insert(50, 10, 1) == IntervalError.InvalidInterval, true);
    try expect_eq(tree.queryOverlap(alloc, 20, 10) == IntervalError.InvalidInterval, true);
    try expect_eq(tree.hasOverlap(20, 10) == IntervalError.InvalidInterval, true);
}

test "interval_tree: cap.interval_tree dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    let tree = cap.ds.IntervalTree(i64, i64).init(alloc);
    defer tree.deinit();
    try tree.insert(1, 5, 99);
    const hits = try tree.queryPoint(alloc, 3);
    defer alloc.free(hits);
    try expect_eq(hits.len, 1);
    try expect_eq(hits[0].value, 99);
}
