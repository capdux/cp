import { test, expect_eq } from "cap:test";
import {
    VECMAP_CAP,
    VecMapI64,
} from "../../packages/vec_map/src/index.cp";

fn main() {
    test("1. @cap/vec-map — empty state, insert & O(1) lookup across lo/hi bitmaps", () => {
        let m: VecMapI64 = VecMapI64.new();
        expect_eq(VECMAP_CAP, 128 as usize);
        expect_eq(m.len(), 0 as usize);
        expect_eq(m.isEmpty(), true);
        expect_eq(m.containsKey(0), false);
        expect_eq(m.get(0) == null, true);

        expect_eq(m.insert(0, 100) == null, true);
        expect_eq(m.insert(63, -42) == null, true);
        expect_eq(m.insert(64, 777) == null, true);
        expect_eq(m.insert(127, 9999) == null, true);

        expect_eq(m.len(), 4 as usize);
        expect_eq(m.isEmpty(), false);
        expect_eq(m.containsKey(0), true);
        expect_eq(m.containsKey(63), true);
        expect_eq(m.containsKey(64), true);
        expect_eq(m.containsKey(127), true);
        expect_eq(m.containsKey(1), false);

        expect_eq(m.get(0).?, 100 as i64);
        expect_eq(m.get(63).?, -42 as i64);
        expect_eq(m.get(64).?, 777 as i64);
        expect_eq(m.get(127).?, 9999 as i64);
    });

    test("2. @cap/vec-map — overwrite returns old value without changing len", () => {
        let m: VecMapI64 = VecMapI64.new();
        expect_eq(m.insert(10, 50) == null, true);
        expect_eq(m.insert(10, 80).?, 50 as i64);
        expect_eq(m.get(10).?, 80 as i64);
        expect_eq(m.len(), 1 as usize);

        expect_eq(m.insert(100, -5) == null, true);
        expect_eq(m.insert(100, 0).?, -5 as i64);
        expect_eq(m.get(100).?, 0 as i64);
        expect_eq(m.len(), 2 as usize);
    });

    test("3. @cap/vec-map — remove & out-of-bounds key safety", () => {
        let m: VecMapI64 = VecMapI64.new();
        * = m.insert(5, 55);
        * = m.insert(70, 700);

        // Out-of-bounds keys (>= 128) are safely ignored
        expect_eq(m.insert(128, 1) == null, true);
        expect_eq(m.insert(999, 2) == null, true);
        expect_eq(m.containsKey(128), false);
        expect_eq(m.get(128) == null, true);
        expect_eq(m.remove(128) == null, true);
        expect_eq(m.len(), 2 as usize);

        // Valid removes
        expect_eq(m.remove(5).?, 55 as i64);
        expect_eq(m.containsKey(5), false);
        expect_eq(m.get(5) == null, true);
        expect_eq(m.remove(5) == null, true);
        expect_eq(m.len(), 1 as usize);

        expect_eq(m.remove(70).?, 700 as i64);
        expect_eq(m.isEmpty(), true);
    });

    test("4. @cap/vec-map — keys() & values() emit in ascending key order", () => {
        let m: VecMapI64 = VecMapI64.new();
        * = m.insert(90, 900);
        * = m.insert(2, 20);
        * = m.insert(64, 640);
        * = m.insert(15, 150);

        let ks: [4]usize = [0; 4];
        let vs: [4]i64 = [0; 4];
        expect_eq(m.keys(ks[0..4]), 4 as usize);
        expect_eq(m.values(vs[0..4]), 4 as usize);

        expect_eq(ks[0], 2 as usize);
        expect_eq(ks[1], 15 as usize);
        expect_eq(ks[2], 64 as usize);
        expect_eq(ks[3], 90 as usize);

        expect_eq(vs[0], 20 as i64);
        expect_eq(vs[1], 150 as i64);
        expect_eq(vs[2], 640 as i64);
        expect_eq(vs[3], 900 as i64);

        // Truncated output slice
        let small_k: [2]usize = [0; 2];
        expect_eq(m.keys(small_k[0..2]), 2 as usize);
        expect_eq(small_k[0], 2 as usize);
        expect_eq(small_k[1], 15 as usize);
    });

    test("5. @cap/vec-map — retainGreaterThan filters sparse entries in place", () => {
        let m: VecMapI64 = VecMapI64.new();
        * = m.insert(1, -10);
        * = m.insert(20, 0);
        * = m.insert(63, 10);
        * = m.insert(64, 11);
        * = m.insert(120, 50);

        const kept: usize = m.retainGreaterThan(10);
        expect_eq(kept, 2 as usize);
        expect_eq(m.len(), 2 as usize);
        expect_eq(m.containsKey(1), false);
        expect_eq(m.containsKey(20), false);
        expect_eq(m.containsKey(63), false);
        expect_eq(m.containsKey(64), true);
        expect_eq(m.containsKey(120), true);
        expect_eq(m.get(64).?, 11 as i64);
        expect_eq(m.get(120).?, 50 as i64);
    });

    test("6. @cap/vec-map — clear() resets map and supports reuse", () => {
        let m: VecMapI64 = VecMapI64.new();
        * = m.insert(3, 30);
        * = m.insert(65, 650);
        expect_eq(m.len(), 2 as usize);

        m.clear();
        expect_eq(m.len(), 0 as usize);
        expect_eq(m.isEmpty(), true);
        expect_eq(m.containsKey(3), false);
        expect_eq(m.containsKey(65), false);
        expect_eq(m.get(3) == null, true);

        // Re-insert after clear
        expect_eq(m.insert(3, 300) == null, true);
        expect_eq(m.get(3).?, 300 as i64);
        expect_eq(m.len(), 1 as usize);
    });
}
