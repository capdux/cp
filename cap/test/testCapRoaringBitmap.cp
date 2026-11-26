// cap/test/testCapRoaringBitmap.cp
// Comprehensive unit tests for @cap/roaring-bitmap

import { test, expect_eq } from "cap:test";
import {
    RoaringBitmap,
} from "../../packages/@cap/roaring-bitmap/src/index.cp";

fn main() {
    test("1. @cap/roaring-bitmap — cross high-16-bit u32 insert, contains & remove", () => {
        let bm = RoaringBitmap.new();
        expect_eq(bm.isEmpty(), true);
        expect_eq(bm.len(), 0 as u64);

        expect_eq(bm.insert(0x00010005 as u32), true);
        expect_eq(bm.insert(0x00010005 as u32), false); // duplicate
        expect_eq(bm.insert(0xFFFF0001 as u32), true);
        expect_eq(bm.insert(0x00000007 as u32), true);
        expect_eq(bm.insert(0xFFFF0000 as u32), true);
        expect_eq(bm.len(), 4 as u64);
        expect_eq(bm.isEmpty(), false);

        expect_eq(bm.contains(0x00010005 as u32), true);
        expect_eq(bm.contains(0xFFFF0001 as u32), true);
        expect_eq(bm.contains(0x00010006 as u32), false);
        expect_eq(bm.contains(0x00020005 as u32), false);

        expect_eq(bm.remove(0x00010005 as u32), true);
        expect_eq(bm.remove(0x00010005 as u32), false);
        expect_eq(bm.contains(0x00010005 as u32), false);
        expect_eq(bm.len(), 3 as u64);
    });

    test("2. @cap/roaring-bitmap — min, max & rank across multiple chunks", () => {
        let bm = RoaringBitmap.new();
        expect_eq(bm.min() == null, true);
        expect_eq(bm.max() == null, true);
        expect_eq(bm.rank(100 as u32), 0 as u64);

        * = bm.insert(0x00050010 as u32);
        * = bm.insert(0x00000003 as u32);
        * = bm.insert(0x00050002 as u32);
        * = bm.insert(0xFFFF0009 as u32);
        * = bm.insert(0x00000020 as u32);

        expect_eq(bm.min() ?? (0 as u32), 0x00000003 as u32);
        expect_eq(bm.max() ?? (0 as u32), 0xFFFF0009 as u32);

        expect_eq(bm.rank(0x00000002 as u32), 0 as u64);
        expect_eq(bm.rank(0x00000003 as u32), 1 as u64);
        expect_eq(bm.rank(0x00000020 as u32), 2 as u64);
        expect_eq(bm.rank(0x00050002 as u32), 3 as u64);
        expect_eq(bm.rank(0x00050010 as u32), 4 as u64);
        expect_eq(bm.rank(0xFFFFFFFF as u32), 5 as u64);
    });

    test("3. @cap/roaring-bitmap — insertRange spanning chunk boundary", () => {
        let bm = RoaringBitmap.new();
        // Spans 0x0000FFFC..0x00010004 (8 elements across chunk 0 and chunk 1)
        let added = bm.insertRange(0x0000FFFC as u32, 0x00010004 as u32);
        expect_eq(added, 8 as u64);
        expect_eq(bm.len(), 8 as u64);
        expect_eq(bm.contains(0x0000FFFB as u32), false);
        expect_eq(bm.contains(0x0000FFFC as u32), true);
        expect_eq(bm.contains(0x0000FFFF as u32), true);
        expect_eq(bm.contains(0x00010000 as u32), true);
        expect_eq(bm.contains(0x00010003 as u32), true);
        expect_eq(bm.contains(0x00010004 as u32), false);

        // Re-inserting overlapping range counts only newly added elements.
        let added2 = bm.insertRange(0x00010000 as u32, 0x00010006 as u32);
        expect_eq(added2, 2 as u64);
        expect_eq(bm.len(), 10 as u64);

        // Empty/inverted range adds 0.
        expect_eq(bm.insertRange(100 as u32, 100 as u32), 0 as u64);
        expect_eq(bm.insertRange(200 as u32, 100 as u32), 0 as u64);
    });

    test("4. @cap/roaring-bitmap — unionWith, intersectWith & differenceWith", () => {
        let a = RoaringBitmap.new();
        * = a.insert(1 as u32);
        * = a.insert(0x00010002 as u32);
        * = a.insert(0x00020003 as u32);

        let b = RoaringBitmap.new();
        * = b.insert(0x00010002 as u32);
        * = b.insert(0x00020003 as u32);
        * = b.insert(0xFFFF0004 as u32);

        let u = a;
        u.unionWith(b);
        expect_eq(u.len(), 4 as u64);
        expect_eq(u.contains(1 as u32), true);
        expect_eq(u.contains(0xFFFF0004 as u32), true);

        let inter = a;
        inter.intersectWith(b);
        expect_eq(inter.len(), 2 as u64);
        expect_eq(inter.contains(1 as u32), false);
        expect_eq(inter.contains(0x00010002 as u32), true);
        expect_eq(inter.contains(0x00020003 as u32), true);

        let diff = a;
        diff.differenceWith(b);
        expect_eq(diff.len(), 1 as u64);
        expect_eq(diff.contains(1 as u32), true);
        expect_eq(diff.contains(0x00010002 as u32), false);
    });

    test("5. @cap/roaring-bitmap — isSubset & isDisjoint predicates", () => {
        let small = RoaringBitmap.new();
        * = small.insert(10 as u32);
        * = small.insert(0x00030001 as u32);

        let big = RoaringBitmap.new();
        * = big.insert(10 as u32);
        * = big.insert(20 as u32);
        * = big.insert(0x00030001 as u32);

        let other = RoaringBitmap.new();
        * = other.insert(0x00040001 as u32);

        expect_eq(small.isSubset(big), true);
        expect_eq(big.isSubset(small), false);
        expect_eq(small.isDisjoint(big), false);
        expect_eq(small.isDisjoint(other), true);
    });

    test("6. @cap/roaring-bitmap — clear resets all state & allows clean reuse", () => {
        let bm = RoaringBitmap.new();
        * = bm.insert(0x00010001 as u32);
        * = bm.insert(0x00020002 as u32);
        expect_eq(bm.len(), 2 as u64);

        bm.clear();
        expect_eq(bm.isEmpty(), true);
        expect_eq(bm.len(), 0 as u64);
        expect_eq(bm.min() == null, true);
        expect_eq(bm.max() == null, true);
        expect_eq(bm.contains(0x00010001 as u32), false);

        expect_eq(bm.insert(0xABCDEF01 as u32), true);
        expect_eq(bm.len(), 1 as u64);
        expect_eq(bm.min() ?? (0 as u32), 0xABCDEF01 as u32);
        expect_eq(bm.max() ?? (0 as u32), 0xABCDEF01 as u32);
    });
}
