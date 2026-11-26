// cap/test/testCapSegmentTree.cp
// Comprehensive unit tests for @cap/segment-tree

import { test, expect_eq } from "cap:test";
import {
    SegmentTree,
} from "../../packages/@cap/segment-tree/src/index.cp";

fn main() {
    test("1. @cap/segment-tree — Empty tree & invalid range guards", () => {
        let empty = SegmentTree.init(0 as usize);
        expect_eq(empty.len(), 0 as usize);
        expect_eq(empty.set(0 as usize, 10 as i64), false);
        expect_eq(empty.add(0 as usize, 5 as i64), false);
        expect_eq(empty.get(0 as usize), 0 as i64);
        expect_eq(empty.query(0 as usize, 1 as usize).valid, false);

        const st = SegmentTree.init(4 as usize);
        expect_eq(st.query(2 as usize, 2 as usize).valid, false);
        expect_eq(st.query(3 as usize, 1 as usize).valid, false);
        expect_eq(st.query(4 as usize, 5 as usize).valid, false);
    });

    test("2. @cap/segment-tree — O(N) fromSlice full & sub-range sum/min/max", () => {
        const vals: [6]i64 = [3, 1, 4, 1, 5, 9];
        const st = SegmentTree.fromSlice(vals[0..6]);
        expect_eq(st.len(), 6 as usize);
        expect_eq(st.get(5 as usize), 9 as i64);

        const full = st.query(0 as usize, 6 as usize);
        expect_eq(full.valid, true);
        expect_eq(full.sum, 23 as i64);
        expect_eq(full.min_val, 1 as i64);
        expect_eq(full.max_val, 9 as i64);

        const sub = st.query(2 as usize, 5 as usize);
        expect_eq(sub.valid, true);
        expect_eq(sub.sum, 10 as i64);
        expect_eq(sub.min_val, 1 as i64);
        expect_eq(sub.max_val, 5 as i64);
    });

    test("3. @cap/segment-tree — Negative values rangeMin & rangeMax precision", () => {
        const vals: [5]i64 = [-50, -10, -80, -3, -25];
        const st = SegmentTree.fromSlice(vals[0..5]);
        expect_eq(st.rangeSum(0 as usize, 5 as usize), -168 as i64);
        expect_eq(st.rangeMin(0 as usize, 5 as usize), -80 as i64);
        expect_eq(st.rangeMax(0 as usize, 5 as usize), -3 as i64);
        expect_eq(st.rangeMax(0 as usize, 3 as usize), -10 as i64);
    });

    test("4. @cap/segment-tree — Point set & add updates reflected across ranges", () => {
        const vals: [5]i64 = [10, 20, 30, 40, 50];
        let st = SegmentTree.fromSlice(vals[0..5]);

        expect_eq(st.set(2 as usize, -5 as i64), true);
        expect_eq(st.get(2 as usize), -5 as i64);
        expect_eq(st.rangeMin(0 as usize, 5 as usize), -5 as i64);
        expect_eq(st.rangeSum(0 as usize, 5 as usize), 115 as i64);

        expect_eq(st.add(1 as usize, 100 as i64), true);
        expect_eq(st.get(1 as usize), 120 as i64);
        expect_eq(st.rangeMax(0 as usize, 4 as usize), 120 as i64);
        expect_eq(st.rangeSum(1 as usize, 4 as usize), 155 as i64);
    });

    test("5. @cap/segment-tree — Single-element ranges & full capacity (256) clamping", () => {
        let st = SegmentTree.init(300 as usize); // clamped to 256
        expect_eq(st.len(), 256 as usize);

        let i: usize = 0;
        while (i < 256) {
            expect_eq(st.set(i, (i as i64) - (100 as i64)), true);
            i += 1;
        }
        // Single-element query
        const s0 = st.query(100 as usize, 101 as usize);
        expect_eq(s0.valid, true);
        expect_eq(s0.sum, 0 as i64);
        expect_eq(s0.min_val, 0 as i64);
        expect_eq(s0.max_val, 0 as i64);

        const all = st.query(0 as usize, 256 as usize);
        expect_eq(all.min_val, -100 as i64);
        expect_eq(all.max_val, 155 as i64);
        // Sum(-100..=155) = 256 * (-100 + 155) / 2 = 128 * 55 = 7040
        expect_eq(all.sum, 7040 as i64);
    });

    test("6. @cap/segment-tree — Cross-validation against brute-force O(N) over 50 intervals", () => {
        let arr: [40]i64 = undefined;
        let i: usize = 0;
        while (i < 40) {
            arr[i] = (((i * (37 as usize)) % (97 as usize)) as i64) - (48 as i64);
            i += 1;
        }
        let st = SegmentTree.fromSlice(arr[0..40]);

        // Deterministic LCG generating 50 intervals + intermittent point updates
        let rng: u64 = 12345;
        let q: usize = 0;
        while (q < 50) {
            rng = ((rng * (1103515245 as u64)) + (12345 as u64)) & (0x7FFFFFFF as u64);
            const a = (rng as usize) % (40 as usize);
            rng = ((rng * (1103515245 as u64)) + (12345 as u64)) & (0x7FFFFFFF as u64);
            const b = (rng as usize) % (40 as usize);
            let lo = a;
            let hi = b;
            if (lo > hi) {
                lo = b;
                hi = a;
            }
            hi += 1;

            // Brute-force reference
            let bSum: i64 = 0;
            let bMin = arr[lo];
            let bMax = arr[lo];
            let k = lo;
            while (k < hi) {
                bSum += arr[k];
                if (arr[k] < bMin) {
                    bMin = arr[k];
                }
                if (arr[k] > bMax) {
                    bMax = arr[k];
                }
                k += 1;
            }

            const got = st.query(lo, hi);
            expect_eq(got.valid, true);
            expect_eq(got.sum, bSum);
            expect_eq(got.min_val, bMin);
            expect_eq(got.max_val, bMax);

            // Apply a point mutation every 5 queries
            if ((q % (5 as usize)) == (0 as usize)) {
                const newV = ((q as i64) * (13 as i64)) - (200 as i64);
                arr[lo] = newV;
                * = st.set(lo, newV);
            }
            q += 1;
        }
    });
}
