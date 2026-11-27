import { test, expect_eq } from "cap:test";
import {
    computeStats,
    speedupRatio,
} from "../../packages/tinybench/src/index.cp";

fn approx(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return d >= -eps && d <= eps;
}

fn main() {
    test("1. @cap/tinybench — computes mean, min, max, and ops/sec accurately", () => {
        const samples: [5]f64 = [1000.0, 2000.0, 3000.0, 4000.0, 5000.0];
        const st = computeStats(samples[0..5]);
        expect_eq(st.count, 5 as usize);
        expect_eq(approx(st.mean_ns, 3000.0, 1.0e-6), true);
        expect_eq(approx(st.min_ns, 1000.0, 1.0e-6), true);
        expect_eq(approx(st.max_ns, 5000.0, 1.0e-6), true);
        expect_eq(approx(st.ops_per_sec, 1000000000.0 / 3000.0, 1.0), true);
    });

    test("2. @cap/tinybench — computes percentiles on unsorted sample input", () => {
        const samples: [5]f64 = [500.0, 100.0, 400.0, 200.0, 300.0];
        const st = computeStats(samples[0..5]);
        expect_eq(approx(st.p50_ns, 300.0, 1.0e-6), true);
        expect_eq(approx(st.p75_ns, 400.0, 1.0e-6), true);
        expect_eq(approx(st.p99_ns, 496.0, 1.0e-6), true);
    });

    test("3. @cap/tinybench — computes sample variance, stddev, and RME", () => {
        const samples: [4]f64 = [10.0, 10.0, 10.0, 10.0];
        const st = computeStats(samples[0..4]);
        expect_eq(approx(st.variance, 0.0, 1.0e-9), true);
        expect_eq(approx(st.stddev_ns, 0.0, 1.0e-9), true);
        expect_eq(approx(st.rme_percent, 0.0, 1.0e-9), true);
    });

    test("4. @cap/tinybench — non-zero variance and positive RME on spread samples", () => {
        const samples: [4]f64 = [100.0, 200.0, 300.0, 400.0];
        const st = computeStats(samples[0..4]);
        expect_eq(st.stddev_ns > 100.0, true);
        expect_eq(st.rme_percent > 0.0, true);
    });

    test("5. @cap/tinybench — speedupRatio compares baseline vs faster candidate", () => {
        const base_s: [3]f64 = [1000.0, 1000.0, 1000.0];
        const fast_s: [3]f64 = [250.0, 250.0, 250.0];
        const ratio = speedupRatio(computeStats(base_s[0..3]), computeStats(fast_s[0..3]));
        expect_eq(approx(ratio, 4.0, 1.0e-6), true);
    });

    test("6. @cap/tinybench — handles empty and single-sample slices safely", () => {
        const one: [1]f64 = [500.0];
        const st0 = computeStats(one[0..0]);
        expect_eq(st0.count, 0 as usize);
        expect_eq(st0.ops_per_sec, 0.0);

        const st1 = computeStats(one[0..1]);
        expect_eq(st1.count, 1 as usize);
        expect_eq(approx(st1.p50_ns, 500.0, 1.0e-6), true);
        expect_eq(approx(st1.stddev_ns, 0.0, 1.0e-9), true);
    });
}
