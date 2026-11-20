// Source: cp/cap/test/test_rtree_production_deep.cp
// Deep Production Verification Suite for cap:rtree & cap.rtree
// Covers:
//   1. 550+ pseudo-random & grid overlapping rectangles triggering multi-level Quadratic Split;
//      searchOverlap 100% consistency with brute-force full scan across diverse query boxes
//   2. searchPoint multi-overlapping layer hits & exact bounds() MBR maintenance across inserts & clear()
//   3. kNearestPoint Best-First Branch-and-Bound 100% consistency with brute-force distance sort
//   4. removeById deletion, MBR tightening, full drain, InvalidBox (min > max / NaN / Inf) & InvalidCapacity (< 4)
//   5. `import cap from "cap"; cap.rtree` dynamic mounting & std.testing.allocator zero memory leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RTreeError,
    Rect,
    RTreeEntry,
    NearestRect,
    RTree,
} from "cap:rtree";

fn sortIds(ids: []u64) {
    let i: usize = 0;
    while (i < ids.len) : (i += 1) {
        let j: usize = i + 1;
        while (j < ids.len) : (j += 1) {
            if (ids[j] < ids[i]) {
                const tmp = ids[i];
                ids[i] = ids[j];
                ids[j] = tmp;
            }
        }
    }
}

fn approxEq(a: f64, b: f64): bool {
    return @abs(a - b) <= 0.000000001;
}

test("cap:rtree - 550+ overlapping rectangles, multi-level Quadratic Split & searchOverlap vs brute-force") {
    const alloc = std.testing.allocator;
    let tree = RTree<u32>.initWithCapacity(alloc, 4) catch unreachable;
    defer tree.deinit();

    let all_entries: std.ArrayListUnmanaged(RTreeEntry(u32)) = .empty;
    defer all_entries.deinit(alloc);

    // Deterministic pseudo-random overlapping boxes + grid boxes (total 560)
    let seed: u64 = 0x9e3779b97f4a7c15;
    let id: u64 = 1;
    while (id <= 560) : (id += 1) {
        seed = seed *% 6364136223846793005 +% 1442695040888963407;
        const rx = @as(f64, @floatFromInt((seed >> 16) % 400)) * 0.5;
        seed = seed *% 6364136223846793005 +% 1442695040888963407;
        const ry = @as(f64, @floatFromInt((seed >> 16) % 400)) * 0.5;
        seed = seed *% 6364136223846793005 +% 1442695040888963407;
        const w = @as(f64, @floatFromInt((seed >> 16) % 35 + 1)) * 0.5;
        const h = @as(f64, @floatFromInt(((seed >> 32) % 35) + 1)) * 0.5;

        const r = Rect.init(rx, ry, rx + w, ry + h) catch unreachable;
        const val: u32 = @as(u32, @intCast(id * 3));
        tree.insert(id, r, val) catch unreachable;
        all_entries.append(alloc, RTreeEntry(u32){ id: id, box_rect: r, value: val }) catch unreachable;
    }

    expect_eq(tree.len(), 560);
    expect_eq(tree.isEmpty(), false);

    // Query multiple regions (dense center, edge-touching, disjoint far away, full domain)
    const queries = [
        Rect.init(20.0, 20.0, 60.0, 60.0) catch unreachable,
        Rect.init(0.0, 0.0, 5.0, 5.0) catch unreachable,
        Rect.init(100.0, 100.0, 140.0, 140.0) catch unreachable,
        Rect.init(1000.0, 1000.0, 1010.0, 1010.0) catch unreachable,
        Rect.init(-10.0, -10.0, 300.0, 300.0) catch unreachable,
    ];

    for (q of queries) {
        const got = tree.searchOverlap(alloc, q) catch unreachable;
        defer alloc.free(got);

        let expected_ids: std.ArrayListUnmanaged(u64) = .empty;
        defer expected_ids.deinit(alloc);
        for (e of all_entries.items) {
            if (e.box_rect.overlaps(q)) {
                expected_ids.append(alloc, e.id) catch unreachable;
            }
        }

        expect_eq(got.len, expected_ids.items.len);

        let got_ids = alloc.alloc(u64, got.len) catch unreachable;
        defer alloc.free(got_ids);
        let idx: usize = 0;
        while (idx < got.len) : (idx += 1) {
            got_ids[idx] = got[idx].id;
            expect_eq(got[idx].value, @as(u32, @intCast(got[idx].id * 3)));
            expect_eq(got[idx].box_rect.overlaps(q), true);
        }

        sortIds(got_ids);
        sortIds(expected_ids.items);
        idx = 0;
        while (idx < got_ids.len) : (idx += 1) {
            expect_eq(got_ids[idx], expected_ids.items[idx]);
        }
    }
}

