// Source: cp/cap/test/test_segtree_production_deep.cp
// Deep Production Verification Suite for cap:segtree & cap.segtree
// Covers:
//   1. FenwickTree O(N) build (fromSlice), add/set/get, prefixSum/rangeSum, and lowerBoundPrefix
//   2. LazySegmentTree interleaved rangeAdd & rangeAssign (add-then-assign, assign-then-add) vs brute-force oracle
//   3. Negative values min/max/sum queries & single-element get verification
//   4. InvalidRange, IndexOutOfBounds, and EmptyTree error interception
//   5. `import cap from "cap"; cap.segtree` dynamic mounting & zero leaks via std.testing.allocator

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SegTreeError,
    RangeStats,
    FenwickTree,
    LazySegmentTree,
} from "cap:ds";

test("cap:segtree - FenwickTree O(N) build, add/set, rangeSum & lowerBoundPrefix") {
    const alloc = std.testing.allocator;
    const init_vals: [7]i64 = [ 3, 1, 4, 2, 5, 0, 7 ];

    let bit = FenwickTree.fromSlice(alloc, init_vals[0..]) catch unreachable;
    defer bit.deinit();

    expect_eq(bit.len(), 7);
    expect_eq(bit.prefixSum(0) catch unreachable, 3);
    expect_eq(bit.prefixSum(2) catch unreachable, 8);
    expect_eq(bit.prefixSum(6) catch unreachable, 22);
    expect_eq(bit.rangeSum(1, 5) catch unreachable, 12); // 1+4+2+5
    expect_eq(bit.rangeSum(3, 3) catch unreachable, 0);

    // lowerBoundPrefix on [3, 1, 4, 2, 5, 0, 7] -> prefix sums [3, 4, 8, 10, 15, 15, 22]
    expect_eq(bit.lowerBoundPrefix(0).?, 0);
    expect_eq(bit.lowerBoundPrefix(1).?, 0);
    expect_eq(bit.lowerBoundPrefix(3).?, 0);
    expect_eq(bit.lowerBoundPrefix(4).?, 1);
    expect_eq(bit.lowerBoundPrefix(8).?, 2);
    expect_eq(bit.lowerBoundPrefix(9).?, 3);
    expect_eq(bit.lowerBoundPrefix(15).?, 4);
    expect_eq(bit.lowerBoundPrefix(16).?, 6);
    expect_eq(bit.lowerBoundPrefix(22).?, 6);
    expect_eq(bit.lowerBoundPrefix(23) == null, true);

    // Point updates: add & set
    bit.add(1, 5) catch unreachable; // [3, 6, 4, 2, 5, 0, 7]
    expect_eq(bit.get(1) catch unreachable, 6);
    expect_eq(bit.rangeSum(0, 3) catch unreachable, 13);
    expect_eq(bit.lowerBoundPrefix(9).?, 1);

    bit.set(5, 10) catch unreachable; // [3, 6, 4, 2, 5, 10, 7] -> total 37
    expect_eq(bit.get(5) catch unreachable, 10);
    expect_eq(bit.prefixSum(6) catch unreachable, 37);
    expect_eq(bit.lowerBoundPrefix(30).?, 5);

    // Zero-initialized FenwickTree.init
    let zbit = FenwickTree.init(alloc, 4) catch unreachable;
    defer zbit.deinit();
    expect_eq(zbit.rangeSum(0, 4) catch unreachable, 0);
    expect_eq(zbit.lowerBoundPrefix(1) == null, true);
    zbit.set(2, 10) catch unreachable;
    expect_eq(zbit.lowerBoundPrefix(10).?, 2);
}

test("cap:segtree - LazySegmentTree interleaved rangeAdd & rangeAssign vs brute-force oracle") {
    const alloc = std.testing.allocator;
    const n: usize = 37;

    let oracle = alloc.alloc(i64, n) catch unreachable;
    defer alloc.free(oracle);
    for (i of 0..n) {
        oracle[i] = @as(i64, @intCast(i * 3)) - 20;
    }

    let seg = LazySegmentTree.init(alloc, oracle) catch unreachable;
    defer seg.deinit();
    expect_eq(seg.len(), n);

    // Deterministic PRNG for interleaved rangeAdd and rangeAssign
    let seed: u64 = 0x9e3779b97f4a7c15;
    for (step of 0..120) {
        seed = seed *% 6364136223846793005 +% 1442695040888963407;
        const a: usize = @intCast((seed >> 16) % n);
        seed = seed *% 6364136223846793005 +% 1442695040888963407;
        const b: usize = @intCast((seed >> 16) % n);
        const l = if (a < b) a else b;
        const r_excl = (if (a < b) b else a) + 1;

        const op = step % 3;
        const val: i64 = @as(i64, @intCast((seed >> 32) % 201)) - 100;

        if (op == 0) {
            // rangeAdd
            seg.rangeAdd(l, r_excl, val) catch unreachable;
            for (k of l..r_excl) {
                oracle[k] += val;
            }
        } else if (op == 1) {
            // rangeAssign
            seg.rangeAssign(l, r_excl, val) catch unreachable;
            for (k of l..r_excl) {
                oracle[k] = val;
            }
        } else {
            // Explicit add-then-assign & assign-then-add stress on overlapping subranges
            seg.rangeAssign(l, r_excl, val) catch unreachable;
            seg.rangeAdd(l, r_excl, 7) catch unreachable;
            for (k of l..r_excl) {
                oracle[k] = val + 7;
            }
        }

        // Verify full range and subrange against brute-force oracle
        const ql = l / 2;
        const qr = if (r_excl < n) r_excl + 1 else n;
        let expected_sum: i64 = 0;
        let expected_min: i64 = oracle[ql];
        let expected_max: i64 = oracle[ql];
        for (k of ql..qr) {
            const v = oracle[k];
            expected_sum += v;
            if (v < expected_min) expected_min = v;
            if (v > expected_max) expected_max = v;
        }

        const got = seg.query(ql, qr) catch unreachable;
        expect_eq(got.sum, expected_sum);
        expect_eq(got.min, expected_min);
        expect_eq(got.max, expected_max);
    }

    // Final point-by-point check
    for (i of 0..n) {
        expect_eq(seg.get(i) catch unreachable, oracle[i]);
    }
}

