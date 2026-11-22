// Origin: Pure-CP Stdlib Test Suite (lib/ds/fenwick_order.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Fenwick BIT, O(log N) Order-Stat & Weighted Sampler:
//   1. O(N) linear build_from_slice vs naive prefix/range sum 100% parity
//   2. Signed add / set / point_value / range_sum / total_sum dynamic mutation parity
//   3. FenwickOrderStat O(log N) find_lower_bound dynamic K-th smallest (including zero-weight holes)
//   4. sub_weight dynamic K-th shift & strict underflow rejection (zero state corruption)
//   5. sample_weighted_index large-sample empirical distribution converges to exact target weight ratios
//   6. Full FENWICK_MAX_LEN (1024) capacity stress test (all 1024 slots, prefix & Binary Lifting boundaries)
//   7. Boundary guards: len 0 / >1024, out-of-bounds idx, lo > hi, total_weight == 0 & target out of range

import { test, expect_eq } from "cap:test";
import {
    FENWICK_MAX_LEN,
    FenwickStats,
    FenwickTreeI64,
    FenwickOrderStat,
} from "cap:ds";

fn splitMix64Next(state: *u64): u64 {
    state.* = state.* +% 0x9e3779b97f4a7c15;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

// ============================================================================
// Test 1: O(N) Linear Build vs Naive Prefix & Range Sums (100% Parity)
// ============================================================================
test("fenwick_order: O(N) build_from_slice matches naive prefix & range sums 100%", () => {
    expect_eq(FENWICK_MAX_LEN, 1024);

    const vals: [10]i64 = [5, -2, 13, 0, -7, 8, 4, -1, 9, -6];
    let ft = FenwickTreeI64.build_from_slice(vals[0..]) ?? return;

    // Verify every prefix_sum and point_value against naive reference
    let running: i64 = 0;
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        running += vals[i];
        expect_eq(ft.prefix_sum(i), running);
        expect_eq(ft.point_value(i), vals[i]);
    }
    expect_eq(ft.total_sum(), running);

    // Verify all O(N^2) range_sum(lo, hi) pairs
    let lo: usize = 0;
    while (lo < 10) : (lo += 1) {
        let rsum: i64 = 0;
        let hi: usize = lo;
        while (hi < 10) : (hi += 1) {
            rsum += vals[hi];
            expect_eq(ft.range_sum(lo, hi), rsum);
        }
    }
});

// ============================================================================
// Test 2: Dynamic Signed add / set / point_value / range_sum Consistency
// ============================================================================
test("fenwick_order: dynamic signed add, set & range_sum consistency", () => {
    let ft = FenwickTreeI64.init(8) ?? return;
    let ref_arr: [8]i64 = [0; 8];

    expect_eq(ft.add(0, 100), true);
    ref_arr[0] += 100;
    expect_eq(ft.add(3, -45), true);
    ref_arr[3] -= 45;
    expect_eq(ft.add(7, 25), true);
    ref_arr[7] += 25;
    expect_eq(ft.set(3, 15), true); // -45 -> +15 (delta +60)
    ref_arr[3] = 15;
    expect_eq(ft.set(0, -10), true); // 100 -> -10
    ref_arr[0] = -10;

    let i: usize = 0;
    while (i < 8) : (i += 1) {
        expect_eq(ft.point_value(i), ref_arr[i]);
    }
    expect_eq(ft.range_sum(0, 7), -10 + 15 + 25);
    expect_eq(ft.range_sum(1, 6), 15);
    expect_eq(ft.stats().updates_count, 5);
    expect_eq(ft.stats().total_sum, 30);
});

// ============================================================================
// Test 3: FenwickOrderStat O(log N) find_lower_bound (Zero-Weight Holes)
// ============================================================================
test("fenwick_order: O(log N) find_lower_bound skips zero-weight holes accurately", () => {
    let os = FenwickOrderStat.init(8) ?? return;
    // Weights: [0, 0, 5, 0, 0, 3, 0, 2] -> total = 10
    expect_eq(os.add_weight(2, 5), true);
    expect_eq(os.add_weight(5, 3), true);
    expect_eq(os.add_weight(7, 2), true);

    expect_eq(os.total_weight(), 10);
    expect_eq(os.prefix_weight(1), 0);
    expect_eq(os.prefix_weight(2), 5);
    expect_eq(os.prefix_weight(6), 8);
    expect_eq(os.prefix_weight(7), 10);

    // 1..5 -> idx 2 (skipping zero-weight holes 0, 1)
    expect_eq(os.find_lower_bound(1) ?? 99, 2);
    expect_eq(os.find_lower_bound(5) ?? 99, 2);
    // 6..8 -> idx 5 (skipping zero-weight holes 3, 4)
    expect_eq(os.find_lower_bound(6) ?? 99, 5);
    expect_eq(os.find_lower_bound(8) ?? 99, 5);
    // 9..10 -> idx 7 (skipping zero-weight hole 6)
    expect_eq(os.find_lower_bound(9) ?? 99, 7);
    expect_eq(os.find_lower_bound(10) ?? 99, 7);

    // Out-of-range targets return null
    expect_eq(os.find_lower_bound(0) == null, true);
    expect_eq(os.find_lower_bound(11) == null, true);
});

