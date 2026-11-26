import { test, expect_eq } from "cap:test";
import { SmallVecU64, SmallVecU8, SMALLVEC_U64_CAP } from "../../packages/@cap/smallvec/src/index.cp";

fn main() {
    test("1. @cap/smallvec — push, pop, get, set and capacity", () => {
        let v = SmallVecU64.new();
        expect_eq(v.isEmpty(), true);
        expect_eq(v.capacity(), SMALLVEC_U64_CAP);
        expect_eq(v.push(10), true);
        expect_eq(v.push(20), true);
        expect_eq(v.push(30), true);
        expect_eq(v.len(), 3);
        expect_eq(v.get(1).value, 20);
        expect_eq(v.set(1, 25), true);
        expect_eq(v.get(1).value, 25);
        expect_eq(v.pop().value, 30);
        expect_eq(v.len(), 2);
    });

    test("2. @cap/smallvec — insert and ordered remove", () => {
        let v = SmallVecU64.new();
        * = v.push(1);
        * = v.push(3);
        expect_eq(v.insert(1, 2), true);
        expect_eq(v.len(), 3);
        expect_eq(v.get(0).value, 1);
        expect_eq(v.get(1).value, 2);
        expect_eq(v.get(2).value, 3);

        let r = v.remove(1);
        expect_eq(r.ok, true);
        expect_eq(r.value, 2);
        expect_eq(v.len(), 2);
        expect_eq(v.get(1).value, 3);
    });

    test("3. @cap/smallvec — O(1) swapRemove", () => {
        let v = SmallVecU64.new();
        * = v.push(100);
        * = v.push(200);
        * = v.push(300);
        let r = v.swapRemove(0);
        expect_eq(r.ok, true);
        expect_eq(r.value, 100);
        expect_eq(v.len(), 2);
        expect_eq(v.get(0).value, 300);
        expect_eq(v.get(1).value, 200);
    });

    test("4. @cap/smallvec — extendFromSlice, dedup and reverse", () => {
        let v = SmallVecU64.new();
        let arr: [6]u64 = [1, 1, 2, 3, 3, 3];
        expect_eq(v.extendFromSlice(arr[0..6]), true);
        v.dedup();
        expect_eq(v.len(), 3);
        expect_eq(v.get(0).value, 1);
        expect_eq(v.get(1).value, 2);
        expect_eq(v.get(2).value, 3);

        v.reverse();
        expect_eq(v.get(0).value, 3);
        expect_eq(v.get(2).value, 1);
        expect_eq(v.contains(2), true);
        expect_eq(v.contains(9), false);
    });

    test("5. @cap/smallvec — SmallVecU8 byte buffer operations", () => {
        let b = SmallVecU8.new();
        expect_eq(b.extendFromSlice("hello"), true);
        expect_eq(b.push(33), true); // '!'
        expect_eq(b.len(), 6);
        let out: [16]u8 = undefined;
        let n = b.copyTo(out[0..16]);
        expect_eq(n, 6);
        expect_eq(out[0], 104);
        expect_eq(out[5], 33);
    });

    test("6. @cap/smallvec — Overflow and out-of-bounds safety", () => {
        let v = SmallVecU64.new();
        let i: usize = 0;
        while (i < SMALLVEC_U64_CAP) {
            expect_eq(v.push(i as u64), true);
            i = i + 1;
        }
        expect_eq(v.isFull(), true);
        expect_eq(v.push(999), false);
        expect_eq(v.get(SMALLVEC_U64_CAP).ok, false);
        expect_eq(v.remove(SMALLVEC_U64_CAP).ok, false);
    });
}
