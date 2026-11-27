// cap/test/testCapHyperloglog.cp
// Comprehensive unit tests for @cap/hyperloglog (HyperLogLog / Redis PF* parity)

import { test, expect_eq } from "cap:test";
import { HyperLogLog256 } from "../../packages/hyperloglog/src/index.cp";

fn main() {
    test("1. @cap/hyperloglog — empty sketch returns 0 & metadata checks", () => {
        let hll = HyperLogLog256.init();
        expect_eq(hll.registerCount(), 256 as usize);
        expect_eq(hll.count(), 0 as u64);
        const err = hll.relativeError();
        expect_eq(err > 0.064 && err < 0.066, true);
    });

    test("2. @cap/hyperloglog — duplicate additions keep cardinality stable", () => {
        let hll = HyperLogLog256.init();
        let i: usize = 0;
        while (i < 500) {
            hll.add("same-user-id");
            hll.addU64(42 as u64);
            i += 1;
        }
        const est = hll.count();
        expect_eq(est >= (1 as u64) && est <= (3 as u64), true);
    });

    test("3. @cap/hyperloglog — small cardinality LinearCounting accuracy (50 distinct)", () => {
        let hll = HyperLogLog256.init();
        let i: u64 = 1;
        while (i <= 50) {
            hll.addU64(i);
            i += 1;
        }
        const est = hll.count();
        // With LinearCounting on m=256, 50 distinct items is typically within +/-15%
        expect_eq(est >= (40 as u64) && est <= (62 as u64), true);
    });

    test("4. @cap/hyperloglog — medium cardinality accuracy (2000 distinct items)", () => {
        let hll = HyperLogLog256.init();
        let i: u64 = 0;
        while (i < 2000) {
            hll.addU64(i);
            i += 1;
        }
        const est = hll.count();
        // Standard error is 6.5% -> 2000 should comfortably fall within [1600, 2400]
        expect_eq(est >= (1600 as u64) && est <= (2400 as u64), true);
    });

    test("5. @cap/hyperloglog — distributed merge equals single-node union", () => {
        let node_a = HyperLogLog256.init();
        let node_b = HyperLogLog256.init();
        let combined = HyperLogLog256.init();

        let i: u64 = 0;
        while (i < 1000) {
            node_a.addU64(i);
            combined.addU64(i);
            i += 1;
        }
        // Overlapping range [500..1500) on node_b
        i = 500;
        while (i < 1500) {
            node_b.addU64(i);
            combined.addU64(i);
            i += 1;
        }

        node_a.merge(node_b);
        expect_eq(node_a.count(), combined.count());
        const est = node_a.count();
        expect_eq(est >= (1200 as u64) && est <= (1800 as u64), true);
    });

    test("6. @cap/hyperloglog — clear resets all registers to 0", () => {
        let hll = HyperLogLog256.init();
        hll.add("alpha");
        hll.add("beta");
        hll.add("gamma");
        expect_eq(hll.count() > (0 as u64), true);

        hll.clear();
        expect_eq(hll.count(), 0 as u64);
    });
}