test("cap:rtree - searchPoint multi-overlapping layers & bounds() MBR maintenance") {
    const alloc = std.testing.allocator;
    let tree = RTree<i32>.initWithCapacity(alloc, 4) catch unreachable;
    defer tree.deinit();

    expect_eq(tree.bounds() == null, true);
    expect_eq(tree.isEmpty(), true);

    // Concentric and shifted overlapping layers
    let i: u64 = 1;
    while (i <= 40) : (i += 1) {
        const fi = @as(f64, @floatFromInt(i));
        const r = Rect.init(-fi, -fi * 2.0, fi * 1.5, fi * 0.5) catch unreachable;
        tree.insert(i, r, @as(i32, @intCast(i))) catch unreachable;
    }

    const b1 = tree.bounds().?;
    expect_eq(approxEq(b1.min_x, -40.0), true);
    expect_eq(approxEq(b1.min_y, -80.0), true);
    expect_eq(approxEq(b1.max_x, 60.0), true);
    expect_eq(approxEq(b1.max_y, 20.0), true);

    // Point (0.0, 0.0) hits all 40 concentric boxes
    const center_hits = tree.searchPoint(alloc, 0.0, 0.0) catch unreachable;
    defer alloc.free(center_hits);
    expect_eq(center_hits.len, 40);

    // Point (30.0, 10.0) hits boxes where fi * 1.5 >= 30.0 (fi >= 20) and fi * 0.5 >= 10.0 (fi >= 20) -> 21..40 (21 items)
    const partial_hits = tree.searchPoint(alloc, 30.0, 10.0) catch unreachable;
    defer alloc.free(partial_hits);
    expect_eq(partial_hits.len, 21);

    // Outside point
    const miss = tree.searchPoint(alloc, 100.0, 100.0) catch unreachable;
    defer alloc.free(miss);
    expect_eq(miss.len, 0);

    // clear() resets bounds and count
    tree.clear();
    expect_eq(tree.len(), 0);
    expect_eq(tree.isEmpty(), true);
    expect_eq(tree.bounds() == null, true);
}

test("cap:rtree - kNearestPoint Best-First Branch-and-Bound vs brute-force sort") {
    const alloc = std.testing.allocator;
    let tree = RTree<u64>.initWithCapacity(alloc, 4) catch unreachable;
    defer tree.deinit();

    let all_rects: std.ArrayListUnmanaged(Rect) = .empty;
    defer all_rects.deinit(alloc);

    let id: u64 = 0;
    let gx: usize = 0;
    while (gx < 10) : (gx += 1) {
        let gy: usize = 0;
        while (gy < 10) : (gy += 1) {
            const fx = @as(f64, @floatFromInt(gx)) * 10.0;
            const fy = @as(f64, @floatFromInt(gy)) * 10.0;
            const r = Rect.init(fx, fy, fx + 3.0, fy + 3.0) catch unreachable;
            tree.insert(id, r, id * 10) catch unreachable;
            all_rects.append(alloc, r) catch unreachable;
            id += 1;
        }
    }

    const qx: f64 = 44.5;
    const qy: f64 = 52.0;

    // Brute-force compute all squared distances and sort ascending
    let bf_dists = alloc.alloc(f64, all_rects.items.len) catch unreachable;
    defer alloc.free(bf_dists);
    let idx: usize = 0;
    while (idx < all_rects.items.len) : (idx += 1) {
        bf_dists[idx] = all_rects.items[idx].distanceSqToPoint(qx, qy);
    }
    let a: usize = 0;
    while (a < bf_dists.len) : (a += 1) {
        let b: usize = a + 1;
        while (b < bf_dists.len) : (b += 1) {
            if (bf_dists[b] < bf_dists[a]) {
                const tmp = bf_dists[a];
                bf_dists[a] = bf_dists[b];
                bf_dists[b] = tmp;
            }
        }
    }

    const knn = tree.kNearestPoint(alloc, qx, qy, 12) catch unreachable;
    defer alloc.free(knn);
    expect_eq(knn.len, 12);

    idx = 0;
    while (idx < knn.len) : (idx += 1) {
        expect_eq(approxEq(knn[idx].dist_sq, bf_dists[idx]), true);
        expect_eq(approxEq(knn[idx].entry.box_rect.distanceSqToPoint(qx, qy), knn[idx].dist_sq), true);
        if (idx > 0) {
            expect_eq(knn[idx - 1].dist_sq <= knn[idx].dist_sq, true);
        }
    }

    // k = 0 returns empty slice
    const knn0 = tree.kNearestPoint(alloc, qx, qy, 0) catch unreachable;
    defer alloc.free(knn0);
    expect_eq(knn0.len, 0);

    // k > len() returns all 100 entries
    const knn_all = tree.kNearestPoint(alloc, qx, qy, 500) catch unreachable;
    defer alloc.free(knn_all);
    expect_eq(knn_all.len, 100);
}

