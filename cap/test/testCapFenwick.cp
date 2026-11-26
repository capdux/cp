// cap/test/testCapFenwick.cp
// Comprehensive unit tests for @cap/fenwick

import { test, expect_eq } from "cap:test";
import {
    FenwickTree,
} from "../../packages/@cap/fenwick/src/index.cp";

fn main() {
    test("1. @cap/fenwick — Empty tree & out-of-bounds guards", () => {
        let empty = FenwickTree.init(0 as usize);
        expect_eq(empty.len(), 0 as usize);
        expect_eq(empty.total(), 0 as i64);
        expect_eq(empty.add(0 as usize, 5 as i64), false);
        expect_eq(empty.set(0 as usize, 5 as i64), false);
        expect_eq(empty.get(0 as usize), 0 as i64);
        expect_eq(empty.lowerBound(1 as i64), 0 as usize);

        let ft = FenwickTree.init(4 as usize);
        expect_eq(ft.add(4 as usize, 1 as i64), false);
        expect_eq(ft.rangeSum(3 as usize, 1 as usize), 0 as i64);
        expect_eq(ft.rangeSum(2 as usize, 2 as usize), 0 as i64);
    });

    test("2. @cap/fenwick — Point add, set & get operations", () => {
        let ft = FenwickTree.init(5 as usize);
        expect_eq(ft.add(0 as usize, 10 as i64), true);
        expect_eq(ft.add(2 as usize, 20 as i64), true);
        expect_eq(ft.add(4 as usize, 30 as i64), true);
        expect_eq(ft.get(0 as usize), 10 as i64);
        expect_eq(ft.get(1 as usize), 0 as i64);
        expect_eq(ft.get(2 as usize), 20 as i64);
        expect_eq(ft.total(), 60 as i64);

        expect_eq(ft.set(2 as usize, 5 as i64), true);
        expect_eq(ft.get(2 as usize), 5 as i64);
        expect_eq(ft.total(), 45 as i64);
    });

    test("3. @cap/fenwick — O(N) fromSlice, prefixSum & rangeSum", () => {
        const vals: [6]i64 = [3, 1, 4, 1, 5, 9];
        const ft = FenwickTree.fromSlice(vals[0..6]);
        expect_eq(ft.len(), 6 as usize);
        expect_eq(ft.prefixSum(0 as usize), 0 as i64);
        expect_eq(ft.prefixSum(1 as usize), 3 as i64);
        expect_eq(ft.prefixSum(3 as usize), 8 as i64);
        expect_eq(ft.prefixSum(6 as usize), 23 as i64);
        expect_eq(ft.total(), 23 as i64);

        expect_eq(ft.rangeSum(1 as usize, 4 as usize), 6 as i64);
        expect_eq(ft.rangeSum(2 as usize, 6 as usize), 19 as i64);
        expect_eq(ft.get(5 as usize), 9 as i64);
    });

    test("4. @cap/fenwick — Negative deltas & reset", () => {
        const vals: [4]i64 = [10, 20, 30, 40];
        let ft = FenwickTree.fromSlice(vals[0..4]);
        expect_eq(ft.add(1 as usize, -25 as i64), true);
        expect_eq(ft.get(1 as usize), -5 as i64);
        expect_eq(ft.rangeSum(0 as usize, 2 as usize), 5 as i64);
        expect_eq(ft.total(), 75 as i64);

        ft.reset();
        expect_eq(ft.total(), 0 as i64);
        expect_eq(ft.get(3 as usize), 0 as i64);
    });

    test("5. @cap/fenwick — O(log N) binary-lifting lowerBound for k-th order statistic", () => {
        // Frequency histogram: bucket 0 has 2, bucket 1 has 0, bucket 2 has 3, bucket 3 has 5 (total 10)
        const freq: [4]i64 = [2, 0, 3, 5];
        const ft = FenwickTree.fromSlice(freq[0..4]);

        expect_eq(ft.lowerBound(1 as i64), 0 as usize);
        expect_eq(ft.lowerBound(2 as i64), 0 as usize);
        expect_eq(ft.lowerBound(3 as i64), 2 as usize);
        expect_eq(ft.lowerBound(5 as i64), 2 as usize);
        expect_eq(ft.lowerBound(6 as i64), 3 as usize);
        expect_eq(ft.lowerBound(10 as i64), 3 as usize);
        expect_eq(ft.lowerBound(11 as i64), 4 as usize);
    });

    test("6. @cap/fenwick — Full capacity (256) clamping & sum stress test", () => {
        let ft = FenwickTree.init(300 as usize); // clamped to 256
        expect_eq(ft.len(), 256 as usize);

        let i: usize = 0;
        while (i < 256) {
            expect_eq(ft.add(i, (i as i64) + (1 as i64)), true);
            i += 1;
        }
        // Sum of 1..=256 = 256 * 257 / 2 = 32896
        expect_eq(ft.total(), 32896 as i64);
        // Sum of 1..=100 = 5050
        expect_eq(ft.prefixSum(100 as usize), 5050 as i64);
        // lowerBound(5050) -> index 99 (value 100)
        expect_eq(ft.lowerBound(5050 as i64), 99 as usize);
        expect_eq(ft.lowerBound(5051 as i64), 100 as usize);
    });
}
