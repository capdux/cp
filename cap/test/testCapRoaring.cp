// cap/test/testCapRoaring.cp
// Comprehensive unit tests for @cap/roaring

import { test, expect_eq } from "cap:test";
import {
    RoaringBitmap,
} from "../../packages/@cap/roaring/src/index.cp";

fn main() {
    test("1. @cap/roaring — Empty bitmap invariants & min/max sentinel", () => {
        const rb = RoaringBitmap.init();
        expect_eq(rb.isEmpty(), true);
        expect_eq(rb.len(), 0 as u64);
        expect_eq(rb.min(), -1 as i64);
        expect_eq(rb.max(), -1 as i64);
        expect_eq(rb.contains(0 as u32), false);
        expect_eq(rb.rank(100 as u32), 0 as u64);
    });

    test("2. @cap/roaring — Multi-bucket high-16-bit insert & binary search contains", () => {
        let rb = RoaringBitmap.init();
        // Insert out of order across 3 distinct 16-bit high-key buckets
        expect_eq(rb.insert(200000 as u32), true); // hi = 3
        expect_eq(rb.insert(10 as u32), true);     // hi = 0
        expect_eq(rb.insert(70000 as u32), true);  // hi = 1
        expect_eq(rb.insert(5 as u32), true);      // hi = 0
        expect_eq(rb.insert(65536 as u32), true);  // hi = 1

        expect_eq(rb.len(), 5 as u64);
        expect_eq(rb.container_count, 3 as usize);
        expect_eq(rb.min(), 5 as i64);
        expect_eq(rb.max(), 200000 as i64);

        expect_eq(rb.contains(5 as u32), true);
        expect_eq(rb.contains(10 as u32), true);
        expect_eq(rb.contains(65536 as u32), true);
        expect_eq(rb.contains(70000 as u32), true);
        expect_eq(rb.contains(200000 as u32), true);
        expect_eq(rb.contains(70001 as u32), false);
    });

    test("3. @cap/roaring — Duplicate rejection & empty container reclamation on remove", () => {
        let rb = RoaringBitmap.init();
        expect_eq(rb.insert(100 as u32), true);
        expect_eq(rb.insert(100 as u32), false);
        expect_eq(rb.insert(131072 as u32), true); // hi = 2
        expect_eq(rb.container_count, 2 as usize);

        expect_eq(rb.remove(999 as u32), false);
        expect_eq(rb.remove(131072 as u32), true);
        expect_eq(rb.remove(131072 as u32), false);
        // Bucket hi=2 should be reclaimed
        expect_eq(rb.container_count, 1 as usize);
        expect_eq(rb.len(), 1 as u64);
        expect_eq(rb.max(), 100 as i64);
    });

    test("4. @cap/roaring — rank(value) count of elements <= value across buckets", () => {
        let rb = RoaringBitmap.init();
        * = rb.insert(10 as u32);
        * = rb.insert(20 as u32);
        * = rb.insert(65536 as u32);
        * = rb.insert(65540 as u32);
        * = rb.insert(196608 as u32);

        expect_eq(rb.rank(5 as u32), 0 as u64);
        expect_eq(rb.rank(10 as u32), 1 as u64);
        expect_eq(rb.rank(15 as u32), 1 as u64);
        expect_eq(rb.rank(20 as u32), 2 as u64);
        expect_eq(rb.rank(65536 as u32), 3 as u64);
        expect_eq(rb.rank(65539 as u32), 3 as u64);
        expect_eq(rb.rank(65540 as u32), 4 as u64);
        expect_eq(rb.rank(300000 as u32), 5 as u64);
    });

    test("5. @cap/roaring — Multi-bucket unionWith, intersectWith & differenceWith", () => {
        let a = RoaringBitmap.init();
        * = a.insert(1 as u32);
        * = a.insert(2 as u32);
        * = a.insert(70000 as u32);
        * = a.insert(140000 as u32);

        let b = RoaringBitmap.init();
        * = b.insert(2 as u32);
        * = b.insert(3 as u32);
        * = b.insert(70000 as u32);
        * = b.insert(210000 as u32);

        const u = a.unionWith(b);
        expect_eq(u.len(), 6 as u64);
        expect_eq(u.contains(1 as u32), true);
        expect_eq(u.contains(3 as u32), true);
        expect_eq(u.contains(140000 as u32), true);
        expect_eq(u.contains(210000 as u32), true);

        const inter = a.intersectWith(b);
        expect_eq(inter.len(), 2 as u64);
        expect_eq(inter.contains(2 as u32), true);
        expect_eq(inter.contains(70000 as u32), true);
        expect_eq(inter.contains(1 as u32), false);

        const diff = a.differenceWith(b);
        expect_eq(diff.len(), 2 as u64);
        expect_eq(diff.contains(1 as u32), true);
        expect_eq(diff.contains(140000 as u32), true);
        expect_eq(diff.contains(2 as u32), false);
    });

    test("6. @cap/roaring — clear & reuse", () => {
        let rb = RoaringBitmap.init();
        * = rb.insert(42 as u32);
        * = rb.insert(100000 as u32);
        expect_eq(rb.len(), 2 as u64);

        rb.clear();
        expect_eq(rb.isEmpty(), true);
        expect_eq(rb.container_count, 0 as usize);
        expect_eq(rb.min(), -1 as i64);

        expect_eq(rb.insert(7 as u32), true);
        expect_eq(rb.len(), 1 as u64);
        expect_eq(rb.min(), 7 as i64);
    });
}
