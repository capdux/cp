import { test, expect_eq } from "cap:test";
import { TinyVecI64, TinyVecU32, TV_CAP } from "../../packages/tinyvec/src/index.cp";

fn main() {
    test("1. @cap/tinyvec — push, pop, first, last & capacity bounds", () => {
        let v = TinyVecI64.new();
        expect_eq(v.isEmpty(), true);
        expect_eq(v.capacity(), TV_CAP);
        expect_eq(v.push(10), true);
        expect_eq(v.push(20), true);
        expect_eq(v.push(30), true);
        expect_eq(v.len(), 3);
        expect_eq(v.first() ?? -1, 10);
        expect_eq(v.last() ?? -1, 30);
        expect_eq(v.sum(), 60);
        expect_eq(v.pop() ?? -1, 30);
        expect_eq(v.len(), 2);
    });

    test("2. @cap/tinyvec — insert, remove & swapRemove", () => {
        let v = TinyVecI64.new();
        v.push(1); v.push(3); v.push(4);
        expect_eq(v.insert(1, 2), true);
        expect_eq(v.get(1) ?? -1, 2);
        expect_eq(v.remove(1) ?? -1, 2);
        expect_eq(v.len(), 3);
        expect_eq(v.swapRemove(0) ?? -1, 1);
        expect_eq(v.get(0) ?? -1, 4);
        expect_eq(v.len(), 2);
    });

    test("3. @cap/tinyvec — reverse & dedup", () => {
        let v = TinyVecI64.new();
        v.push(1); v.push(1); v.push(2); v.push(2); v.push(2); v.push(3);
        v.dedup();
        expect_eq(v.len(), 3);
        expect_eq(v.get(0) ?? -1, 1);
        expect_eq(v.get(2) ?? -1, 3);
        v.reverse();
        expect_eq(v.get(0) ?? -1, 3);
        expect_eq(v.get(2) ?? -1, 1);
    });

    test("4. @cap/tinyvec — extendFromSlice & binarySearch", () => {
        let v = TinyVecI64.new();
        let arr: [5]i64 = [10, 20, 30, 40, 50];
        let added = v.extendFromSlice(arr[0..5]);
        expect_eq(added, 5);
        expect_eq(v.binarySearch(30) ?? 999, 2);
        expect_eq(v.binarySearch(10) ?? 999, 0);
        expect_eq(v.binarySearch(50) ?? 999, 4);
        expect_eq(v.binarySearch(25) == null, true);
    });

    test("5. @cap/tinyvec — truncate, clear & full capacity overflow protection", () => {
        let v = TinyVecI64.new();
        let i: usize = 0;
        while (i < TV_CAP) {
            expect_eq(v.push(i as i64), true);
            i = i + 1;
        }
        expect_eq(v.isFull(), true);
        expect_eq(v.push(999), false);
        v.truncate(5);
        expect_eq(v.len(), 5);
        v.clear();
        expect_eq(v.isEmpty(), true);
    });

    test("6. @cap/tinyvec — TinyVecU32 push, pop & swapRemove", () => {
        let u = TinyVecU32.new();
        u.push(100); u.push(200); u.push(300);
        expect_eq(u.len(), 3);
        expect_eq(u.swapRemove(0) ?? 0, 100);
        expect_eq(u.get(0) ?? 0, 300);
        expect_eq(u.pop() ?? 0, 200);
        expect_eq(u.len(), 1);
    });
}
