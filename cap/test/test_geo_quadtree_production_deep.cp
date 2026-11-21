// Source: cp/cap/test/test_geo_quadtree_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { QuadError, Rect, QuadTree } from "cap:geo_quadtree";

test "geo_quadtree: insert, subdivide, and queryRect accuracy" {
    const alloc = std.testing.allocator;
    const world = Rect{ .cx = 50.0, .cy = 50.0, .half_w = 50.0, .half_h = 50.0 };
    let qt = try QuadTree(usize).init(alloc, world, 2);
    defer qt.deinit();

    let i: usize = 0;
    while (i < 50) : (i += 1) {
        const fi: f64 = @floatFromInt(i);
        const ok = try qt.insert(fi * 1.8 + 1.0, fi * 1.8 + 1.0, i);
        try expect_eq(ok, true);
    }
    try expect_eq(qt.len(), 50);

    // Out-of-bounds point rejected
    try expect_eq(try qt.insert(150.0, 50.0, 999), false);

    const hits = try qt.queryRect(alloc, Rect{ .cx = 20.0, .cy = 20.0, .half_w = 10.0, .half_h = 10.0 });
    defer alloc.free(hits);
    // x, y in [10, 30] => fi*1.8+1 in [10, 30] => i in [5..16] => 12 points
    try expect_eq(hits.len, 12);
}

test "geo_quadtree: queryRadius Euclidean circle filtering" {
    const alloc = std.testing.allocator;
    const world = Rect{ .cx = 0.0, .cy = 0.0, .half_w = 100.0, .half_h = 100.0 };
    let qt = try QuadTree(u8).init(alloc, world, 4);
    defer qt.deinit();

    // (3, 4) dist = 5.0; (8, 8) dist = sqrt(128) > 10.0; (0, 0) dist = 0.0
    * = try qt.insert(0.0, 0.0, 1);
    * = try qt.insert(3.0, 4.0, 2);
    * = try qt.insert(8.0, 8.0, 3);

    const hits = try qt.queryRadius(alloc, 0.0, 0.0, 6.0);
    defer alloc.free(hits);
    try expect_eq(hits.len, 2);
}

test "geo_quadtree: coincident points hit depth cap cleanly without infinite split" {
    const alloc = std.testing.allocator;
    const world = Rect{ .cx = 0.0, .cy = 0.0, .half_w = 10.0, .half_h = 10.0 };
    let qt = try QuadTree(usize).init(alloc, world, 1);
    defer qt.deinit();

    let i: usize = 0;
    while (i < 10) : (i += 1) {
        * = try qt.insert(1.0, 1.0, i);
    }
    try expect_eq(qt.len(), 10);
    const hits = try qt.queryRadius(alloc, 1.0, 1.0, 0.1);
    defer alloc.free(hits);
    try expect_eq(hits.len, 10);
}

test "geo_quadtree: error guards for zero capacity & invalid bounds" {
    const alloc = std.testing.allocator;
    const valid = Rect{ .cx = 0.0, .cy = 0.0, .half_w = 10.0, .half_h = 10.0 };
    const bad = Rect{ .cx = 0.0, .cy = 0.0, .half_w = -5.0, .half_h = 10.0 };
    try expect_eq(QuadTree(u8).init(alloc, valid, 0) == QuadError.InvalidCapacity, true);
    try expect_eq(QuadTree(u8).init(alloc, bad, 4) == QuadError.InvalidBounds, true);
}

test "geo_quadtree: cap.geo_quadtree dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    const world = cap.geo_quadtree.Rect{ .cx = 0.0, .cy = 0.0, .half_w = 50.0, .half_h = 50.0 };
    let qt = try cap.geo_quadtree.QuadTree(i32).init(alloc, world, 4);
    defer qt.deinit();
    * = try qt.insert(5.0, -5.0, 77);
    const hits = try qt.queryRadius(alloc, 5.0, -5.0, 1.0);
    defer alloc.free(hits);
    try expect_eq(hits.len, 1);
    try expect_eq(hits[0].value, 77);
}
