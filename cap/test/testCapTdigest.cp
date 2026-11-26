// cap/test/testCapTdigest.cp
// Comprehensive unit tests for @cap/tdigest

import { test, expect_eq } from "cap:test";
import {
    MAX_CENTROIDS,
    Centroid,
    TDigest,
} from "../../packages/@cap/tdigest/src/index.cp";

fn absDiff(a: f64, b: f64) -> f64 {
    return if (a > b) a - b else b - a;
}

fn main() {
    test("1. @cap/tdigest — Empty & single-element boundary cases", () => {
        let td = TDigest.init(50.0);
        expect_eq(td.isEmpty(), true);
        expect_eq(td.count(), 0.0);
        expect_eq(td.quantile(0.5), 0.0);
        expect_eq(td.cdf(10.0), 0.0);

        td.add(42.5);
        expect_eq(td.isEmpty(), false);
        expect_eq(td.count(), 1.0);
        expect_eq(td.min(), 42.5);
        expect_eq(td.max(), 42.5);
        expect_eq(td.quantile(0.0), 42.5);
        expect_eq(td.quantile(0.5), 42.5);
        expect_eq(td.quantile(1.0), 42.5);
    });

    test("2. @cap/tdigest — Uniform 1..1000 high-accuracy P50, P90 & P99", () => {
        let td = TDigest.init(50.0);
        let i: usize = 1;
        while (i <= 1000) {
            td.add(i as f64);
            i += 1;
        }
        expect_eq(td.count(), 1000.0);
        expect_eq(td.min(), 1.0);
        expect_eq(td.max(), 1000.0);
        expect_eq(td.centroidCount() <= MAX_CENTROIDS, true);

        const p50 = td.quantile(0.50);
        const p90 = td.quantile(0.90);
        const p99 = td.quantile(0.99);

        expect_eq(absDiff(p50, 500.5) < 15.0, true);
        expect_eq(absDiff(p90, 900.5) < 12.0, true);
        expect_eq(absDiff(p99, 990.5) < 5.0, true);
    });

    test("3. @cap/tdigest — Heavy-tail latency distribution P99 & P99.9 accuracy", () => {
        let td = TDigest.init(50.0);
        // 990 fast requests distributed smoothly in 10.0..11.0ms, 9 slow at 500ms, 1 outlier at 5000ms
        let i: usize = 0;
        while (i < 990) {
            td.add(10.0 + ((i as f64) / 990.0));
            i += 1;
        }
        i = 0;
        while (i < 9) {
            td.add(500.0);
            i += 1;
        }
        td.add(5000.0);

        expect_eq(td.quantile(0.50) >= 10.0 && td.quantile(0.50) <= 11.0, true);
        expect_eq(td.quantile(0.95) >= 10.5 && td.quantile(0.95) <= 12.0, true);
        // P99 sits right at the transition to 500ms, P99.9 approaches 5000ms
        const p99 = td.quantile(0.99);
        const p999 = td.quantile(0.999);
        expect_eq(p99 > 11.0 && p99 <= 510.0, true);
        expect_eq(p999 > 500.0, true);
        expect_eq(td.quantile(1.0), 5000.0);
    });

    test("4. @cap/tdigest — CDF monotonicity & quantile inversion consistency", () => {
        let td = TDigest.init(50.0);
        let i: usize = 1;
        while (i <= 500) {
            td.add(i as f64);
            i += 1;
        }

        expect_eq(td.cdf(0.0), 0.0);
        expect_eq(td.cdf(500.0), 1.0);
        expect_eq(td.cdf(999.0), 1.0);

        const c1 = td.cdf(100.0);
        const c2 = td.cdf(250.0);
        const c3 = td.cdf(400.0);
        expect_eq(c1 < c2 && c2 < c3, true);
        expect_eq(absDiff(c2, 0.5) < 0.05, true);

        // Inversion: cdf(quantile(0.75)) ~= 0.75
        const q75 = td.quantile(0.75);
        expect_eq(absDiff(td.cdf(q75), 0.75) < 0.05, true);
    });

    test("5. @cap/tdigest — Distributed merge of two independent nodes matches full stream", () => {
        let node_a = TDigest.init(50.0);
        let node_b = TDigest.init(50.0);

        // Node A collects odd numbers 1, 3, ..., 999; Node B collects even 2, 4, ..., 1000
        let i: usize = 1;
        while (i <= 1000) {
            if ((i % 2) == 1) {
                node_a.add(i as f64);
            } else {
                node_b.add(i as f64);
            }
            i += 1;
        }

        node_a.merge(&node_b);
        expect_eq(node_a.count(), 1000.0);
        expect_eq(node_a.min(), 1.0);
        expect_eq(node_a.max(), 1000.0);
        expect_eq(absDiff(node_a.quantile(0.50), 500.5) < 20.0, true);
        expect_eq(absDiff(node_a.quantile(0.99), 990.5) < 10.0, true);
    });

    test("6. @cap/tdigest — addWeighted (ignoring <=0 weight) & reset()", () => {
        let td = TDigest.init(50.0);
        td.addWeighted(10.0, 0.0);
        td.addWeighted(20.0, -5.0);
        expect_eq(td.isEmpty(), true);

        td.addWeighted(100.0, 75.0);
        td.addWeighted(200.0, 25.0);
        expect_eq(td.count(), 100.0);
        expect_eq(td.quantile(0.25), 100.0);
        expect_eq(td.quantile(0.95), 200.0);

        td.reset();
        expect_eq(td.isEmpty(), true);
        expect_eq(td.count(), 0.0);
        expect_eq(td.centroidCount(), 0 as usize);
    });
}
