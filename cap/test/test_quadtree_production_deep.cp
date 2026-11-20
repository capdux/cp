// Source: cp/cap/test/test_quadtree_production_deep.cp
// Deep Production Verification Suite for cap:quadtree & cap.quadtree
// Covers:
//   1. 500+ points & duplicate-coordinate insertion, splitting, OutOfBounds/InvalidConfig/InvalidBounds guards, len() & nodeCount()
//   2. queryRect & queryCircle 100% exact equivalence against brute-force linear scan
//   3. nearestK exact Top-K equivalence & ascending distance ordering against brute-force sort
//   4. remove / update relocation, dynamic node collapse, and std.testing.allocator zero-leak check
//   5. `import cap from "cap"; cap.quadtree` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    QuadError,
    QuadEntry,
    QuadHit,
    QuadTree,
} from "cap:quadtree";

fn nextRand(state: *u64): f64 {
    state.* = state.* *% 6364136223846793005 +% 1442695040888963407;
    const shifted = (state.* >> 11) & 0x1FFFFFFFFFFFFF;
    return f64(shifted) / 9007199254740992.0;
}

fn sortU64Asc(arr: []u64) {
    if (arr.len <= 1) return;
    for (i of 0..arr.len) {
        let min_idx = i;
        for (j of (i + 1)..arr.len) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }
        if (min_idx != i) {
            const tmp = arr[i];
            arr[i] = arr[min_idx];
            arr[min_idx] = tmp;
        }
    }
}

fn sortHitsBrute(arr: []QuadHit) {
    if (arr.len <= 1) return;
    for (i of 0..arr.len) {
        let best = i;
        for (j of (i + 1)..arr.len) {
            if (arr[j].dist_sq < arr[best].dist_sq || (arr[j].dist_sq == arr[best].dist_sq && arr[j].id < arr[best].id)) {
                best = j;
            }
        }
        if (best != i) {
            const tmp = arr[i];
            arr[i] = arr[best];
            arr[best] = tmp;
        }
    }
}

test("cap:quadtree - 500+ points, duplicate coords, OutOfBounds & config validation") {
    const alloc = std.testing.allocator;

    // Invalid bounds & config guards
    let bad_bounds = QuadTree.init(alloc, 10.0, 0.0, 10.0, 100.0, 4, 8);
    expect_eq(bad_bounds == QuadError.InvalidBounds, true);

    let bad_cfg = QuadTree.init(alloc, 0.0, 0.0, 100.0, 100.0, 0, 8);
    expect_eq(bad_cfg == QuadError.InvalidConfig, true);

    let qt = QuadTree.init(alloc, 0.0, 0.0, 1000.0, 1000.0, 4, 10) catch unreachable;
    defer qt.deinit();

    expect_eq(qt.len(), 0);
    expect_eq(qt.nodeCount(), 1);

    // OutOfBounds interception
    let oob1 = qt.insert(9999, -0.01, 500.0);
    expect_eq(oob1 == QuadError.OutOfBounds, true);
    let oob2 = qt.insert(9998, 500.0, 1000.01);
    expect_eq(oob2 == QuadError.OutOfBounds, true);
    expect_eq(qt.len(), 0);

    // Insert 550 pseudo-random points
    let rng: u64 = 0x123456789ABCDEF0;
    for (i of 0..550) {
        const x = nextRand(*rng) * 1000.0;
        const y = nextRand(*rng) * 1000.0;
        qt.insert(u64(i), x, y) catch unreachable;
    }
    expect_eq(qt.len(), 550);
    expect_eq(qt.nodeCount() > 1, true);

    // Insert 25 identical-coordinate points (must not trigger infinite split)
    const nodes_before_dup = qt.nodeCount();
    for (j of 0..25) {
        qt.insert(u64(10000 + j), 250.25, 750.75) catch unreachable;
    }
    expect_eq(qt.len(), 575);
    expect_eq(qt.nodeCount() >= nodes_before_dup, true);

    // Verify all 25 duplicate-coord points are retrievable
    const dup_hits = qt.queryRect(alloc, 250.24, 750.74, 250.26, 750.76) catch unreachable;
    defer alloc.free(dup_hits);
    let dup_count: usize = 0;
    for (h of dup_hits) {
        if (h.id >= 10000) dup_count += 1;
    }
    expect_eq(dup_count, 25);
}

