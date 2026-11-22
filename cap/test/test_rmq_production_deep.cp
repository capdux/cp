// Source: cp/cap/test/test_rmq_production_deep.cp (Production deep tests for cap:rmq)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    floorLog2,
    sparseTableCells,
    gcdU64,
    RmqMin,
    RmqMax,
    RmqArgMin,
    RangeGcd,
    EulerLca,
    slidingWindowMin,
    slidingWindowMax,
} from "cap:ds";

fn bruteMin(vals: []const i64, l: usize, r: usize): i64 {
    let best = vals[l];
    let i = l + 1;
    while (i <= r) {
        if (vals[i] < best) {
            best = vals[i];
        }
        i += 1;
    }
    return best;
}

fn bruteMax(vals: []const i64, l: usize, r: usize): i64 {
    let best = vals[l];
    let i = l + 1;
    while (i <= r) {
        if (vals[i] > best) {
            best = vals[i];
        }
        i += 1;
    }
    return best;
}

fn bruteArgMin(vals: []const i64, l: usize, r: usize): usize {
    let best_idx = l;
    let best_val = vals[l];
    let i = l + 1;
    while (i <= r) {
        if (vals[i] < best_val) {
            best_val = vals[i];
            best_idx = i;
        }
        i += 1;
    }
    return best_idx;
}

fn bruteGcd(vals: []const u64, l: usize, r: usize): u64 {
    let g = vals[l];
    let i = l + 1;
    while (i <= r) {
        g = gcdU64(g, vals[i]);
        i += 1;
    }
    return g;
}

test("1. Exhaustive all 528 closed intervals on 32-element signed array: RmqMin, RmqMax & RmqArgMin", () => {
    // 32 elements with negatives, zeros, extremes, and repeated minimums to stress tie-breaking
    const vals: [32]i64 = [
        15, -4, 7, -4, 22, 0, -19, 8,
        8, -19, 3, 100, -100, 42, -4, 11,
        -100, 5, 9, -7, 0, 0, 64, -33,
        12, -33, 99, -1, 2, -100, 77, -15,
    ];
    // sparseTableCells(32) = 32 * (5 + 1) = 192
    expect_eq(sparseTableCells(32), 192 as usize);

    let min_buf: [192]i64 = std.mem.zeroes([192]i64);
    let max_buf: [192]i64 = std.mem.zeroes([192]i64);
    let arg_buf: [192]usize = std.mem.zeroes([192]usize);

    const rmq_min = RmqMin.build(vals[0..], min_buf[0..]) ?? unreachable;
    const rmq_max = RmqMax.build(vals[0..], max_buf[0..]) ?? unreachable;
    const rmq_arg = RmqArgMin.build(vals[0..], arg_buf[0..]) ?? unreachable;

    let checked: usize = 0;
    let l: usize = 0;
    while (l < 32) {
        let r = l;
        while (r < 32) {
            const q_min = rmq_min.query(l, r) ?? unreachable;
            const q_max = rmq_max.query(l, r) ?? unreachable;
            const q_arg = rmq_arg.queryIndex(vals[0..], l, r) ?? unreachable;

            expect_eq(q_min, bruteMin(vals[0..], l, r));
            expect_eq(q_max, bruteMax(vals[0..], l, r));
            expect_eq(q_arg, bruteArgMin(vals[0..], l, r));
            expect_eq(vals[q_arg], q_min);

            checked += 1;
            r += 1;
        }
        l += 1;
    }
    expect_eq(checked, 528 as usize);
});

test("2. Exhaustive all 300 intervals on 24-element array: RangeGcd & gcdU64 identities", () => {
    expect_eq(gcdU64(0, 0), 0 as u64);
    expect_eq(gcdU64(42, 0), 42 as u64);
    expect_eq(gcdU64(0, 18), 18 as u64);
    expect_eq(gcdU64(1071, 462), 21 as u64);

    const nums: [24]u64 = [
        24, 36, 60, 12, 0, 48, 18, 90,
        15, 45, 75, 30, 14, 28, 42, 7,
        21, 63, 13, 26, 39, 52, 65, 130,
    ];
    // sparseTableCells(24) = 24 * (4 + 1) = 120
    expect_eq(sparseTableCells(24), 120 as usize);

    let gcd_buf: [120]u64 = std.mem.zeroes([120]u64);
    const rg = RangeGcd.build(nums[0..], gcd_buf[0..]) ?? unreachable;

    let checked: usize = 0;
    let l: usize = 0;
    while (l < 24) {
        let r = l;
        while (r < 24) {
            const q = rg.query(l, r) ?? unreachable;
            expect_eq(q, bruteGcd(nums[0..], l, r));
            checked += 1;
            r += 1;
        }
        l += 1;
    }
    expect_eq(checked, 300 as usize);
});