// ============================================================================
// Test 4: sub_weight Dynamic K-th Migration & Underflow Rejection
// ============================================================================
test("fenwick_order: sub_weight shifts K-th order stat & rejects underflow cleanly", () => {
    let os = FenwickOrderStat.init(5) ?? return;
    expect_eq(os.add_weight(0, 4), true);
    expect_eq(os.add_weight(2, 6), true);
    expect_eq(os.add_weight(4, 5), true);
    // 5th smallest is at idx 2
    expect_eq(os.find_lower_bound(5) ?? 99, 2);

    // Attempt underflow on idx 0 (4 - 5) -> rejected without corrupting tree
    expect_eq(os.sub_weight(0, 5), false);
    expect_eq(os.total_weight(), 15);
    expect_eq(os.prefix_weight(0), 4);

    // Valid subtraction: remove all 6 from idx 2 -> idx 2 becomes a 0-weight hole
    expect_eq(os.sub_weight(2, 6), true);
    expect_eq(os.total_weight(), 9);
    // Now 5th smallest migrates directly to idx 4!
    expect_eq(os.find_lower_bound(5) ?? 99, 4);

    const st: FenwickStats = os.stats();
    expect_eq(st.len, 5);
    expect_eq(st.total_sum, 9);
    expect_eq(st.updates_count, 4);
});

// ============================================================================
// Test 5: sample_weighted_index Large-Sample Empirical Distribution Accuracy
// ============================================================================
test("fenwick_order: sample_weighted_index converges accurately to target weights", () => {
    let os = FenwickOrderStat.init(4) ?? return;
    // Target ratios: idx 0 = 10%, idx 1 = 20%, idx 2 = 0%, idx 3 = 70%
    expect_eq(os.add_weight(0, 10), true);
    expect_eq(os.add_weight(1, 20), true);
    expect_eq(os.add_weight(3, 70), true);

    let counts: [4]usize = [0; 4];
    let rng: u64 = 0xCAFEBABE12345678;
    let trials: usize = 0;
    while (trials < 10000) : (trials += 1) {
        const r: u64 = splitMix64Next(&rng);
        const idx: usize = os.sample_weighted_index(r) ?? return;
        counts[idx] += 1;
    }

    // Zero-weight slot 2 must NEVER be sampled
    expect_eq(counts[2], 0);
    // Empirical counts out of 10000 must stay tightly around 1000, 2000, 7000 (+/- 250)
    expect_eq(counts[0] >= 850 && counts[0] <= 1150, true);
    expect_eq(counts[1] >= 1800 && counts[1] <= 2200, true);
    expect_eq(counts[3] >= 6750 && counts[3] <= 7250, true);
});

// ============================================================================
// Test 6: Full FENWICK_MAX_LEN (1024) Capacity & Binary Lifting Stress Test
// ============================================================================
test("fenwick_order: full 1024-slot capacity prefix & Binary Lifting exactness", () => {
    let os = FenwickOrderStat.init(FENWICK_MAX_LEN) ?? return;
    let i: usize = 0;
    while (i < FENWICK_MAX_LEN) : (i += 1) {
        expect_eq(os.add_weight(i, 3), true);
    }
    expect_eq(os.total_weight(), 3072);
    expect_eq(os.prefix_weight(1023), 3072);

    // First, middle, and last elements via O(log N) Binary Lifting
    expect_eq(os.find_lower_bound(1) ?? 9999, 0);
    expect_eq(os.find_lower_bound(3) ?? 9999, 0);
    expect_eq(os.find_lower_bound(4) ?? 9999, 1);
    expect_eq(os.find_lower_bound(1536) ?? 9999, 511);
    expect_eq(os.find_lower_bound(1537) ?? 9999, 512);
    expect_eq(os.find_lower_bound(3070) ?? 9999, 1023);
    expect_eq(os.find_lower_bound(3072) ?? 9999, 1023);
});

// ============================================================================
// Test 7: Boundary Guards (Invalid Length, Out-of-Bounds, lo > hi & Empty Weight)
// ============================================================================
test("fenwick_order: boundary guards for init, bounds, lo > hi & empty sampler", () => {
    // Invalid init lengths
    expect_eq(FenwickTreeI64.init(0) == null, true);
    expect_eq(FenwickTreeI64.init(FENWICK_MAX_LEN + 1) == null, true);
    expect_eq(FenwickOrderStat.init(0) == null, true);
    expect_eq(FenwickOrderStat.init(FENWICK_MAX_LEN + 1) == null, true);

    const dummy: [1]i64 = [1];
    expect_eq(FenwickTreeI64.build_from_slice(dummy[0..0]) == null, true);

    let ft = FenwickTreeI64.init(4) ?? return;
    expect_eq(ft.add(4, 10), false);
    expect_eq(ft.set(4, 10), false);
    expect_eq(ft.point_value(4), 0);
    expect_eq(ft.range_sum(3, 1), 0);  // lo > hi
    expect_eq(ft.range_sum(4, 5), 0);  // lo >= len

    // Empty FenwickOrderStat sampler returns null safely
    let os = FenwickOrderStat.init(4) ?? return;
    expect_eq(os.add_weight(4, 1), false);
    expect_eq(os.sub_weight(4, 1), false);
    expect_eq(os.sample_weighted_index(42) == null, true);
});
