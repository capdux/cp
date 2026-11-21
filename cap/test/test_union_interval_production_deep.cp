// Source: cp/cap/test/test_union_interval_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { RangeError, IntervalSet } from "cap:union_interval";

test "union_interval: addRange merges overlapping and adjacent intervals" {
    const alloc = std.testing.allocator;
    let set = IntervalSet(i64).init(alloc);
    defer set.deinit();

    try set.addRange(10, 20);
    try set.addRange(25, 35);
    try set.addRange(20, 25); // bridges [10, 20) and [25, 35) -> [10, 35)
    try expect_eq(set.count(), 1);
    try expect_eq(set.items()[0].start, 10);
    try expect_eq(set.items()[0].end, 35);
    try expect_eq(set.totalCovered(), 25);
}

test "union_interval: removeRange splits middle & trims ends" {
    const alloc = std.testing.allocator;
    let set = IntervalSet(i64).init(alloc);
    defer set.deinit();

    try set.addRange(0, 100);
    try set.removeRange(20, 40); // [0, 20), [40, 100)
    try expect_eq(set.count(), 2);
    try set.removeRange(80, 120); // [0, 20), [40, 80)
    try expect_eq(set.count(), 2);
    try expect_eq(set.totalCovered(), 60);
    try expect_eq(set.containsPoint(19), true);
    try expect_eq(set.containsPoint(20), false);
    try expect_eq(set.containsPoint(40), true);
    try expect_eq(set.containsPoint(80), false);
}

test "union_interval: empty range no-op & InvalidRange guard" {
    const alloc = std.testing.allocator;
    let set = IntervalSet(i32).init(alloc);
    defer set.deinit();

    try set.addRange(5, 5);
    try expect_eq(set.count(), 0);
    try expect_eq(set.addRange(10, 5) == RangeError.InvalidRange, true);
    try expect_eq(set.removeRange(10, 5) == RangeError.InvalidRange, true);
}

test "union_interval: clear resets state cleanly" {
    const alloc = std.testing.allocator;
    let set = IntervalSet(u64).init(alloc);
    defer set.deinit();
    try set.addRange(1, 50);
    set.clear();
    try expect_eq(set.count(), 0);
    try expect_eq(set.totalCovered(), 0);
}

test "union_interval: cap.union_interval dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    let set = cap.union_interval.IntervalSet(i64).init(alloc);
    defer set.deinit();
    try set.addRange(1, 5);
    try set.addRange(3, 10);
    try expect_eq(set.totalCovered(), 9);
}
