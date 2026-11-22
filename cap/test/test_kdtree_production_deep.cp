// Source: cp/cap/test/test_kdtree_production_deep.cp
// Deep Production Verification Suite for cap:kdtree & cap.kdtree
// Covers:
//   1. 2D/3D/5D high-dimensional build & nearest/kNearest 100% exact equivalence against brute-force Euclidean distance
//   2. withinRadius spherical search & rangeSearch AABB hyperrectangle search equivalence against brute-force scan
//   3. Dynamic incremental insert & empty tree boundary (nearest == null)
//   4. Error guards: DimensionMismatch, InvalidDimension (dims=0), InvalidRadius, InvalidBounds (min > max)
//   5. `import cap from "cap"; cap.kdtree` dynamic mounting & std.testing.allocator zero-leak verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KdError,
    KdPointEntry,
    KdNeighbor,
    KdTree,
} from "cap:ds";

fn nextRandState(state: u64): u64 {
    return state *% 6364136223846793005 +% 1442695040888963407;
}

fn randUnit(state: u64): f64 {
    const shifted = (state >> 11) & 0x1FFFFFFFFFFFFF;
    return f64(shifted) / 9007199254740992.0;
}

fn bruteDistSq(a: []const f64, b: []const f64): f64 {
    let sum: f64 = 0.0;
    for (i of 0..a.len) {
        const d = a[i] - b[i];
        sum += d * d;
    }
    return sum;
}

fn sortBruteDistSq(arr: []f64) {
    if (arr.len <= 1) return;
    for (i of 0..arr.len) {
        let best = i;
        for (j of (i + 1)..arr.len) {
            if (arr[j] < arr[best]) best = j;
        }
        if (best != i) {
            const tmp = arr[i];
            arr[i] = arr[best];
            arr[best] = tmp;
        }
    }
}

fn sortU64Asc(arr: []u64) {
    if (arr.len <= 1) return;
    for (i of 0..arr.len) {
        let best = i;
        for (j of (i + 1)..arr.len) {
            if (arr[j] < arr[best]) best = j;
        }
        if (best != i) {
            const tmp = arr[i];
            arr[i] = arr[best];
            arr[best] = tmp;
        }
    }
}

fn verifyDimBuildAndKnn(alloc: std.mem.Allocator, dims: usize, n: usize, seed: u64) {
    let rng: u64 = seed;
    let raw_coords = alloc.alloc(f64, n * dims) catch unreachable;
    defer alloc.free(raw_coords);

    let entries = alloc.alloc(KdPointEntry(u64), n) catch unreachable;
    defer alloc.free(entries);

    for (i of 0..n) {
        const base = i * dims;
        for (d of 0..dims) {
            rng = nextRandState(rng);
            raw_coords[base + d] = (randUnit(rng) - 0.5) * 200.0;
        }
        entries[i] = KdPointEntry<u64>{
            coords: raw_coords[base..(base + dims)],
            value: u64(i),
        };
    }

    let tree = KdTree<u64>.build(alloc, dims, entries) catch unreachable;
    defer tree.deinit();

    expect_eq(tree.len(), n);
    expect_eq(tree.dims(), dims);
    expect_eq(tree.isEmpty(), false);

    let query = alloc.alloc(f64, dims) catch unreachable;
    defer alloc.free(query);

    let brute_dists = alloc.alloc(f64, n) catch unreachable;
    defer alloc.free(brute_dists);

    for (q_idx of 0..15) {
        for (d of 0..dims) {
            rng = nextRandState(rng);
            query[d] = (randUnit(rng) - 0.5) * 220.0;
        }
        for (i of 0..n) {
            brute_dists[i] = bruteDistSq(entries[i].coords, query);
        }
        sortBruteDistSq(brute_dists);

        const nn = tree.nearest(query) catch unreachable;
        expect_eq(nn != null, true);
        if (nn != null) {
            const hit = nn.?;
            expect_eq(hit.dist_sq == brute_dists[0], true);
            expect_eq(@abs(hit.dist - std.math.sqrt(brute_dists[0])) < 0.000000001, true);
        }

        const k: usize = 7;
        const knn = tree.kNearest(alloc, query, k) catch unreachable;
        defer alloc.free(knn);
        expect_eq(knn.len, k);
        for (j of 0..k) {
            expect_eq(knn[j].dist_sq == brute_dists[j], true);
            if (j > 0) {
                expect_eq(knn[j - 1].dist_sq <= knn[j].dist_sq, true);
            }
        }
        * = q_idx;
    }
}

