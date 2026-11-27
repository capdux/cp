import { test, expect_eq } from "cap:test";
import { THIN_VEC_CAP, ThinVecI64 } from "../../packages/thin-vec/src/index.cp";

fn main() {
    test("1. @cap/thin-vec — push, pop, first, last & isEmpty", () => {
        let v = ThinVecI64.new();
        expect_eq(v.isEmpty(), true);
        expect_eq(v.capacity(), THIN_VEC_CAP);
        expect_eq(v.push(10), true);
        expect_eq(v.push(20), true);
        expect_eq(v.push(30), true);
        expect_eq(v.len(), 3);
        expect_eq(v.first() ?? 0, 10);
        expect_eq(v.last() ?? 0, 30);
        expect_eq(v.pop() ?? 0, 30);
        expect_eq(v.len(), 2);
    });

    test("2. @cap/thin-vec — insert & order-preserving remove", () => {
        let src: [3]i64 = [1, 3, 4];
        let v = ThinVecI64.fromSlice(src[0..3]);
        expect_eq(v.insert(1, 2), true);
        expect_eq(v.len(), 4);
        expect_eq(v.get(1) ?? 0, 2);
        expect_eq(v.get(3) ?? 0, 4);

        expect_eq(v.remove(1) ?? 0, 2);
        expect_eq(v.len(), 3);
        expect_eq(v.get(1) ?? 0, 3);
    });

    test("3. @cap/thin-vec — O(1) swapRemove replaces slot with last element", () => {
        let src: [4]i64 = [10, 20, 30, 40];
        let v = ThinVecI64.fromSlice(src[0..4]);
        expect_eq(v.swapRemove(1) ?? 0, 20);
        expect_eq(v.len(), 3);
        expect_eq(v.get(1) ?? 0, 40);
        expect_eq(v.swapRemove(9) == null, true);
    });

    test("4. @cap/thin-vec — dedup removes consecutive duplicates in-place", () => {
        let src: [7]i64 = [1, 1, 2, 3, 3, 3, 1];
        let v = ThinVecI64.fromSlice(src[0..7]);
        expect_eq(v.dedup(), 3);
        expect_eq(v.len(), 4);
        expect_eq(v.get(0) ?? 0, 1);
        expect_eq(v.get(1) ?? 0, 2);
        expect_eq(v.get(2) ?? 0, 3);
        expect_eq(v.get(3) ?? 0, 1);
    });

    test("5. @cap/thin-vec — truncate, clear & extendFromSlice", () => {
        let a: [3]i64 = [5, 6, 7];
        let b: [2]i64 = [8, 9];
        let v = ThinVecI64.fromSlice(a[0..3]);
        expect_eq(v.extendFromSlice(b[0..2]), 2);
        expect_eq(v.len(), 5);
        v.truncate(2);
        expect_eq(v.len(), 2);
        v.clear();
        expect_eq(v.isEmpty(), true);
    });

    test("6. @cap/thin-vec — enforces THIN_VEC_CAP capacity limit", () => {
        let v = ThinVecI64.new();
        let i: usize = 0;
        while (i < THIN_VEC_CAP) {
            expect_eq(v.push(i as i64), true);
            i = i + 1;
        }
        expect_eq(v.push(999), false);
        expect_eq(v.insert(0, 999), false);
    });
}