test("cap:quadtree - queryRect & queryCircle exact match against brute-force scan") {
    const alloc = std.testing.allocator;
    let qt = QuadTree.init(alloc, -500.0, -500.0, 500.0, 500.0, 6, 10) catch unreachable;
    defer qt.deinit();

    const N: usize = 600;
    let pts = alloc.alloc(QuadEntry, N) catch unreachable;
    defer alloc.free(pts);

    let rng: u64 = 0xCAFEBABE12345678;
    for (i of 0..N) {
        const x = nextRand(*rng) * 1000.0 - 500.0;
        const y = nextRand(*rng) * 1000.0 - 500.0;
        const entry = QuadEntry{ id: u64(i), x: x, y: y };
        pts[i] = entry;
        qt.insert(entry.id, entry.x, entry.y) catch unreachable;
    }

    // Rectangular AOI query comparison
    const r_min_x: f64 = -120.0;
    const r_min_y: f64 = -80.0;
    const r_max_x: f64 = 240.0;
    const r_max_y: f64 = 190.0;

    const qt_rect = qt.queryRect(alloc, r_min_x, r_min_y, r_max_x, r_max_y) catch unreachable;
    defer alloc.free(qt_rect);

    let bf_rect_ids: std.ArrayList(u64) = .empty;
    defer bf_rect_ids.deinit(alloc);
    for (p of pts) {
        if (p.x >= r_min_x && p.x <= r_max_x && p.y >= r_min_y && p.y <= r_max_y) {
            bf_rect_ids.append(alloc, p.id) catch unreachable;
        }
    }

    expect_eq(qt_rect.len, bf_rect_ids.items.len);
    let qt_rect_ids = alloc.alloc(u64, qt_rect.len) catch unreachable;
    defer alloc.free(qt_rect_ids);
    for (i of 0..qt_rect.len) {
        qt_rect_ids[i] = qt_rect[i].id;
    }
    sortU64Asc(qt_rect_ids);
    sortU64Asc(bf_rect_ids.items);
    for (i of 0..qt_rect_ids.len) {
        expect_eq(qt_rect_ids[i], bf_rect_ids.items[i]);
    }

    // Circular radius query comparison
    const cx: f64 = 75.0;
    const cy: f64 = -45.0;
    const radius: f64 = 165.0;
    const r_sq = radius * radius;

    const qt_cir = qt.queryCircle(alloc, cx, cy, radius) catch unreachable;
    defer alloc.free(qt_cir);

    let bf_cir_ids: std.ArrayList(u64) = .empty;
    defer bf_cir_ids.deinit(alloc);
    for (p of pts) {
        const dx = p.x - cx;
        const dy = p.y - cy;
        if (dx * dx + dy * dy <= r_sq) {
            bf_cir_ids.append(alloc, p.id) catch unreachable;
        }
    }

    expect_eq(qt_cir.len, bf_cir_ids.items.len);
    let qt_cir_ids = alloc.alloc(u64, qt_cir.len) catch unreachable;
    defer alloc.free(qt_cir_ids);
    for (i of 0..qt_cir.len) {
        qt_cir_ids[i] = qt_cir[i].id;
    }
    sortU64Asc(qt_cir_ids);
    sortU64Asc(bf_cir_ids.items);
    for (i of 0..qt_cir_ids.len) {
        expect_eq(qt_cir_ids[i], bf_cir_ids.items[i]);
    }
}

test("cap:quadtree - nearestK exact Top-K & ascending order against brute-force") {
    const alloc = std.testing.allocator;
    let qt = QuadTree.init(alloc, 0.0, 0.0, 1000.0, 1000.0, 5, 10) catch unreachable;
    defer qt.deinit();

    const N: usize = 520;
    let pts = alloc.alloc(QuadEntry, N) catch unreachable;
    defer alloc.free(pts);

    let rng: u64 = 0xDEADBEEF87654321;
    for (i of 0..N) {
        const x = nextRand(*rng) * 1000.0;
        const y = nextRand(*rng) * 1000.0;
        const entry = QuadEntry{ id: u64(i), x: x, y: y };
        pts[i] = entry;
        qt.insert(entry.id, entry.x, entry.y) catch unreachable;
    }

    // k = 0 returns empty slice
    const k0 = qt.nearestK(alloc, 500.0, 500.0, 0) catch unreachable;
    defer alloc.free(k0);
    expect_eq(k0.len, 0);

    // Query interior & exterior points with k = 15
    const qx: f64 = 333.3;
    const qy: f64 = 666.7;
    const k: usize = 15;

    const knn = qt.nearestK(alloc, qx, qy, k) catch unreachable;
    defer alloc.free(knn);
    expect_eq(knn.len, k);

    let bf_hits = alloc.alloc(QuadHit, N) catch unreachable;
    defer alloc.free(bf_hits);
    for (i of 0..N) {
        const dx = pts[i].x - qx;
        const dy = pts[i].y - qy;
        bf_hits[i] = QuadHit{
            id: pts[i].id,
            x: pts[i].x,
            y: pts[i].y,
            dist_sq: dx * dx + dy * dy,
        };
    }
    sortHitsBrute(bf_hits);

    for (i of 0..k) {
        expect_eq(knn[i].id, bf_hits[i].id);
        expect_eq(knn[i].dist_sq, bf_hits[i].dist_sq);
        if (i > 0) {
            expect_eq(knn[i - 1].dist_sq <= knn[i].dist_sq, true);
        }
    }

    // k > total_len returns all N sorted
    const knn_all = qt.nearestK(alloc, 1200.0, -100.0, 9999) catch unreachable;
    defer alloc.free(knn_all);
    expect_eq(knn_all.len, N);
    for (i of 1..N) {
        expect_eq(knn_all[i - 1].dist_sq <= knn_all[i].dist_sq, true);
    }
}