test("cap:kdtree - 2D/3D/5D build & nearest/kNearest 100% match with brute force") {
    const alloc = std.testing.allocator;
    verifyDimBuildAndKnn(alloc, 2, 200, 0x1111222233334444);
    verifyDimBuildAndKnn(alloc, 3, 220, 0x5555666677778888);
    verifyDimBuildAndKnn(alloc, 5, 250, 0x9999AAAABBBBCCCC);
}

test("cap:kdtree - withinRadius & rangeSearch exact match against brute-force scan") {
    const alloc = std.testing.allocator;
    const dims: usize = 3;
    const n: usize = 300;

    let rng: u64 = 0xCAFEBABEDEADBEEF;
    let raw_coords = alloc.alloc(f64, n * dims) catch unreachable;
    defer alloc.free(raw_coords);

    let entries = alloc.alloc(KdPointEntry(u64), n) catch unreachable;
    defer alloc.free(entries);

    for (i of 0..n) {
        const base = i * dims;
        for (d of 0..dims) {
            rng = nextRandState(rng);
            raw_coords[base + d] = (randUnit(rng) - 0.5) * 100.0;
        }
        entries[i] = KdPointEntry<u64>{
            coords: raw_coords[base..(base + dims)],
            value: u64(i),
        };
    }

    let tree = KdTree<u64>.build(alloc, dims, entries) catch unreachable;
    defer tree.deinit();

    // Spherical radius search
    const q: [3]f64 = [5.0, -10.0, 12.5];
    const radius: f64 = 35.0;
    const r_sq = radius * radius;

    const rad_hits = tree.withinRadius(alloc, *q, radius) catch unreachable;
    defer alloc.free(rad_hits);

    let brute_rad_ids: std.ArrayListUnmanaged(u64) = .empty;
    defer brute_rad_ids.deinit(alloc);
    for (e of entries) {
        if (bruteDistSq(e.coords, *q) <= r_sq) {
            brute_rad_ids.append(alloc, e.value) catch unreachable;
        }
    }

    expect_eq(rad_hits.len, brute_rad_ids.items.len);
    let kd_rad_ids = alloc.alloc(u64, rad_hits.len) catch unreachable;
    defer alloc.free(kd_rad_ids);
    for (i of 0..rad_hits.len) {
        kd_rad_ids[i] = rad_hits[i].value;
        if (i > 0) {
            expect_eq(rad_hits[i - 1].dist_sq <= rad_hits[i].dist_sq, true);
        }
    }
    sortU64Asc(kd_rad_ids);
    sortU64Asc(brute_rad_ids.items);
    for (i of 0..kd_rad_ids.len) {
        expect_eq(kd_rad_ids[i], brute_rad_ids.items[i]);
    }

    // AABB range search
    const min_b: [3]f64 = [-25.0, -15.0, -30.0];
    const max_b: [3]f64 = [20.0, 25.0, 15.0];

    const box_hits = tree.rangeSearch(alloc, *min_b, *max_b) catch unreachable;
    defer alloc.free(box_hits);

    let brute_box_ids: std.ArrayListUnmanaged(u64) = .empty;
    defer brute_box_ids.deinit(alloc);
    for (e of entries) {
        if (e.coords[0] >= min_b[0] && e.coords[0] <= max_b[0] &&
            e.coords[1] >= min_b[1] && e.coords[1] <= max_b[1] &&
            e.coords[2] >= min_b[2] && e.coords[2] <= max_b[2]) {
            brute_box_ids.append(alloc, e.value) catch unreachable;
        }
    }

    expect_eq(box_hits.len, brute_box_ids.items.len);
    let kd_box_ids = alloc.alloc(u64, box_hits.len) catch unreachable;
    defer alloc.free(kd_box_ids);
    for (i of 0..box_hits.len) {
        kd_box_ids[i] = box_hits[i].value;
    }
    sortU64Asc(kd_box_ids);
    sortU64Asc(brute_box_ids.items);
    for (i of 0..kd_box_ids.len) {
        expect_eq(kd_box_ids[i], brute_box_ids.items[i]);
    }
}

