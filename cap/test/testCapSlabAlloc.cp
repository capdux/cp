import { test, expect_eq } from "cap:test";
import { SlabI64, SLAB_CAP } from "../../packages/slab/src/index.cp";

fn main() {
    test("1. @cap/slab-alloc — insert, get, set & contains", () => {
        let s = SlabI64.new();
        expect_eq(s.isEmpty(), true);
        expect_eq(s.capacity(), SLAB_CAP);
        let k0 = s.insert(100) ?? 999;
        let k1 = s.insert(200) ?? 999;
        expect_eq(k0, 0);
        expect_eq(k1, 1);
        expect_eq(s.len(), 2);
        expect_eq(s.get(k0) ?? -1, 100);
        expect_eq(s.set(k1, 250), true);
        expect_eq(s.get(k1) ?? -1, 250);
        expect_eq(s.contains(k1), true);
        expect_eq(s.contains(2), false);
    });

    test("2. @cap/slab-alloc — O(1) free-list reuse & vacantKey prediction", () => {
        let s = SlabI64.new();
        let k0 = s.insert(10) ?? 999;
        let k1 = s.insert(20) ?? 999;
        let k2 = s.insert(30) ?? 999;
        expect_eq(s.remove(k1) ?? -1, 20);
        expect_eq(s.contains(k1), false);
        expect_eq(s.vacantKey() ?? 999, k1);
        let k_reused = s.insert(22) ?? 999;
        expect_eq(k_reused, k1);
        expect_eq(s.get(k_reused) ?? -1, 22);
        expect_eq(s.get(k0) ?? -1, 10);
        expect_eq(s.get(k2) ?? -1, 30);
    });

    test("3. @cap/slab-alloc — Double remove & out-of-bounds safety", () => {
        let s = SlabI64.new();
        let k0 = s.insert(42) ?? 999;
        expect_eq(s.remove(k0) ?? -1, 42);
        expect_eq(s.remove(k0) == null, true);
        expect_eq(s.get(k0) == null, true);
        expect_eq(s.set(k0, 99), false);
        expect_eq(s.get(999) == null, true);
    });

    test("4. @cap/slab-alloc — nextOccupied iteration over sparse slab", () => {
        let s = SlabI64.new();
        s.insert(1); s.insert(2); s.insert(3); s.insert(4);
        s.remove(0);
        s.remove(2);
        let p1 = s.nextOccupied(0) ?? 999;
        let p2 = s.nextOccupied(p1 + 1) ?? 999;
        expect_eq(p1, 1);
        expect_eq(p2, 3);
        expect_eq(s.nextOccupied(p2 + 1) == null, true);
    });

    test("5. @cap/slab-alloc — Full capacity protection", () => {
        let s = SlabI64.new();
        let i: usize = 0;
        while (i < SLAB_CAP) {
            expect_eq(s.insert(i as i64) != null, true);
            i = i + 1;
        }
        expect_eq(s.isFull(), true);
        expect_eq(s.vacantKey() == null, true);
        expect_eq(s.insert(999) == null, true);
    });

    test("6. @cap/slab-alloc — clear resets count & watermark cleanly", () => {
        let s = SlabI64.new();
        s.insert(7); s.insert(8);
        s.clear();
        expect_eq(s.isEmpty(), true);
        expect_eq(s.vacantKey() ?? 999, 0);
        expect_eq(s.insert(9) ?? 999, 0);
    });
}
