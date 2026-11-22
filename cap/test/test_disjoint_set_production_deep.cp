// Source: cp/cap/test/test_disjoint_set_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { DsuError, DisjointSet } from "../../lib/ds/disjoint_set_.cp";

test "disjoint_set: union, path compression, and component tracking" {
    const alloc = std.testing.allocator;
    let dsu = try DisjointSet.init(alloc, 6);
    defer dsu.deinit();

    try expect_eq(dsu.components(), 6);
    try expect_eq(try dsu.unite(0, 1), true);
    try expect_eq(try dsu.unite(1, 2), true);
    try expect_eq(try dsu.unite(0, 2), false); // already connected
    try expect_eq(dsu.components(), 4);
    try expect_eq(try dsu.connected(0, 2), true);
    try expect_eq(try dsu.connected(0, 3), false);
    try expect_eq(try dsu.componentSize(2), 3);
}

test "disjoint_set: chain compression & Kruskal-style batch merge" {
    const alloc = std.testing.allocator;
    const n: usize = 500;
    let dsu = try DisjointSet.init(alloc, n);
    defer dsu.deinit();

    let i: usize = 0;
    while (i + 1 < n) : (i += 1) {
        * = try dsu.unite(i, i + 1);
    }
    try expect_eq(dsu.components(), 1);
    try expect_eq(try dsu.componentSize(0), n);
    try expect_eq(try dsu.connected(0, n - 1), true);
}

test "disjoint_set: reset restores initial singletons" {
    const alloc = std.testing.allocator;
    let dsu = try DisjointSet.init(alloc, 4);
    defer dsu.deinit();
    * = try dsu.unite(0, 3);
    try expect_eq(dsu.components(), 3);
    dsu.reset();
    try expect_eq(dsu.components(), 4);
    try expect_eq(try dsu.connected(0, 3), false);
}

test "disjoint_set: IndexOutOfBounds guard" {
    const alloc = std.testing.allocator;
    let dsu = try DisjointSet.init(alloc, 3);
    defer dsu.deinit();
    try expect_eq(dsu.find(3) == DsuError.IndexOutOfBounds, true);
    try expect_eq(dsu.unite(0, 5) == DsuError.IndexOutOfBounds, true);
}

test "disjoint_set: cap.disjoint_set dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    let dsu = try DisjointSet.init(alloc, 5);
    defer dsu.deinit();
    * = try dsu.unite(2, 4);
    try expect_eq(try dsu.connected(2, 4), true);
}
