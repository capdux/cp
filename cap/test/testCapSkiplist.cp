// cap/test/testCapSkiplist.cp
// Comprehensive unit tests for @cap/skiplist

import { test, expect_eq } from "cap:test";
import {
    MAX_NODES,
    SkipEntry,
    SkipList,
} from "../../packages/skiplist/src/index.cp";

fn main() {
    test("1. @cap/skiplist — Empty skip list boundaries", () => {
        let sl = SkipList.init();
        expect_eq(sl.isEmpty(), true);
        expect_eq(sl.len(), 0 as usize);
        expect_eq(sl.contains(10), false);
        expect_eq(sl.get(10).found, false);
        expect_eq(sl.first().found, false);
        expect_eq(sl.last().found, false);
        expect_eq(sl.lowerBound(0).found, false);
        expect_eq(sl.rangeCount(-100, 100), 0 as usize);
    });

    test("2. @cap/skiplist — Out-of-order inserts maintain strict order (first, last, lowerBound)", () => {
        let sl = SkipList.init();
        expect_eq(sl.insert(50, 500), true);
        expect_eq(sl.insert(-10, -100), true);
        expect_eq(sl.insert(20, 200), true);
        expect_eq(sl.insert(100, 1000), true);
        expect_eq(sl.insert(5, 50), true);

        expect_eq(sl.len(), 5 as usize);
        expect_eq(sl.first().key, -10 as i64);
        expect_eq(sl.first().value, -100 as i64);
        expect_eq(sl.last().key, 100 as i64);
        expect_eq(sl.last().value, 1000 as i64);

        const lb1 = sl.lowerBound(15);
        expect_eq(lb1.found, true);
        expect_eq(lb1.key, 20 as i64);

        const lb2 = sl.lowerBound(20);
        expect_eq(lb2.found, true);
        expect_eq(lb2.key, 20 as i64);

        expect_eq(sl.lowerBound(101).found, false);
    });

    test("3. @cap/skiplist — Duplicate key updates in-place without growing len", () => {
        let sl = SkipList.init();
        expect_eq(sl.insert(42, 1), true);
        expect_eq(sl.insert(42, 999), true);
        expect_eq(sl.len(), 1 as usize);
        expect_eq(sl.get(42).value, 999 as i64);
    });

    test("4. @cap/skiplist — Remove & freelist slot reuse (> 200 insert/remove cycles)", () => {
        let sl = SkipList.init();
        let i: i64 = 0;
        while (i < 220) {
            expect_eq(sl.insert(i, i * 10), true);
            expect_eq(sl.contains(i), true);
            expect_eq(sl.remove(i), true);
            expect_eq(sl.contains(i), false);
            expect_eq(sl.remove(i), false);
            i += 1;
        }
        expect_eq(sl.isEmpty(), true);
        expect_eq(sl.insert(7, 70), true);
        expect_eq(sl.get(7).value, 70 as i64);
    });

    test("5. @cap/skiplist — rangeCount inclusive interval queries", () => {
        let sl = SkipList.init();
        let k: i64 = 0;
        while (k < 20) {
            expect_eq(sl.insert(k * 5, k), true); // 0, 5, 10, ..., 95
            k += 1;
        }
        expect_eq(sl.rangeCount(10, 30), 5 as usize);  // 10, 15, 20, 25, 30
        expect_eq(sl.rangeCount(11, 29), 3 as usize);  // 15, 20, 25
        expect_eq(sl.rangeCount(0, 95), 20 as usize);
        expect_eq(sl.rangeCount(50, 10), 0 as usize);  // inverted bounds
        expect_eq(sl.rangeCount(200, 300), 0 as usize);
    });

    test("6. @cap/skiplist — Full 128-node saturation & clear() reset", () => {
        let sl = SkipList.init();
        let i: i64 = 0;
        while (i < (MAX_NODES as i64)) {
            expect_eq(sl.insert(i, i), true);
            i += 1;
        }
        expect_eq(sl.len(), MAX_NODES);
        // 129th distinct key rejected cleanly
        expect_eq(sl.insert(9999, 9999), false);
        // Updating existing key while full still succeeds
        expect_eq(sl.insert(0, 12345), true);
        expect_eq(sl.get(0).value, 12345 as i64);

        sl.clear();
        expect_eq(sl.isEmpty(), true);
        expect_eq(sl.insert(9999, 9999), true);
        expect_eq(sl.first().key, 9999 as i64);
    });
}
