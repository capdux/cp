// cap/test/testCapRoaringBitmap.cp
// Comprehensive unit tests for @cap/roaring (RoaringBitmap operations)

import { test, expect_eq } from "cap:test";
import {
    RoaringBitmap,
} from "../../packages/roaring/src/index.cp";

fn main() {
    test("1. @cap/roaring — cross high-16-bit u32 insert, contains & remove", () => {
        let bm = RoaringBitmap.init();
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

    test("2. @cap/roaring — min, max & rank across multiple chunks", () => {
        let bm = RoaringBitmap.init();
        expect_eq(bm.min(), -1 as i64);
        expect_eq(bm.max(), -1 as i64);
        expect_eq(bm.rank(100 as u32), 0 as u64);

        * = bm.insert(0x00050010 as u32);
        * = bm.insert(0x00000003 as u32);
        * = bm.insert(0x00050002 as u32);
        * = bm.insert(0xFFFF0009 as u32);
        * = bm.insert(0x00000020 as u32);

        expect_eq(bm.min(), 0x00000003 as i64);
        expect_eq(bm.max(), 0xFFFF0009 as i64);

        expect_eq(bm.rank(0x00000002 as u32), 0 as u64);
        expect_eq(bm.rank(0x00000003 as u32), 1 as u64);
        expect_eq(bm.rank(0x00000020 as u32), 2 as u64);
        expect_eq(bm.rank(0x00050002 as u32), 3 as u64);
        expect_eq(bm.rank(0x00050010 as u32), 4 as u64);
        expect_eq(bm.rank(0xFFFFFFFF as u32), 5 as u64);
    });

    test("3. @cap/roaring — unionWith, intersectWith & differenceWith", () => {
        let a = RoaringBitmap.init();
        * = a.insert(1 as u32);
        * = a.insert(0x00010002 as u32);
        * = a.insert(0x00020003 as u32);

        let b = RoaringBitmap.init();
        * = b.insert(0x00010002 as u32);
        * = b.insert(0x00020003 as u32);
        * = b.insert(0xFFFF0004 as u32);

        const u = a.unionWith(b);
        expect_eq(u.len(), 4 as u64);
        expect_eq(u.contains(1 as u32), true);
        expect_eq(u.contains(0xFFFF0004 as u32), true);

        const inter = a.intersectWith(b);
        expect_eq(inter.len(), 2 as u64);
        expect_eq(inter.contains(1 as u32), false);
        expect_eq(inter.contains(0x00010002 as u32), true);
        expect_eq(inter.contains(0x00020003 as u32), true);

        const diff = a.differenceWith(b);
        expect_eq(diff.len(), 1 as u64);
        expect_eq(diff.contains(1 as u32), true);
        expect_eq(diff.contains(0x00010002 as u32), false);
    });

    test("4. @cap/roaring — clear & reuse across high-16-bit boundaries", () => {
        let bm = RoaringBitmap.init();
        * = bm.insert(0x0000FFFC as u32);
        * = bm.insert(0x0000FFFF as u32);
        * = bm.insert(0x00010000 as u32);
        * = bm.insert(0x00010003 as u32);
        expect_eq(bm.len(), 4 as u64);
        expect_eq(bm.container_count, 2 as usize);

        bm.clear();
        expect_eq(bm.isEmpty(), true);
        expect_eq(bm.len(), 0 as u64);
        expect_eq(bm.insert(0x00010000 as u32), true);
        expect_eq(bm.len(), 1 as u64);
    });
}