test("3. Sliding window min & max across widths k = 1, 3, 5, 12, n", () => {
    const vals: [20]i64 = [
        9, 3, -5, -5, 12, 8, 4, 4, -10, 7,
        6, 5, 4, 3, 2, 1, 0, -1, 15, -20,
    ];
    const ks: [5]usize = [1, 3, 5, 12, 20];
    let out_min: [20]i64 = std.mem.zeroes([20]i64);
    let out_max: [20]i64 = std.mem.zeroes([20]i64);
    let dq: [20]usize = std.mem.zeroes([20]usize);

    let ki: usize = 0;
    while (ki < 5) {
        const k = ks[ki];
        const expected_len: usize = vals.len - k + 1;

        const n_min = slidingWindowMin(vals[0..], k, out_min[0..], dq[0..]) ?? unreachable;
        const n_max = slidingWindowMax(vals[0..], k, out_max[0..], dq[0..]) ?? unreachable;
        expect_eq(n_min, expected_len);
        expect_eq(n_max, expected_len);

        let w: usize = 0;
        while (w < expected_len) {
            expect_eq(out_min[w], bruteMin(vals[0..], w, w + k - 1));
            expect_eq(out_max[w], bruteMax(vals[0..], w, w + k - 1));
            w += 1;
        }
        ki += 1;
    }
});

test("4. Euler-tour O(1) LCA on 7-node rooted tree for all 49 vertex pairs", () => {
    // Tree rooted at 0:
    //        0 (d=0)
    //       / \
    //      1   2 (d=1)
    //     / \   \
    //    3   4   5 (d=2)
    //        |
    //        6 (d=3)
    // Euler tour (length 2*7 - 1 = 13):
    // idx:   0  1  2  3  4  5  6  7  8  9 10 11 12
    // node:  0  1  3  1  4  6  4  1  0  2  5  2  0
    // depth: 0  1  2  1  2  3  2  1  0  1  2  1  0
    const euler_nodes: [13]usize = [0, 1, 3, 1, 4, 6, 4, 1, 0, 2, 5, 2, 0];
    const euler_depth: [13]i64 = [0, 1, 2, 1, 2, 3, 2, 1, 0, 1, 2, 1, 0];
    const first_occ: [7]usize = [0, 1, 9, 2, 4, 10, 5];

    // Expected 7x7 LCA matrix
    const expected: [49]usize = [
        0, 0, 0, 0, 0, 0, 0,
        0, 1, 0, 1, 1, 0, 1,
        0, 0, 2, 0, 0, 2, 0,
        0, 1, 0, 3, 1, 0, 1,
        0, 1, 0, 1, 4, 0, 4,
        0, 0, 2, 0, 0, 5, 0,
        0, 1, 0, 1, 4, 0, 6,
    ];

    // sparseTableCells(13) = 13 * (3 + 1) = 52
    expect_eq(sparseTableCells(13), 52 as usize);
    let idx_buf: [52]usize = std.mem.zeroes([52]usize);
    const lca = EulerLca.build(7, euler_nodes[0..], euler_depth[0..], first_occ[0..], idx_buf[0..]) ?? unreachable;

    let u: usize = 0;
    while (u < 7) {
        let v: usize = 0;
        while (v < 7) {
            const ans = lca.query(u, v) ?? unreachable;
            expect_eq(ans, expected[u * 7 + v]);
            v += 1;
        }
        u += 1;
    }

    // Out-of-bounds vertex query rejected
    expect_eq(lca.query(7, 0) == null, true);
    expect_eq(lca.query(0, 9) == null, true);
});

test("5. Boundary sizes: n = 1, power-of-two n = 8, non-power-of-two n = 13 & floorLog2", () => {
    expect_eq(floorLog2(0), 0 as usize);
    expect_eq(floorLog2(1), 0 as usize);
    expect_eq(floorLog2(2), 1 as usize);
    expect_eq(floorLog2(3), 1 as usize);
    expect_eq(floorLog2(4), 2 as usize);
    expect_eq(floorLog2(7), 2 as usize);
    expect_eq(floorLog2(8), 3 as usize);
    expect_eq(floorLog2(13), 3 as usize);
    expect_eq(floorLog2(16), 4 as usize);
    expect_eq(sparseTableCells(0), 0 as usize);
    expect_eq(sparseTableCells(1), 1 as usize);
    expect_eq(sparseTableCells(8), 32 as usize);
    expect_eq(sparseTableCells(13), 52 as usize);

    // n = 1 single-element table
    const one: [1]i64 = [-999];
    let buf1: [1]i64 = std.mem.zeroes([1]i64);
    let ibuf1: [1]usize = std.mem.zeroes([1]usize);
    const rmin1 = RmqMin.build(one[0..], buf1[0..]) ?? unreachable;
    expect_eq(rmin1.query(0, 0) ?? unreachable, -999 as i64);
    const rarg1 = RmqArgMin.build(one[0..], ibuf1[0..]) ?? unreachable;
    expect_eq(rarg1.queryIndex(one[0..], 0, 0) ?? unreachable, 0 as usize);

    // Non-power-of-two n = 13 full interval verification
    const arr13: [13]i64 = [10, -3, 8, -3, 14, 2, -11, 6, 9, -11, 4, 1, 7];
    let min13: [52]i64 = std.mem.zeroes([52]i64);
    let max13: [52]i64 = std.mem.zeroes([52]i64);
    const rm13 = RmqMin.build(arr13[0..], min13[0..]) ?? unreachable;
    const rx13 = RmqMax.build(arr13[0..], max13[0..]) ?? unreachable;
    let l: usize = 0;
    while (l < 13) {
        let r = l;
        while (r < 13) {
            expect_eq(rm13.query(l, r) ?? unreachable, bruteMin(arr13[0..], l, r));
            expect_eq(rx13.query(l, r) ?? unreachable, bruteMax(arr13[0..], l, r));
            r += 1;
        }
        l += 1;
    }
});

