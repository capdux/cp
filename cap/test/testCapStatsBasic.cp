// cap/test/testCapStatsBasic.cp
// Comprehensive unit tests for @cap/stats-basic (Welford, Chan merge, Pearson, Percentiles)

import { test, expect_eq } from "cap:test";
import {
    OnlineStats,
    sum,
    mean,
    variance,
    stddev,
    minSlice,
    maxSlice,
    covariance,
    pearsonR,
    percentileSorted,
    medianSorted,
} from "../../packages/@cap/stats-basic/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff: f64 = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/stats-basic — Welford OnlineStats mean, variance & stddev", () => {
        // Classic dataset: [2, 4, 4, 4, 5, 5, 7, 9] -> mean=5, pop var=4, pop sd=2
        const data: [8]f64 = [2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0];
        let st: OnlineStats = OnlineStats.new();
        let i: usize = 0;
        while (i < 8) {
            st.push(data[i]);
            i += 1;
        }
        expect_eq(st.count, 8 as u64);
        expect_eq(approxEq(st.mean, 5.0, 0.000000001), true);
        expect_eq(approxEq(st.variance(), 4.0, 0.000000001), true);
        expect_eq(approxEq(st.stddev(), 2.0, 0.000000001), true);
        expect_eq(approxEq(st.sampleVariance(), 32.0 / 7.0, 0.000000001), true);
        expect_eq(approxEq(st.min_val, 2.0, 0.000000001), true);
        expect_eq(approxEq(st.max_val, 9.0, 0.000000001), true);
    });

    test("2. @cap/stats-basic — Chan parallel merge equals single-pass Welford", () => {
        const data: [8]f64 = [10.0, 20.0, 15.0, 30.0, 25.0, 40.0, 35.0, 50.0];
        let full: OnlineStats = OnlineStats.new();
        let part_a: OnlineStats = OnlineStats.new();
        let part_b: OnlineStats = OnlineStats.new();

        let i: usize = 0;
        while (i < 8) {
            full.push(data[i]);
            if (i < 3) {
                part_a.push(data[i]);
            } else {
                part_b.push(data[i]);
            }
            i += 1;
        }
        part_a.merge(part_b);

        expect_eq(part_a.count, full.count);
        expect_eq(approxEq(part_a.mean, full.mean, 0.000000001), true);
        expect_eq(approxEq(part_a.variance(), full.variance(), 0.000000001), true);
        expect_eq(approxEq(part_a.sampleStddev(), full.sampleStddev(), 0.000000001), true);
        expect_eq(approxEq(part_a.min_val, full.min_val, 0.000000001), true);
        expect_eq(approxEq(part_a.max_val, full.max_val, 0.000000001), true);
    });

    test("3. @cap/stats-basic — slice sum, mean, variance, stddev, min & max", () => {
        const xs: [8]f64 = [2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0];
        expect_eq(approxEq(sum(xs[0..8]), 40.0, 0.000000001), true);
        expect_eq(approxEq(mean(xs[0..8]), 5.0, 0.000000001), true);
        expect_eq(approxEq(variance(xs[0..8]), 4.0, 0.000000001), true);
        expect_eq(approxEq(stddev(xs[0..8]), 2.0, 0.000000001), true);
        expect_eq(approxEq(minSlice(xs[0..8]), 2.0, 0.000000001), true);
        expect_eq(approxEq(maxSlice(xs[0..8]), 9.0, 0.000000001), true);
    });

    test("4. @cap/stats-basic — covariance & Pearson correlation positive/negative", () => {
        const xs: [5]f64 = [1.0, 2.0, 3.0, 4.0, 5.0];
        const ys_pos: [5]f64 = [2.0, 4.0, 6.0, 8.0, 10.0];
        const ys_neg: [5]f64 = [10.0, 8.0, 6.0, 4.0, 2.0];

        expect_eq(approxEq(covariance(xs[0..5], ys_pos[0..5]), 4.0, 0.000000001), true);
        expect_eq(approxEq(pearsonR(xs[0..5], ys_pos[0..5]), 1.0, 0.000000001), true);
        expect_eq(approxEq(pearsonR(xs[0..5], ys_neg[0..5]), -1.0, 0.000000001), true);
    });

    test("5. @cap/stats-basic — medianSorted & percentileSorted (P50, P95, P0, P100)", () => {
        const s_even: [4]f64 = [10.0, 20.0, 30.0, 40.0];
        expect_eq(approxEq(medianSorted(s_even[0..4]), 25.0, 0.000000001), true);
        expect_eq(approxEq(percentileSorted(s_even[0..4], 0.0), 10.0, 0.000000001), true);
        expect_eq(approxEq(percentileSorted(s_even[0..4], 100.0), 40.0, 0.000000001), true);

        const s_21: [5]f64 = [0.0, 25.0, 50.0, 75.0, 100.0];
        expect_eq(approxEq(medianSorted(s_21[0..5]), 50.0, 0.000000001), true);
        expect_eq(approxEq(percentileSorted(s_21[0..5], 95.0), 95.0, 0.000000001), true);
    });

    test("6. @cap/stats-basic — empty & single-element edge cases", () => {
        const one: [1]f64 = [42.0];
        const empty: []const f64 = one[0..0];

        expect_eq(approxEq(sum(empty), 0.0, 0.000000001), true);
        expect_eq(approxEq(mean(empty), 0.0, 0.000000001), true);
        expect_eq(approxEq(variance(empty), 0.0, 0.000000001), true);
        expect_eq(approxEq(medianSorted(empty), 0.0, 0.000000001), true);
        expect_eq(approxEq(pearsonR(empty, empty), 0.0, 0.000000001), true);

        let st: OnlineStats = OnlineStats.new();
        expect_eq(approxEq(st.variance(), 0.0, 0.000000001), true);
        expect_eq(approxEq(st.sampleVariance(), 0.0, 0.000000001), true);
        st.push(42.0);
        expect_eq(approxEq(st.mean, 42.0, 0.000000001), true);
        expect_eq(approxEq(st.sampleVariance(), 0.0, 0.000000001), true);
        expect_eq(approxEq(medianSorted(one[0..1]), 42.0, 0.000000001), true);
    });
}
