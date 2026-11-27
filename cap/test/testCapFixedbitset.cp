// cap/test/testCapFixedbitset.cp
// Comprehensive unit tests for @cap/fixedbitset (BitSet256 & BitSet1024)

import { test, expect_eq } from "cap:test";
import { BitSet256, BitSet1024 } from "../../packages/fixedbitset/src/index.cp";

fn main() {
    test("1. @cap/fixedbitset — BitSet256 insert, remove, setRange, countOnes & set ops", () => {
        let a = BitSet256.init();
        expect_eq(a.isEmpty(), true);
        expect_eq(a.insert(5 as usize), true);
        expect_eq(a.insert(5 as usize), false);
        expect_eq(a.insert(200 as usize), true);
        expect_eq(a.contains(5 as usize), true);
        expect_eq(a.contains(200 as usize), true);
        expect_eq(a.countOnes(), 2 as usize);
        expect_eq(a.firstOne() ?? 999, 5 as usize);
        expect_eq(a.nextOne(6 as usize) ?? 999, 200 as usize);

        let b = BitSet256.init();
        b.setRange(198 as usize, 203 as usize);
        expect_eq(b.countOnes(), 5 as usize);
        expect_eq(a.isDisjoint(b), false);

        a.intersectWith(b);
        expect_eq(a.countOnes(), 1 as usize);
        expect_eq(a.contains(200 as usize), true);
    });

    test("2. @cap/fixedbitset — BitSet1024 high-bit operations & clear", () => {
        let bs = BitSet1024.init();
        expect_eq(bs.insert(1023 as usize), true);
        expect_eq(bs.contains(1023 as usize), true);
        expect_eq(bs.countOnes(), 1 as usize);
        bs.clear();
        expect_eq(bs.isEmpty(), true);
    });
}