test("6. Invalid ranges, empty inputs, k == 0 / k > n & undersized buffer rejection", () => {
    const empty_i64: [0]i64 = [];
    const empty_u64: [0]u64 = [];
    const vals: [5]i64 = [4, 2, 7, 1, 5];
    const uvals: [5]u64 = [12, 18, 24, 6, 30];

    // sparseTableCells(5) = 5 * (2 + 1) = 15
    let small_i64: [14]i64 = std.mem.zeroes([14]i64);
    let ok_i64: [15]i64 = std.mem.zeroes([15]i64);
    let small_usize: [14]usize = std.mem.zeroes([14]usize);
    let ok_usize: [15]usize = std.mem.zeroes([15]usize);
    let small_u64: [14]u64 = std.mem.zeroes([14]u64);
    let ok_u64: [15]u64 = std.mem.zeroes([15]u64);

    // Empty inputs rejected
    expect_eq(RmqMin.build(empty_i64[0..], ok_i64[0..]) == null, true);
    expect_eq(RmqMax.build(empty_i64[0..], ok_i64[0..]) == null, true);
    expect_eq(RmqArgMin.build(empty_i64[0..], ok_usize[0..]) == null, true);
    expect_eq(RangeGcd.build(empty_u64[0..], ok_u64[0..]) == null, true);

    // Undersized table buffers rejected (14 < 15)
    expect_eq(RmqMin.build(vals[0..], small_i64[0..]) == null, true);
    expect_eq(RmqMax.build(vals[0..], small_i64[0..]) == null, true);
    expect_eq(RmqArgMin.build(vals[0..], small_usize[0..]) == null, true);
    expect_eq(RangeGcd.build(uvals[0..], small_u64[0..]) == null, true);

    // Out-of-bounds & inverted intervals rejected
    const rmin = RmqMin.build(vals[0..], ok_i64[0..]) ?? unreachable;
    expect_eq(rmin.query(3, 2) == null, true);
    expect_eq(rmin.query(0, 5) == null, true);
    expect_eq(rmin.query(5, 5) == null, true);

    const rarg = RmqArgMin.build(vals[0..], ok_usize[0..]) ?? unreachable;
    expect_eq(rarg.queryIndex(vals[0..], 4, 1) == null, true);
    expect_eq(rarg.queryIndex(vals[0..], 0, 5) == null, true);
    expect_eq(rarg.queryIndex(vals[0..3], 0, 2) == null, true); // values.len < n

    const rg = RangeGcd.build(uvals[0..], ok_u64[0..]) ?? unreachable;
    expect_eq(rg.query(2, 1) == null, true);
    expect_eq(rg.query(0, 5) == null, true);

    // Sliding window invalid k & undersized out / deque_scratch rejected
    let out2: [2]i64 = std.mem.zeroes([2]i64);
    let out5: [5]i64 = std.mem.zeroes([5]i64);
    let dq4: [4]usize = std.mem.zeroes([4]usize);
    let dq5: [5]usize = std.mem.zeroes([5]usize);

    expect_eq(slidingWindowMin(empty_i64[0..], 1, out5[0..], dq5[0..]) == null, true);
    expect_eq(slidingWindowMin(vals[0..], 0, out5[0..], dq5[0..]) == null, true);
    expect_eq(slidingWindowMin(vals[0..], 6, out5[0..], dq5[0..]) == null, true);
    expect_eq(slidingWindowMin(vals[0..], 3, out2[0..], dq5[0..]) == null, true); // needs 3 outputs
    expect_eq(slidingWindowMin(vals[0..], 3, out5[0..], dq4[0..]) == null, true); // needs 5 deque slots

    expect_eq(slidingWindowMax(vals[0..], 0, out5[0..], dq5[0..]) == null, true);
    expect_eq(slidingWindowMax(vals[0..], 6, out5[0..], dq5[0..]) == null, true);
    expect_eq(slidingWindowMax(vals[0..], 3, out2[0..], dq5[0..]) == null, true);
    expect_eq(slidingWindowMax(vals[0..], 3, out5[0..], dq4[0..]) == null, true);

    // EulerLca invalid first_occ out-of-bounds rejected
    const bad_en: [3]usize = [0, 1, 0];
    const bad_ed: [3]i64 = [0, 1, 0];
    const bad_fo: [2]usize = [0, 5]; // 5 >= 3
    expect_eq(EulerLca.build(2, bad_en[0..], bad_ed[0..], bad_fo[0..], ok_usize[0..]) == null, true);
});
