// cap/test/testCapOnlineStats.cp
// Comprehensive unit tests for @cap/online-stats

import { test, expect_eq } from "cap:test";
import {
    RunningStats,
    linearRegression,
    percentile,
    median,
    iqr,
} from "../../packages/@cap/online-stats/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    const diff = if (a > b) { a - b } else { b - a };
    return diff <= eps;
}

fn main() {
    test("1. @cap/online-stats — Welford mean, sample/pop variance, stddev, min, max, sum", () => {
        let st = RunningStats.init();
        st.push(2.0);
        st.push(4.0);
        st.push(4.0);
        st.push(4.0);
        st.push(5.0);
        st.push(5.0);
        st.push(7.0);
        st.push(9.0);

        expect_eq(st.count(), 8 as u64);
        expect_eq(approxEq(st.mean(), 5.0, 0.000000001), true);
        expect_eq(approxEq(st.sum(), 40.0, 0.000000001), true);
        expect_eq(approxEq(st.min(), 2.0, 0.000000001), true);
        expect_eq(approxEq(st.max(), 9.0, 0.000000001), true);
        expect_eq(approxEq(st.variancePop(), 4.0, 0.000000001), true);
        expect_eq(approxEq(st.stddevPop(), 2.0, 0.000000001), true);
        expect_eq(approxEq(st.varianceSample(), 32.0 / 7.0, 0.000000001), true);
        expect_eq(approxEq(st.stddevSample(), 2.138089935299395, 0.000001), true);
    });

    test("2. @cap/online-stats — Numerical stability under large offset (1e9 + {4, 7, 13, 16})", () => {
        let st = RunningStats.init();
        st.push(1000000004.0);
        st.push(1000000007.0);
        st.push(1000000013.0);
        st.push(1000000016.0);

        expect_eq(approxEq(st.mean(), 1000000010.0, 0.000001), true);
        expect_eq(approxEq(st.varianceSample(), 30.0, 0.000001), true);
        expect_eq(approxEq(st.variancePop(), 22.5, 0.000001), true);
    });

    test("3. @cap/online-stats — Chan parallel merge matches single-pass stats", () => {
        let shardA = RunningStats.init();
        shardA.push(10.0);
        shardA.push(20.0);
        shardA.push(30.0);

        let shardB = RunningStats.init();
        shardB.push(40.0);
        shardB.push(80.0);
        shardB.push(120.0);

        let full = RunningStats.init();
        full.push(10.0);
        full.push(20.0);
        full.push(30.0);
        full.push(40.0);
        full.push(80.0);
        full.push(120.0);

        shardA.merge(shardB);
        expect_eq(shardA.count(), full.count());
        expect_eq(approxEq(shardA.mean(), full.mean(), 0.000000001), true);
        expect_eq(approxEq(shardA.varianceSample(), full.varianceSample(), 0.000000001), true);
        expect_eq(approxEq(shardA.min(), full.min(), 0.000000001), true);
        expect_eq(approxEq(shardA.max(), full.max(), 0.000000001), true);
        expect_eq(approxEq(shardA.sum(), full.sum(), 0.000000001), true);
    });

    test("4. @cap/online-stats — OLS linearRegression & Pearson R²", () => {
        // Perfect linear relation y = 2x + 3
        const xs: [5]f64 = [1.0, 2.0, 3.0, 4.0, 5.0];
        const ys: [5]f64 = [5.0, 7.0, 9.0, 11.0, 13.0];
        const fit = linearRegression(xs[0..5], ys[0..5]);
        expect_eq(fit.valid, true);
        expect_eq(approxEq(fit.slope, 2.0, 0.000000001), true);
        expect_eq(approxEq(fit.intercept, 3.0, 0.000000001), true);
        expect_eq(approxEq(fit.pearson_r, 1.0, 0.000000001), true);
        expect_eq(approxEq(fit.r_squared, 1.0, 0.000000001), true);

        // Negative correlation y = -3x + 10
        const ysNeg: [5]f64 = [7.0, 4.0, 1.0, -2.0, -5.0];
        const fitNeg = linearRegression(xs[0..5], ysNeg[0..5]);
        expect_eq(fitNeg.valid, true);
        expect_eq(approxEq(fitNeg.slope, -3.0, 0.000000001), true);
        expect_eq(approxEq(fitNeg.pearson_r, -1.0, 0.000000001), true);
        expect_eq(approxEq(fitNeg.r_squared, 1.0, 0.000000001), true);
    });

    test("5. @cap/online-stats — Percentiles (P50, P90, P95, P99), median & IQR", () => {
        // Unsorted 11 values: 0..100 step 10
        const vals: [11]f64 = [80.0, 10.0, 100.0, 0.0, 50.0, 30.0, 90.0, 20.0, 70.0, 40.0, 60.0];
        expect_eq(approxEq(median(vals[0..11]) ?? -1.0, 50.0, 0.000000001), true);
        expect_eq(approxEq(percentile(vals[0..11], 90.0) ?? -1.0, 90.0, 0.000000001), true);
        expect_eq(approxEq(percentile(vals[0..11], 95.0) ?? -1.0, 95.0, 0.000000001), true);
        expect_eq(approxEq(percentile(vals[0..11], 99.0) ?? -1.0, 99.0, 0.000000001), true);
        expect_eq(approxEq(iqr(vals[0..11]) ?? -1.0, 50.0, 0.000000001), true);
        // Ensure original input slice was NOT mutated
        expect_eq(approxEq(vals[0], 80.0, 0.000000001), true);
    });

    test("6. @cap/online-stats — Boundary guards for empty/invalid inputs", () => {
        let empty: [0]f64 = undefined;
        expect_eq(percentile(empty[0..0], 50.0) == null, true);
        expect_eq(median(empty[0..0]) == null, true);
        expect_eq(iqr(empty[0..0]) == null, true);

        const single: [1]f64 = [42.0];
        expect_eq(percentile(single[0..1], -1.0) == null, true);
        expect_eq(percentile(single[0..1], 101.0) == null, true);
        expect_eq(approxEq(percentile(single[0..1], 99.0) ?? 0.0, 42.0, 0.000000001), true);

        // Constant X has zero variance -> invalid linear regression
        const constX: [3]f64 = [5.0, 5.0, 5.0];
        const someY: [3]f64 = [1.0, 2.0, 3.0];
        expect_eq(linearRegression(constX[0..3], someY[0..3]).valid, false);
    });
}
