// cap/test/testCapCountMin.cp
// Comprehensive unit tests for @cap/count-min

import { test, expect_eq } from "cap:test";
import {
    CountMinSketch,
} from "../../packages/@cap/count-min/src/index.cp";

fn main() {
    test("1. @cap/count-min — Dimensions, single-key exact counting & clear", () => {
        let cms = CountMinSketch.init();
        expect_eq(cms.width(), 256 as usize);
        expect_eq(cms.depth(), 4 as usize);
        expect_eq(cms.totalCount(), 0 as u64);
        expect_eq(cms.estimate("missing"), 0 as u32);

        cms.increment("alpha");
        cms.add("alpha", 9 as u32);
        cms.add("beta", 25 as u32);

        expect_eq(cms.totalCount(), 35 as u64);
        expect_eq(cms.estimate("alpha"), 10 as u32);
        expect_eq(cms.estimate("beta"), 25 as u32);

        cms.clear();
        expect_eq(cms.totalCount(), 0 as u64);
        expect_eq(cms.estimate("alpha"), 0 as u32);
    });

    test("2. @cap/count-min — Never-underestimate invariant across 150 u64 keys", () => {
        let cms = CountMinSketch.init();
        let i: u64 = 1;
        while (i <= 150) {
            cms.addU64(i * (37 as u64), i as u32);
            i += 1;
        }
        let j: u64 = 1;
        while (j <= 150) {
            const est = cms.estimateU64(j * (37 as u64));
            expect_eq(est >= (j as u32), true);
            j += 1;
        }
    });

    test("3. @cap/count-min — Heavy-hitter separation under long-tail noise", () => {
        let cms = CountMinSketch.init();
        // Inject 200 distinct long-tail noise events (1 each)
        let n: u64 = 1;
        while (n <= 200) {
            cms.addU64(n + (10000 as u64), 1 as u32);
            n += 1;
        }
        // Inject two hot keys
        cms.add("hot:/checkout", 500 as u32);
        cms.add("hot:/search", 300 as u32);

        expect_eq(cms.totalCount(), 1000 as u64);
        expect_eq(cms.estimate("hot:/checkout") >= (500 as u32), true);
        expect_eq(cms.estimate("hot:/checkout") <= (510 as u32), true);
        expect_eq(cms.isHeavyHitter("hot:/checkout", 0.45), true);
        expect_eq(cms.isHeavyHitter("hot:/search", 0.25), true);
        expect_eq(cms.isHeavyHitter("cold:/rare", 0.05), false);
    });

    test("4. @cap/count-min — Distributed merge across two nodes", () => {
        let node1 = CountMinSketch.init();
        let node2 = CountMinSketch.init();

        node1.add("ip:10.0.0.1", 120 as u32);
        node1.add("ip:10.0.0.2", 30 as u32);
        node2.add("ip:10.0.0.1", 80 as u32);
        node2.add("ip:10.0.0.3", 50 as u32);

        node1.merge(node2);
        expect_eq(node1.totalCount(), 280 as u64);
        expect_eq(node1.estimate("ip:10.0.0.1"), 200 as u32);
        expect_eq(node1.estimate("ip:10.0.0.2"), 30 as u32);
        expect_eq(node1.estimate("ip:10.0.0.3"), 50 as u32);
    });

    test("5. @cap/count-min — Saturating u32 addition prevents overflow wrap", () => {
        let cms = CountMinSketch.init();
        cms.add("flood", 0xFFFFFFF0 as u32);
        cms.add("flood", 100 as u32);
        expect_eq(cms.estimate("flood"), 0xFFFFFFFF as u32);
    });

    test("6. @cap/count-min — Empty sketch heavy-hitter guard", () => {
        const empty = CountMinSketch.init();
        expect_eq(empty.isHeavyHitter("anything", 0.01), false);
        expect_eq(empty.estimateU64(42 as u64), 0 as u32);
    });
}