test("cap:quadtree - remove, update relocation, node collapse & zero memory leaks") {
    const alloc = std.testing.allocator;
    let qt = QuadTree.init(alloc, 0.0, 0.0, 100.0, 100.0, 2, 8) catch unreachable;
    defer qt.deinit();

    // Insert 12 points across quadrants to force multiple splits
    let xs: [12]f64 = [10.0, 15.0, 20.0, 80.0, 85.0, 90.0, 12.0, 18.0, 82.0, 88.0, 45.0, 55.0];
    let ys: [12]f64 = [10.0, 15.0, 20.0, 10.0, 15.0, 20.0, 80.0, 85.0, 80.0, 85.0, 45.0, 55.0];
    for (i of 0..12) {
        qt.insert(u64(i + 1), xs[i], ys[i]) catch unreachable;
    }
    expect_eq(qt.len(), 12);
    expect_eq(qt.nodeCount() > 5, true);

    // Update out-of-bounds must fail without deleting original point
    let bad_upd = qt.update(1, 10.0, 10.0, 150.0, 50.0);
    expect_eq(bad_upd == QuadError.OutOfBounds, true);
    expect_eq(qt.len(), 12);

    // Update non-existent entry returns false
    const miss_upd = qt.update(999, 10.0, 10.0, 50.0, 50.0) catch unreachable;
    expect_eq(miss_upd, false);

    // Relocate point 1 from SW (10, 10) to NE (95, 95)
    const ok_upd = qt.update(1, 10.0, 10.0, 95.0, 95.0) catch unreachable;
    expect_eq(ok_upd, true);
    expect_eq(qt.len(), 12);

    const old_q = qt.queryRect(alloc, 9.0, 9.0, 11.0, 11.0) catch unreachable;
    defer alloc.free(old_q);
    expect_eq(old_q.len, 0);

    const new_q = qt.queryRect(alloc, 94.0, 94.0, 96.0, 96.0) catch unreachable;
    defer alloc.free(new_q);
    expect_eq(new_q.len, 1);
    expect_eq(new_q[0].id, 1);

    // Remove non-existent returns false
    expect_eq(qt.remove(1, 10.0, 10.0), false);

    // Remove all points except 1 and verify dynamic collapse brings nodeCount back to 1
    expect_eq(qt.remove(1, 95.0, 95.0), true);
    for (i of 1..11) {
        expect_eq(qt.remove(u64(i + 1), xs[i], ys[i]), true);
    }
    expect_eq(qt.len(), 1);
    expect_eq(qt.nodeCount(), 1);
}

test("cap.quadtree - dynamic lazy-mount equivalence via import cap") {
    const alloc = std.testing.allocator;
    let qt = cap.quadtree.QuadTree.init(alloc, -100.0, -100.0, 100.0, 100.0, 4, 6) catch unreachable;
    defer qt.deinit();

    qt.insert(101, 10.0, 10.0) catch unreachable;
    qt.insert(102, 20.0, 20.0) catch unreachable;
    qt.insert(103, -50.0, -50.0) catch unreachable;
    expect_eq(qt.len(), 3);

    const rect = qt.queryRect(alloc, 0.0, 0.0, 30.0, 30.0) catch unreachable;
    defer alloc.free(rect);
    expect_eq(rect.len, 2);

    const knn = qt.nearestK(alloc, 11.0, 11.0, 1) catch unreachable;
    defer alloc.free(knn);
    expect_eq(knn.len, 1);
    expect_eq(knn[0].id, 101);
}
