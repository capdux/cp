// Source: cp/cap/test/test_fenwick_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { FenwickError, FenwickTree } from "../../lib/ds/fenwick_.cp";

test "fenwick: initFromSlice, prefixSum, rangeSum, add, set" {
    const alloc = std.testing.allocator;
    const vals: [6]i64 = [ 3, 2, -1, 6, 5, 4 ];
    let ft = try FenwickTree(i64).initFromSlice(alloc, &vals);
    defer ft.deinit();

    try expect_eq(ft.len(), 6);
    try expect_eq(try ft.prefixSum(3), 10);
    try expect_eq(try ft.rangeSum(1, 4), 12);
    try expect_eq(try ft.get(2), -1);

    try ft.add(2, 5); // -1 -> 4
    try expect_eq(try ft.get(2), 4);
    try expect_eq(try ft.rangeSum(1, 4), 17);

    try ft.set(0, 10); // 3 -> 10
    try expect_eq(try ft.prefixSum(1), 12);
}

test "fenwick: lowerBound binary lifting on cumulative frequencies" {
    const alloc = std.testing.allocator;
    const freqs: [5]u64 = [ 2, 3, 5, 1, 4 ]; // prefix: 2, 5, 10, 11, 15
    let ft = try FenwickTree(u64).initFromSlice(alloc, &freqs);
    defer ft.deinit();

    try expect_eq(ft.lowerBound(1).?, 0);
    try expect_eq(ft.lowerBound(2).?, 0);
    try expect_eq(ft.lowerBound(3).?, 1);
    try expect_eq(ft.lowerBound(6).?, 2);
    try expect_eq(ft.lowerBound(15).?, 4);
    try expect_eq(ft.lowerBound(16) == null, true);
}

test "fenwick: out of bounds and invalid range guards" {
    const alloc = std.testing.allocator;
    let ft = try FenwickTree(i32).init(alloc, 4);
    defer ft.deinit();

    try expect_eq(ft.add(4, 1) == FenwickError.IndexOutOfBounds, true);
    try expect_eq(ft.rangeSum(3, 1) == FenwickError.InvalidRange, true);
    try expect_eq(ft.rangeSum(0, 4) == FenwickError.IndexOutOfBounds, true);
}

test "fenwick: large 1000-element stress accuracy" {
    const alloc = std.testing.allocator;
    let ft = try FenwickTree(i64).init(alloc, 1000);
    defer ft.deinit();

    let i: usize = 0;
    while (i < 1000) : (i += 1) {
        try ft.add(i, @intCast(i + 1));
    }
    try expect_eq(try ft.prefixSum(999), 500500);
    try expect_eq(try ft.rangeSum(99, 199), 15150);
}

test "fenwick: cap.fenwick dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    let ft = try FenwickTree(i64).init(alloc, 8);
    defer ft.deinit();
    try ft.add(3, 42);
    try expect_eq(try ft.prefixSum(7), 42);
}
