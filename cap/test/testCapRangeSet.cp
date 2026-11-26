// cap/test/testCapRangeSet.cp
// Comprehensive unit tests for @cap/range-set (Rust rangemap / npm merge-ranges parity)

import { test, expect_eq } from "cap:test";
import { RangeSet } from "../../packages/@cap/range-set/src/index.cp";

fn main() {
    test("1. @cap/range-set — empty set & empty/invalid interval handling", () => {
        let rs = RangeSet.init();
        expect_eq(rs.isEmpty(), true);
        expect_eq(rs.len(), 0 as usize);
        expect_eq(rs.totalSpan(), 0 as i64);
        expect_eq(rs.contains(0), false);
        expect_eq(rs.overlaps(0, 10), false);

        // Invalid/empty intervals (start >= end) are ignored
        expect_eq(rs.insert(5, 5), true);
        expect_eq(rs.insert(10, 3), true);
        expect_eq(rs.isEmpty(), true);

        const oob = rs.get(99 as usize);
        expect_eq(oob.start, 0 as i64);
        expect_eq(oob.end, 0 as i64);
    });

    test("2. @cap/range-set — adjacent & overlapping interval automatic coalescing", () => {
        let rs = RangeSet.init();
        expect_eq(rs.insert(0, 10), true);
        expect_eq(rs.insert(10, 20), true); // adjacent -> [0, 20)
        expect_eq(rs.len(), 1 as usize);
        expect_eq(rs.get(0 as usize).start, 0 as i64);
        expect_eq(rs.get(0 as usize).end, 20 as i64);

        expect_eq(rs.insert(25, 35), true);
        expect_eq(rs.len(), 2 as usize);

        // Bridge [15, 28) merges both ranges into [0, 35)
        expect_eq(rs.insert(15, 28), true);
        expect_eq(rs.len(), 1 as usize);
        expect_eq(rs.get(0 as usize).start, 0 as i64);
        expect_eq(rs.get(0 as usize).end, 35 as i64);
        expect_eq(rs.totalSpan(), 35 as i64);
    });

    test("3. @cap/range-set — unordered insertions maintain strict ascending order", () => {
        let rs = RangeSet.init();
        * = rs.insert(50, 60);
        * = rs.insert(10, 20);
        * = rs.insert(30, 40);

        expect_eq(rs.len(), 3 as usize);
        expect_eq(rs.get(0 as usize).start, 10 as i64);
        expect_eq(rs.get(1 as usize).start, 30 as i64);
        expect_eq(rs.get(2 as usize).start, 50 as i64);
        expect_eq(rs.totalSpan(), 30 as i64);
    });

    test("4. @cap/range-set — remove truncation, full deletion & mid-interval split", () => {
        let rs = RangeSet.init();
        * = rs.insert(0, 100);

        // Mid-interval split: [0, 100) - [20, 40) -> [0, 20), [40, 100)
        expect_eq(rs.remove(20, 40), true);
        expect_eq(rs.len(), 2 as usize);
        expect_eq(rs.get(0 as usize).start, 0 as i64);
        expect_eq(rs.get(0 as usize).end, 20 as i64);
        expect_eq(rs.get(1 as usize).start, 40 as i64);
        expect_eq(rs.get(1 as usize).end, 100 as i64);

        // Cross-interval truncation: remove [10, 60) -> [0, 10), [60, 100)
        expect_eq(rs.remove(10, 60), true);
        expect_eq(rs.len(), 2 as usize);
        expect_eq(rs.get(0 as usize).end, 10 as i64);
        expect_eq(rs.get(1 as usize).start, 60 as i64);

        // Full deletion of first range + partial right: remove [0, 80) -> [80, 100)
        expect_eq(rs.remove(0, 80), true);
        expect_eq(rs.len(), 1 as usize);
        expect_eq(rs.get(0 as usize).start, 80 as i64);
        expect_eq(rs.get(0 as usize).end, 100 as i64);

        // Complete wipe: remove [80, 100) -> empty
        expect_eq(rs.remove(80, 100), true);
        expect_eq(rs.isEmpty(), true);
    });

    test("5. @cap/range-set — contains, containsRange, overlaps & totalSpan", () => {
        let rs = RangeSet.init();
        * = rs.insert(10, 20);
        * = rs.insert(30, 50);

        expect_eq(rs.contains(9), false);
        expect_eq(rs.contains(10), true);
        expect_eq(rs.contains(19), true);
        expect_eq(rs.contains(20), false); // half-open!
        expect_eq(rs.contains(35), true);

        expect_eq(rs.containsRange(10, 20), true);
        expect_eq(rs.containsRange(32, 48), true);
        expect_eq(rs.containsRange(15, 35), false); // gap [20, 30)
        expect_eq(rs.containsRange(5, 5), true);    // empty range

        expect_eq(rs.overlaps(0, 10), false);  // touching at 10 is disjoint
        expect_eq(rs.overlaps(0, 11), true);
        expect_eq(rs.overlaps(20, 30), false); // inside gap
        expect_eq(rs.overlaps(25, 31), true);
        expect_eq(rs.totalSpan(), 30 as i64);
    });

    test("6. @cap/range-set — negative coordinates & clear", () => {
        let rs = RangeSet.init();
        * = rs.insert(-50, -20);
        * = rs.insert(-25, 10); // merges to [-50, 10)
        expect_eq(rs.len(), 1 as usize);
        expect_eq(rs.get(0 as usize).start, -50 as i64);
        expect_eq(rs.get(0 as usize).end, 10 as i64);
        expect_eq(rs.totalSpan(), 60 as i64);
        expect_eq(rs.contains(-50), true);
        expect_eq(rs.contains(-1), true);
        expect_eq(rs.contains(10), false);

        rs.clear();
        expect_eq(rs.isEmpty(), true);
        expect_eq(rs.totalSpan(), 0 as i64);
    });
}
