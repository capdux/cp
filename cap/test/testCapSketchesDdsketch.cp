import { test, expect_eq } from "cap:test";
import {
    DDSK_BUCKETS,
    DDSketch,
} from "../../packages/sketches-ddsketch/src/index.cp";

fn absDiff(a: f64, b: f64): f64 {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

fn main() {
    test("1. @cap/sketches-ddsketch — empty sketch reports zero count and zero quantiles", () => {
        let sk = DDSketch.new(0.01);
        expect_eq(DDSK_BUCKETS, 512 as usize);
        expect_eq(sk.count(), 0 as u64);
        expect_eq(sk.sum(), 0.0);
        expect_eq(sk.min(), 0.0);
        expect_eq(sk.max(), 0.0);
        expect_eq(sk.quantile(0.5), 0.0);
    });

    test("2. @cap/sketches-ddsketch — tracks exact count, sum, min, and max", () => {
        let sk = DDSketch.new(0.01);
        sk.add(10.0);
        sk.add(20.0);
        sk.add(30.0);
        sk.add(40.0);
        expect_eq(sk.count(), 4 as u64);
        expect_eq(sk.sum(), 100.0);
        expect_eq(sk.min(), 10.0);
        expect_eq(sk.max(), 40.0);
        expect_eq(sk.quantile(0.0), 10.0);
        expect_eq(sk.quantile(1.0), 40.0);
    });

    test("3. @cap/sketches-ddsketch — p50, p95, p99 within relative accuracy on 1..1000", () => {
        let sk = DDSketch.new(0.01);
        let i: i32 = 1;
        while (i <= 1000) : (i += 1) {
            sk.add(@as(f64, @floatFromInt(i)));
        }
        expect_eq(sk.count(), 1000 as u64);
        const p50 = sk.quantile(0.50);
        const p95 = sk.quantile(0.95);
        const p99 = sk.quantile(0.99);
        expect_eq(absDiff(p50, 500.0) <= 500.0 * 0.015, true);
        expect_eq(absDiff(p95, 950.0) <= 950.0 * 0.015, true);
        expect_eq(absDiff(p99, 990.0) <= 990.0 * 0.015, true);
    });

    test("4. @cap/sketches-ddsketch — handles zero samples alongside positive latencies", () => {
        let sk = DDSketch.new(0.01);
        let i: i32 = 0;
        while (i < 50) : (i += 1) {
            sk.add(0.0);
        }
        let j: i32 = 1;
        while (j <= 50) : (j += 1) {
            sk.add(100.0);
        }
        expect_eq(sk.count(), 100 as u64);
        expect_eq(sk.quantile(0.40), 0.0);
        const p80 = sk.quantile(0.80);
        expect_eq(absDiff(p80, 100.0) <= 2.0, true);
    });

    test("5. @cap/sketches-ddsketch — merge combines two independent sketches accurately", () => {
        let a = DDSketch.new(0.01);
        let b = DDSketch.new(0.01);
        let i: i32 = 1;
        while (i <= 500) : (i += 1) {
            a.add(@as(f64, @floatFromInt(i)));
        }
        let j: i32 = 501;
        while (j <= 1000) : (j += 1) {
            b.add(@as(f64, @floatFromInt(j)));
        }
        a.merge(b);
        expect_eq(a.count(), 1000 as u64);
        expect_eq(a.min(), 1.0);
        expect_eq(a.max(), 1000.0);
        const p50 = a.quantile(0.50);
        const p99 = a.quantile(0.99);
        expect_eq(absDiff(p50, 500.0) <= 500.0 * 0.015, true);
        expect_eq(absDiff(p99, 990.0) <= 990.0 * 0.015, true);
    });

    test("6. @cap/sketches-ddsketch — collapsing lowest buckets preserves high quantiles across wide range", () => {
        let sk = DDSketch.new(0.01);
        sk.add(0.001);
        let i: i32 = 1;
        while (i <= 100) : (i += 1) {
            sk.add(100000.0);
        }
        expect_eq(sk.count(), 101 as u64);
        const p95 = sk.quantile(0.95);
        expect_eq(absDiff(p95, 100000.0) <= 100000.0 * 0.015, true);
    });
}
