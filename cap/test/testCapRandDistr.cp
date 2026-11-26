import { test, expect_eq } from "cap:test";
import { Xoshiro256, seedFromU64 } from "../../packages/@cap/xoshiro256/src/index.cp";
import {
    UniformRange,
    NormalDistr,
    ExpDistr,
    BernoulliDistr,
    WeightedIndex
} from "../../packages/@cap/rand-distr/src/index.cp";

fn main() {
    test("1. @cap/rand-distr — UniformRange bounds & empirical mean", () => {
        let rng: Xoshiro256 = seedFromU64(42);
        let maybe = UniformRange.new(-5.0, 15.0);
        expect_eq(maybe == null, false);
        let u: UniformRange = maybe ?? UniformRange { low: 0.0, high: 1.0 };

        let sum: f64 = 0.0;
        let n: u32 = 2000;
        let i: u32 = 0;
        let in_bounds: bool = true;
        while (i < n) {
            let v: f64 = u.sample(&rng);
            if (v < -5.0 || v >= 15.0) { in_bounds = false; }
            sum = sum + v;
            i = i + 1;
        }
        expect_eq(in_bounds, true);
        let avg: f64 = sum / (n as f64);
        expect_eq(avg > 4.0 && avg < 6.0, true);
    });

    test("2. @cap/rand-distr — NormalDistr Box-Muller mean & std_dev convergence", () => {
        let rng: Xoshiro256 = seedFromU64(1337);
        let maybe = NormalDistr.new(10.0, 2.5);
        expect_eq(maybe == null, false);
        let nd: NormalDistr = maybe ?? NormalDistr { mean: 0.0, std_dev: 1.0 };

        let n: u32 = 4000;
        let sum: f64 = 0.0;
        let sum_sq: f64 = 0.0;
        let i: u32 = 0;
        while (i < n) {
            let v: f64 = nd.sample(&rng);
            sum = sum + v;
            sum_sq = sum_sq + v * v;
            i = i + 1;
        }
        let avg: f64 = sum / (n as f64);
        let var_est: f64 = (sum_sq / (n as f64)) - avg * avg;
        expect_eq(avg > 9.7 && avg < 10.3, true);
        expect_eq(var_est > 5.5 && var_est < 7.0, true);

        // Degenerate zero std_dev returns exact mean
        let zero_sd: NormalDistr = NormalDistr.new(7.5, 0.0) ?? NormalDistr { mean: 0.0, std_dev: 0.0 };
        expect_eq(zero_sd.sample(&rng), 7.5);
    });

    test("3. @cap/rand-distr — ExpDistr inverse transform non-negative & mean == 1/lambda", () => {
        let rng: Xoshiro256 = seedFromU64(2026);
        let maybe = ExpDistr.new(0.5);
        expect_eq(maybe == null, false);
        let ed: ExpDistr = maybe ?? ExpDistr { lambda: 1.0 };

        let n: u32 = 4000;
        let sum: f64 = 0.0;
        let all_nonneg: bool = true;
        let i: u32 = 0;
        while (i < n) {
            let v: f64 = ed.sample(&rng);
            if (v < 0.0) { all_nonneg = false; }
            sum = sum + v;
            i = i + 1;
        }
        expect_eq(all_nonneg, true);
        let avg: f64 = sum / (n as f64);
        // Expected mean = 1 / 0.5 = 2.0
        expect_eq(avg > 1.8 && avg < 2.2, true);
    });

    test("4. @cap/rand-distr — BernoulliDistr probability convergence & boundary p=0/p=1", () => {
        let rng: Xoshiro256 = seedFromU64(999);
        let b0: BernoulliDistr = BernoulliDistr.new(0.0) ?? BernoulliDistr { p: 0.0 };
        let b1: BernoulliDistr = BernoulliDistr.new(1.0) ?? BernoulliDistr { p: 1.0 };
        expect_eq(b0.sample(&rng), false);
        expect_eq(b1.sample(&rng), true);

        let b30: BernoulliDistr = BernoulliDistr.new(0.3) ?? BernoulliDistr { p: 0.3 };
        let hits: u32 = 0;
        let n: u32 = 4000;
        let i: u32 = 0;
        while (i < n) {
            if (b30.sample(&rng)) { hits = hits + 1; }
            i = i + 1;
        }
        let rate: f64 = (hits as f64) / (n as f64);
        expect_eq(rate > 0.27 && rate < 0.33, true);
    });

    test("5. @cap/rand-distr — WeightedIndex zero-heap discrete sampling proportions", () => {
        let rng: Xoshiro256 = seedFromU64(7777);
        let ws: [4]f64 = [1.0, 2.0, 0.0, 7.0]; // expected: 10%, 20%, 0%, 70%
        let maybe = WeightedIndex.fromWeights(ws[0..4]);
        expect_eq(maybe == null, false);
        let wi: WeightedIndex = maybe ?? WeightedIndex {
            cumulative: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
            count: 1,
            total: 1.0,
        };

        let counts: [4]u32 = [0, 0, 0, 0];
        let n: u32 = 4000;
        let i: u32 = 0;
        while (i < n) {
            let idx: usize = wi.sample(&rng);
            counts[idx] = counts[idx] + 1;
            i = i + 1;
        }
        expect_eq(counts[2], 0);
        let p0: f64 = (counts[0] as f64) / (n as f64);
        let p1: f64 = (counts[1] as f64) / (n as f64);
        let p3: f64 = (counts[3] as f64) / (n as f64);
        expect_eq(p0 > 0.07 && p0 < 0.13, true);
        expect_eq(p1 > 0.17 && p1 < 0.23, true);
        expect_eq(p3 > 0.67 && p3 < 0.73, true);
    });

    test("6. @cap/rand-distr — strict parameter validation rejects invalid inputs", () => {
        expect_eq(UniformRange.new(5.0, 5.0) == null, true);
        expect_eq(UniformRange.new(10.0, 2.0) == null, true);
        expect_eq(NormalDistr.new(0.0, -1.0) == null, true);
        expect_eq(ExpDistr.new(0.0) == null, true);
        expect_eq(ExpDistr.new(-2.0) == null, true);
        expect_eq(BernoulliDistr.new(-0.1) == null, true);
        expect_eq(BernoulliDistr.new(1.1) == null, true);

        let empty_w: [1]f64 = [0.0];
        expect_eq(WeightedIndex.fromWeights(empty_w[0..0]) == null, true);
        expect_eq(WeightedIndex.fromWeights(empty_w[0..1]) == null, true);
        let neg_w: [2]f64 = [1.0, -0.5];
        expect_eq(WeightedIndex.fromWeights(neg_w[0..2]) == null, true);
    });
}
