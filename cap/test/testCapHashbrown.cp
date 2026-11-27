import { test, expect_eq } from "cap:test";
import { HashMapU64, HashSetU64, HASHBROWN_MAX_LOAD } from "../../packages/hashbrown/src/index.cp";

fn main() {
    test("1. @cap/hashbrown — Basic insert, get, containsKey, and replace", () => {
        let m = HashMapU64.new();
        expect_eq(m.isEmpty(), true);
        let r1 = m.insert(10, 100);
        expect_eq(r1.ok, true);
        expect_eq(r1.replaced, false);
        expect_eq(m.get(10).found, true);
        expect_eq(m.get(10).value, 100);
        expect_eq(m.containsKey(10), true);
        expect_eq(m.containsKey(99), false);

        let r2 = m.insert(10, 200);
        expect_eq(r2.ok, true);
        expect_eq(r2.replaced, true);
        expect_eq(r2.old_value, 100);
        expect_eq(m.get(10).value, 200);
        expect_eq(m.len(), 1);
    });

    test("2. @cap/hashbrown — Remove with tombstones and re-insert", () => {
        let m = HashMapU64.new();
        * = m.insert(1, 11);
        * = m.insert(2, 22);
        * = m.insert(3, 33);
        let rm = m.remove(2);
        expect_eq(rm.found, true);
        expect_eq(rm.value, 22);
        expect_eq(m.len(), 2);
        expect_eq(m.containsKey(2), false);
        expect_eq(m.get(3).value, 33);

        // Re-insert reuses tombstone cleanly
        let ri = m.insert(2, 222);
        expect_eq(ri.ok, true);
        expect_eq(ri.replaced, false);
        expect_eq(m.get(2).value, 222);
        expect_eq(m.len(), 3);
    });

    test("3. @cap/hashbrown — Fill to max load (96 items) and reject overflow", () => {
        let m = HashMapU64.new();
        let i: u64 = 0;
        while (i < (HASHBROWN_MAX_LOAD as u64)) {
            let r = m.insert(i * 17 + 5, (i * 3) as i64);
            expect_eq(r.ok, true);
            i = i + 1;
        }
        expect_eq(m.len(), HASHBROWN_MAX_LOAD);
        expect_eq(m.insert(999999, 1).ok, false);

        // Verify all 96 entries retrievable
        let k: u64 = 0;
        while (k < (HASHBROWN_MAX_LOAD as u64)) {
            let g = m.get(k * 17 + 5);
            expect_eq(g.found, true);
            expect_eq(g.value, (k * 3) as i64);
            k = k + 1;
        }
    });

    test("4. @cap/hashbrown — Clear resets all slots cleanly", () => {
        let m = HashMapU64.new();
        * = m.insert(42, -7);
        m.clear();
        expect_eq(m.isEmpty(), true);
        expect_eq(m.containsKey(42), false);
        expect_eq(m.insert(42, 99).replaced, false);
        expect_eq(m.get(42).value, 99);
    });

    test("5. @cap/hashbrown — HashSetU64 insert, contains, remove", () => {
        let s = HashSetU64.new();
        expect_eq(s.insert(100), true);
        expect_eq(s.insert(100), false);
        expect_eq(s.insert(200), true);
        expect_eq(s.len(), 2);
        expect_eq(s.contains(100), true);
        expect_eq(s.contains(300), false);
        expect_eq(s.remove(100), true);
        expect_eq(s.remove(100), false);
        expect_eq(s.len(), 1);
    });

    test("6. @cap/hashbrown — Handles key=0 and negative values accurately", () => {
        let m = HashMapU64.new();
        * = m.insert(0, -999999999);
        expect_eq(m.get(0).found, true);
        expect_eq(m.get(0).value, -999999999);
        expect_eq(m.remove(9999).found, false);
    });
}
