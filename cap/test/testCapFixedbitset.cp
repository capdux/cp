import { test, expect_eq } from "cap:test";
import { FixedBitSet } from "../../packages/fixedbitset/src/index.cp";

fn main() {
    test("1. @cap/fixedbitset — insert, contains, remove & toggle", () => {
        let bs = FixedBitSet.withCapacity(128);
        expect_eq(bs.len(), 128);
        expect_eq(bs.isClear(), true);
        expect_eq(bs.insert(0), true);
        expect_eq(bs.insert(0), false);
        expect_eq(bs.insert(63), true);
        expect_eq(bs.insert(64), true);
        expect_eq(bs.insert(127), true);
        expect_eq(bs.insert(128), false);
        expect_eq(bs.contains(64), true);
        expect_eq(bs.countOnes(), 4);
        bs.toggle(63);
        expect_eq(bs.contains(63), false);
        expect_eq(bs.remove(64), true);
        expect_eq(bs.countOnes(), 2);
    });

    test("2. @cap/fixedbitset — insertRange & clear", () => {
        let bs = FixedBitSet.withCapacity(200);
        bs.insertRange(60, 70);
        expect_eq(bs.countOnes(), 10);
        expect_eq(bs.contains(59), false);
        expect_eq(bs.contains(60), true);
        expect_eq(bs.contains(69), true);
        expect_eq(bs.contains(70), false);
        bs.clear();
        expect_eq(bs.isClear(), true);
    });

    test("3. @cap/fixedbitset — unionWith & intersectWith", () => {
        let a = FixedBitSet.withCapacity(100);
        let b = FixedBitSet.withCapacity(100);
        a.insert(1); a.insert(5); a.insert(70);
        b.insert(5); b.insert(70); b.insert(99);

        let u = a;
        u.unionWith(b);
        expect_eq(u.countOnes(), 4);
        expect_eq(u.contains(1), true);
        expect_eq(u.contains(99), true);

        let inter = a;
        inter.intersectWith(b);
        expect_eq(inter.countOnes(), 2);
        expect_eq(inter.contains(5), true);
        expect_eq(inter.contains(70), true);
    });

    test("4. @cap/fixedbitset — differenceWith & symmetricDifferenceWith", () => {
        let a = FixedBitSet.withCapacity(100);
        let b = FixedBitSet.withCapacity(100);
        a.insert(2); a.insert(10); a.insert(80);
        b.insert(10); b.insert(90);

        let diff = a;
        diff.differenceWith(b);
        expect_eq(diff.countOnes(), 2);
        expect_eq(diff.contains(2), true);
        expect_eq(diff.contains(80), true);

        let sym = a;
        sym.symmetricDifferenceWith(b);
        expect_eq(sym.countOnes(), 3);
        expect_eq(sym.contains(10), false);
        expect_eq(sym.contains(90), true);
    });

    test("5. @cap/fixedbitset — isSubset & isDisjoint", () => {
        let a = FixedBitSet.withCapacity(256);
        let b = FixedBitSet.withCapacity(256);
        let c = FixedBitSet.withCapacity(256);
        a.insert(3); a.insert(150);
        b.insert(3); b.insert(150); b.insert(200);
        c.insert(4); c.insert(151);

        expect_eq(a.isSubset(b), true);
        expect_eq(b.isSubset(a), false);
        expect_eq(a.isDisjoint(c), true);
        expect_eq(a.isDisjoint(b), false);
    });

    test("6. @cap/fixedbitset — nextOne cursor across 64-bit word boundaries", () => {
        let bs = FixedBitSet.withCapacity(300);
        bs.insert(5);
        bs.insert(64);
        bs.insert(255);
        let p1 = bs.nextOne(0) ?? 999;
        let p2 = bs.nextOne(p1 + 1) ?? 999;
        let p3 = bs.nextOne(p2 + 1) ?? 999;
        expect_eq(p1, 5);
        expect_eq(p2, 64);
        expect_eq(p3, 255);
        expect_eq(bs.nextOne(p3 + 1) == null, true);
    });
}