test("cap:kdtree - dynamic incremental insert & empty tree boundary") {
    const alloc = std.testing.allocator;
    let tree = KdTree<u64>.initEmpty(alloc, 3) catch unreachable;
    defer tree.deinit();

    expect_eq(tree.isEmpty(), true);
    expect_eq(tree.len(), 0);
    expect_eq(tree.dims(), 3);

    const q: [3]f64 = [0.0, 0.0, 0.0];
    const empty_nn = tree.nearest(*q) catch unreachable;
    expect_eq(empty_nn == null, true);

    const empty_knn = tree.kNearest(alloc, *q, 5) catch unreachable;
    defer alloc.free(empty_knn);
    expect_eq(empty_knn.len, 0);

    const empty_rad = tree.withinRadius(alloc, *q, 10.0) catch unreachable;
    defer alloc.free(empty_rad);
    expect_eq(empty_rad.len, 0);

    const min_b: [3]f64 = [-1.0, -1.0, -1.0];
    const max_b: [3]f64 = [1.0, 1.0, 1.0];
    const empty_box = tree.rangeSearch(alloc, *min_b, *max_b) catch unreachable;
    defer alloc.free(empty_box);
    expect_eq(empty_box.len, 0);

    // Dynamic inserts
    const p1: [3]f64 = [10.0, 10.0, 10.0];
    const p2: [3]f64 = [1.0, 2.0, 2.0];
    const p3: [3]f64 = [-3.0, 4.0, 0.0];
    const p4: [3]f64 = [0.5, 0.5, 0.5];
    tree.insert(*p1, 101) catch unreachable;
    tree.insert(*p2, 102) catch unreachable;
    tree.insert(*p3, 103) catch unreachable;
    tree.insert(*p4, 104) catch unreachable;

    expect_eq(tree.isEmpty(), false);
    expect_eq(tree.len(), 4);

    const nn = tree.nearest(*q) catch unreachable;
    expect_eq(nn != null, true);
    if (nn != null) {
        expect_eq(nn.?.value, 104);
    }

    const knn = tree.kNearest(alloc, *q, 10) catch unreachable;
    defer alloc.free(knn);
    expect_eq(knn.len, 4);
    expect_eq(knn[0].value, 104);
    expect_eq(knn[1].value, 102);
    expect_eq(knn[2].value, 103);
    expect_eq(knn[3].value, 101);
}

test("cap:kdtree - InvalidDimension, DimensionMismatch, InvalidRadius & InvalidBounds guards") {
    const alloc = std.testing.allocator;

    let bad_dim = KdTree<u64>.initEmpty(alloc, 0);
    expect_eq(bad_dim == KdError.InvalidDimension, true);

    const empty_entries: [0]KdPointEntry<u64> = [];
    let bad_build_dim = KdTree<u64>.build(alloc, 0, *empty_entries);
    expect_eq(bad_build_dim == KdError.InvalidDimension, true);

    const p2: [2]f64 = [1.0, 2.0];
    const mismatch_entries: [1]KdPointEntry<u64> = [
        KdPointEntry<u64>{ coords: *p2, value: 1 },
    ];
    let bad_build_mismatch = KdTree<u64>.build(alloc, 3, *mismatch_entries);
    expect_eq(bad_build_mismatch == KdError.DimensionMismatch, true);

    let tree = KdTree<u64>.initEmpty(alloc, 3) catch unreachable;
    defer tree.deinit();

    let bad_insert = tree.insert(*p2, 42);
    expect_eq(bad_insert == KdError.DimensionMismatch, true);

    let bad_nn = tree.nearest(*p2);
    expect_eq(bad_nn == KdError.DimensionMismatch, true);

    let bad_knn = tree.kNearest(alloc, *p2, 2);
    expect_eq(bad_knn == KdError.DimensionMismatch, true);

    let bad_rad_dim = tree.withinRadius(alloc, *p2, 5.0);
    expect_eq(bad_rad_dim == KdError.DimensionMismatch, true);

    const p3: [3]f64 = [1.0, 2.0, 3.0];
    let bad_rad_neg = tree.withinRadius(alloc, *p3, -0.5);
    expect_eq(bad_rad_neg == KdError.InvalidRadius, true);

    const min_bad: [3]f64 = [0.0, 5.0, 0.0];
    const max_bad: [3]f64 = [1.0, 4.0, 1.0];
    let bad_bounds = tree.rangeSearch(alloc, *min_bad, *max_bad);
    expect_eq(bad_bounds == KdError.InvalidBounds, true);

    let bad_range_dim = tree.rangeSearch(alloc, *p2, *max_bad);
    expect_eq(bad_range_dim == KdError.DimensionMismatch, true);
}

test("cap:kdtree - cap.kdtree dynamic mounting & std.testing.allocator zero-leak") {
    const alloc = std.testing.allocator;

    let tree = cap.ds.KdTree<u64>.initEmpty(alloc, 4) catch unreachable;
    defer tree.deinit();

    const a: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const b: [4]f64 = [5.0, 6.0, 7.0, 8.0];
    tree.insert(*a, 11) catch unreachable;
    tree.insert(*b, 22) catch unreachable;

    const q: [4]f64 = [4.5, 5.5, 6.5, 7.5];
    const nn = tree.nearest(*q) catch unreachable;
    expect_eq(nn != null, true);
    if (nn != null) {
        expect_eq(nn.?.value, 22);
    }
}
