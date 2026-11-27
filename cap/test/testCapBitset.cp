// cap/test/testCapBitset.cp
// Comprehensive unit tests for @cap/bitset

import { test, expect_eq } from "cap:test";
import {
    BitSet256,
    BitSet1024,
} from "../../packages/fixedbitset/src/index.cp";

fn main() {
    test("1. @cap/bitset — BitSet256 insert, remove, toggle, contains & bounds safety", () => {
        let bs = BitSet256.init();
        expect_eq(bs.len(), 256 as usize);
        expect_eq(bs.isEmpty(), true);
        expect_eq(bs.countOnes(), 0 as usize);

        expect_eq(bs.insert(0 as usize), true);
        expect_eq(bs.insert(0 as usize), false);
        expect_eq(bs.insert(63 as usize), true);
        expect_eq(bs.insert(64 as usize), true);
        expect_eq(bs.insert(255 as usize), true);
        expect_eq(bs.insert(256 as usize), false);
        expect_eq(bs.contains(256 as usize), false);
        expect_eq(bs.countOnes(), 4 as usize);

        bs.toggle(63 as usize);
        expect_eq(bs.contains(63 as usize), false);
        expect_eq(bs.remove(63 as usize), false);
        expect_eq(bs.remove(64 as usize), true);
        expect_eq(bs.countOnes(), 2 as usize);
    });

    test("2. @cap/bitset — full(), clear() & SWAR popcount", () => {
        let f = BitSet256.full();
        expect_eq(f.isEmpty(), false);
        expect_eq(f.countOnes(), 256 as usize);
        expect_eq(f.contains(128 as usize), true);

        f.clear();
        expect_eq(f.isEmpty(), true);
        expect_eq(f.countOnes(), 0 as usize);
        expect_eq(f.firstOne() == null, true);
    });

    test("3. @cap/bitset — firstOne & nextOne word-skipping scan", () => {
        let bs = BitSet256.init();
        * = bs.insert(5 as usize);
        * = bs.insert(63 as usize);
        * = bs.insert(64 as usize);
        * = bs.insert(200 as usize);

        expect_eq(bs.firstOne() ?? (999 as usize), 5 as usize);
        expect_eq(bs.nextOne(6 as usize) ?? (999 as usize), 63 as usize);
        expect_eq(bs.nextOne(64 as usize) ?? (999 as usize), 64 as usize);
        expect_eq(bs.nextOne(65 as usize) ?? (999 as usize), 200 as usize);
        expect_eq(bs.nextOne(201 as usize) == null, true);
    });

    test("4. @cap/bitset — setRange across 64-bit word boundaries", () => {
        let bs = BitSet256.init();
        bs.setRange(60 as usize, 70 as usize);
        expect_eq(bs.countOnes(), 10 as usize);
        expect_eq(bs.contains(59 as usize), false);
        expect_eq(bs.contains(60 as usize), true);
        expect_eq(bs.contains(64 as usize), true);
        expect_eq(bs.contains(69 as usize), true);
        expect_eq(bs.contains(70 as usize), false);
    });

    test("5. @cap/bitset — Set algebra (union, intersect, difference, symmetricDiff, subset, disjoint)", () => {
        let a = BitSet256.init();
        * = a.insert(1 as usize);
        * = a.insert(2 as usize);
        * = a.insert(3 as usize);

        let b = BitSet256.init();
        * = b.insert(2 as usize);
        * = b.insert(3 as usize);
        * = b.insert(4 as usize);

        let c = BitSet256.init();
        * = c.insert(100 as usize);

        expect_eq(a.isDisjoint(b), false);
        expect_eq(a.isDisjoint(c), true);
        expect_eq(a.isSubsetOf(b), false);

        let u = a;
        u.unionWith(b);
        expect_eq(u.countOnes(), 4 as usize);
        expect_eq(a.isSubsetOf(u), true);
        expect_eq(b.isSubsetOf(u), true);

        let inter = a;
        inter.intersectWith(b);
        expect_eq(inter.countOnes(), 2 as usize);
        expect_eq(inter.contains(2 as usize), true);
        expect_eq(inter.contains(3 as usize), true);

        let diff = a;
        diff.differenceWith(b);
        expect_eq(diff.countOnes(), 1 as usize);
        expect_eq(diff.contains(1 as usize), true);

        let sym = a;
        sym.symmetricDiffWith(b);
        expect_eq(sym.countOnes(), 2 as usize);
        expect_eq(sym.contains(1 as usize), true);
        expect_eq(sym.contains(4 as usize), true);
    });

    test("6. @cap/bitset — BitSet1024 full capacity, high-bit scanning & algebra", () => {
        let big = BitSet1024.init();
        expect_eq(big.len(), 1024 as usize);
        big.setRange(500 as usize, 520 as usize);
        * = big.insert(1023 as usize);
        expect_eq(big.insert(1024 as usize), false);
        expect_eq(big.countOnes(), 21 as usize);
        expect_eq(big.firstOne() ?? (0 as usize), 500 as usize);
        expect_eq(big.nextOne(520 as usize) ?? (0 as usize), 1023 as usize);

        let all = BitSet1024.full();
        expect_eq(all.countOnes(), 1024 as usize);
        expect_eq(big.isSubsetOf(all), true);
        expect_eq(all.equals(BitSet1024.full()), true);
    });
}