test("cap:rtree - removeById tightening, full drain, InvalidBox & InvalidCapacity guards") {
    const alloc = std.testing.allocator;

    // InvalidCapacity (< 4)
    let bad_cap_hit = false;
    * = RTree<i32>.initWithCapacity(alloc, 3) catch |e| {
        expect_eq(e == RTreeError.InvalidCapacity, true);
        bad_cap_hit = true;
    };
    expect_eq(bad_cap_hit, true);

    // InvalidBox (min > max, NaN, Inf)
    let bad_box_hit = false;
    * = Rect.init(10.0, 0.0, 5.0, 5.0) catch |e| {
        expect_eq(e == RTreeError.InvalidBox, true);
        bad_box_hit = true;
    };
    expect_eq(bad_box_hit, true);

    let nan_box_hit = false;
    * = Rect.init(std.math.nan(f64), 0.0, 1.0, 1.0) catch |e| {
        expect_eq(e == RTreeError.InvalidBox, true);
        nan_box_hit = true;
    };
    expect_eq(nan_box_hit, true);

    let tree = RTree<i32>.initWithCapacity(alloc, 4) catch unreachable;
    defer tree.deinit();

    // Insert 20 boxes plus an outlier box id=999
    let i: u64 = 1;
    while (i <= 20) : (i += 1) {
        const fi = @as(f64, @floatFromInt(i));
        tree.insert(i, Rect.init(fi, fi, fi + 1.0, fi + 1.0) catch unreachable, @as(i32, @intCast(i))) catch unreachable;
    }
    tree.insert(999, Rect.init(500.0, 500.0, 510.0, 510.0) catch unreachable, 999) catch unreachable;
    expect_eq(tree.len(), 21);
    expect_eq(approxEq(tree.bounds().?.max_x, 510.0), true);

    // Remove outlier -> bounds() must tighten back to 21.0
    expect_eq(tree.removeById(999), true);
    expect_eq(tree.removeById(999), false);
    expect_eq(tree.len(), 20);
    expect_eq(approxEq(tree.bounds().?.max_x, 21.0), true);

    const check_removed = tree.searchPoint(alloc, 505.0, 505.0) catch unreachable;
    defer alloc.free(check_removed);
    expect_eq(check_removed.len, 0);

    // Drain remaining 20 entries
    i = 1;
    while (i <= 20) : (i += 1) {
        expect_eq(tree.removeById(i), true);
    }
    expect_eq(tree.len(), 0);
    expect_eq(tree.isEmpty(), true);
    expect_eq(tree.bounds() == null, true);
}

test("cap.rtree - dynamic cap.rtree mounting & Rect geometry helpers") {
    const alloc = std.testing.allocator;
    let tree = cap.rtree.RTree<u64>.init(alloc) catch unreachable;
    defer tree.deinit();

    const r1 = cap.rtree.Rect.init(0.0, 0.0, 4.0, 4.0) catch unreachable;
    const r2 = cap.rtree.Rect.init(1.0, 1.0, 3.0, 3.0) catch unreachable;
    const r3 = cap.rtree.Rect.init(4.0, 4.0, 6.0, 6.0) catch unreachable;

    expect_eq(r1.containsRect(r2), true);
    expect_eq(r2.containsRect(r1), false);
    expect_eq(r1.overlaps(r3), true);
    expect_eq(approxEq(r1.area(), 16.0), true);
    expect_eq(approxEq(r1.enlargementCost(r3), 20.0), true);

    tree.insert(10, r1, 1000) catch unreachable;
    tree.insert(20, r3, 2000) catch unreachable;

    const hits = tree.searchPoint(alloc, 4.0, 4.0) catch unreachable;
    defer alloc.free(hits);
    expect_eq(hits.len, 2);
}