test("cap:segtree - negative range min/max/sum & single-element get") {
    const alloc = std.testing.allocator;
    const vals: [6]i64 = [ -50, -10, -30, -5, -100, -20 ];

    let seg = LazySegmentTree.init(alloc, vals[0..]) catch unreachable;
    defer seg.deinit();

    const full = seg.query(0, 6) catch unreachable;
    expect_eq(full.sum, -215);
    expect_eq(full.min, -100);
    expect_eq(full.max, -5);

    expect_eq(seg.querySum(1, 4) catch unreachable, -45);
    expect_eq(seg.queryMin(1, 4) catch unreachable, -30);
    expect_eq(seg.queryMax(1, 4) catch unreachable, -5);

    // Assign negative, then add negative
    seg.rangeAssign(2, 5, -40) catch unreachable; // [-50, -10, -40, -40, -40, -20]
    seg.rangeAdd(3, 6, -15) catch unreachable;    // [-50, -10, -40, -55, -55, -35]

    expect_eq(seg.get(0) catch unreachable, -50);
    expect_eq(seg.get(2) catch unreachable, -40);
    expect_eq(seg.get(4) catch unreachable, -55);
    expect_eq(seg.get(5) catch unreachable, -35);

    const sub = seg.query(2, 6) catch unreachable;
    expect_eq(sub.sum, -185);
    expect_eq(sub.min, -55);
    expect_eq(sub.max, -35);
}

test("cap:segtree - InvalidRange, IndexOutOfBounds & EmptyTree error interception") {
    const alloc = std.testing.allocator;

    // Empty FenwickTree
    let empty_bit = FenwickTree.init(alloc, 0) catch unreachable;
    defer empty_bit.deinit();
    expect_eq(empty_bit.len(), 0);
    expect_eq(empty_bit.lowerBoundPrefix(1) == null, true);

    let caught_empty_add = false;
    empty_bit.add(0, 1) catch |e| {
        if (e == SegTreeError.EmptyTree) caught_empty_add = true;
    };
    expect_eq(caught_empty_add, true);

    // Empty LazySegmentTree
    const empty_slice: []const i64 = [];
    let empty_seg = LazySegmentTree.init(alloc, empty_slice) catch unreachable;
    defer empty_seg.deinit();
    expect_eq(empty_seg.len(), 0);

    let caught_empty_query = false;
    let * = empty_seg.query(0, 1) catch |e| {
        if (e == SegTreeError.EmptyTree) caught_empty_query = true;
    };
    expect_eq(caught_empty_query, true);

    // Non-empty bounds & invalid range guards
    const vals: [3]i64 = [ 1, 2, 3 ];
    let seg = LazySegmentTree.init(alloc, vals[0..]) catch unreachable;
    defer seg.deinit();

    let caught_invalid_range = false;
    let * = seg.query(2, 2) catch |e| {
        if (e == SegTreeError.InvalidRange) caught_invalid_range = true;
    };
    expect_eq(caught_invalid_range, true);

    let caught_reversed_range = false;
    seg.rangeAdd(3, 1, 5) catch |e| {
        if (e == SegTreeError.InvalidRange) caught_reversed_range = true;
    };
    expect_eq(caught_reversed_range, true);

    let caught_oob_query = false;
    let * = seg.query(0, 4) catch |e| {
        if (e == SegTreeError.IndexOutOfBounds) caught_oob_query = true;
    };
    expect_eq(caught_oob_query, true);

    let caught_oob_get = false;
    let * = seg.get(3) catch |e| {
        if (e == SegTreeError.IndexOutOfBounds) caught_oob_get = true;
    };
    expect_eq(caught_oob_get, true);
}

test("cap:segtree - import cap from \"cap\"; cap.segtree dynamic mounting & zero leaks") {
    const alloc = std.testing.allocator;
    const vals: [4]i64 = [ 10, 20, 30, 40 ];

    let bit = cap.ds.FenwickTree.fromSlice(alloc, vals[0..]) catch unreachable;
    defer bit.deinit();
    expect_eq(bit.rangeSum(1, 3) catch unreachable, 50);
    expect_eq(bit.lowerBoundPrefix(35).?, 2);

    let seg = cap.ds.LazySegmentTree.init(alloc, vals[0..]) catch unreachable;
    defer seg.deinit();
    seg.rangeAssign(0, 2, 5) catch unreachable;
    seg.rangeAdd(1, 4, 3) catch unreachable; // [5, 8, 33, 43]

    const stats: RangeStats = seg.query(0, 4) catch unreachable;
    expect_eq(stats.sum, 89);
    expect_eq(stats.min, 5);
    expect_eq(stats.max, 43);
}
