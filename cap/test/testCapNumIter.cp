import { test, expect_eq } from "cap:test";
import {
    RangeStep,
    RangeStepInclusive,
    RangeStepU64,
    rangeStep,
    rangeStepInclusive,
    rangeStepU64,
    collectRangeStep
} from "../../packages/num-iter/src/index.cp";

fn main() {
    test("1. @cap/num-iter — RangeStep positive step & collectRangeStep", () => {
        let r: RangeStep = rangeStep(0, 10, 3);
        expect_eq(r.isDone(), false);
        expect_eq(r.remaining(), 4);

        let buf: [8]i64 = [0, 0, 0, 0, 0, 0, 0, 0];
        let n: usize = collectRangeStep(&r, buf[0..8]);
        expect_eq(n, 4);
        expect_eq(buf[0], 0);
        expect_eq(buf[1], 3);
        expect_eq(buf[2], 6);
        expect_eq(buf[3], 9);
        expect_eq(r.isDone(), true);
        expect_eq(r.remaining(), 0);
        expect_eq(r.next() == null, true);
    });

    test("2. @cap/num-iter — RangeStep negative step & empty/zero-step edge cases", () => {
        let r: RangeStep = rangeStep(10, 3, -3);
        expect_eq(r.remaining(), 3);
        expect_eq(r.next() ?? -1, 10);
        expect_eq(r.next() ?? -1, 7);
        expect_eq(r.next() ?? -1, 4);
        expect_eq(r.next() == null, true);
        expect_eq(r.isDone(), true);

        let empty1: RangeStep = rangeStep(5, 5, 1);
        expect_eq(empty1.isDone(), true);
        expect_eq(empty1.remaining(), 0);

        let empty2: RangeStep = rangeStep(0, 10, 0);
        expect_eq(empty2.isDone(), true);
        expect_eq(empty2.next() == null, true);

        let empty3: RangeStep = rangeStep(0, 10, -2);
        expect_eq(empty3.isDone(), true);
    });

    test("3. @cap/num-iter — RangeStep O(1) nth() skipping", () => {
        let r: RangeStep = rangeStep(2, 20, 3);
        expect_eq(r.remaining(), 6);
        expect_eq(r.nth(2) ?? -1, 8);
        expect_eq(r.remaining(), 3);
        expect_eq(r.nth(0) ?? -1, 11);
        expect_eq(r.nth(1) ?? -1, 17);
        expect_eq(r.isDone(), true);
        expect_eq(r.nth(0) == null, true);

        let rneg: RangeStep = rangeStep(10, -10, -4);
        expect_eq(rneg.nth(3) ?? 0, -2);
        expect_eq(rneg.next() ?? 0, -6);
        expect_eq(rneg.next() == null, true);
    });

    test("4. @cap/num-iter — RangeStepInclusive positive & negative bounds", () => {
        let r1: RangeStepInclusive = rangeStepInclusive(0, 10, 5);
        expect_eq(r1.remaining(), 3);
        expect_eq(r1.next() ?? -1, 0);
        expect_eq(r1.next() ?? -1, 5);
        expect_eq(r1.next() ?? -1, 10);
        expect_eq(r1.next() == null, true);
        expect_eq(r1.isDone(), true);

        let r2: RangeStepInclusive = rangeStepInclusive(10, 4, -3);
        expect_eq(r2.remaining(), 3);
        expect_eq(r2.next() ?? -1, 10);
        expect_eq(r2.next() ?? -1, 7);
        expect_eq(r2.next() ?? -1, 4);
        expect_eq(r2.next() == null, true);

        let single: RangeStepInclusive = rangeStepInclusive(7, 7, 10);
        expect_eq(single.remaining(), 1);
        expect_eq(single.next() ?? -1, 7);
        expect_eq(single.isDone(), true);
    });

    test("5. @cap/num-iter — RangeStepInclusive i64::MAX / i64::MIN overflow safety", () => {
        let max_i64: i64 = 9223372036854775807;
        let min_i64: i64 = -9223372036854775807 - 1;

        let hi: RangeStepInclusive = rangeStepInclusive(max_i64 - 4, max_i64, 3);
        expect_eq(hi.remaining(), 2);
        expect_eq(hi.next() ?? 0, max_i64 - 4);
        expect_eq(hi.next() ?? 0, max_i64 - 1);
        expect_eq(hi.isDone(), true);
        expect_eq(hi.next() == null, true);

        let lo: RangeStepInclusive = rangeStepInclusive(min_i64 + 2, min_i64, -2);
        expect_eq(lo.remaining(), 2);
        expect_eq(lo.next() ?? 0, min_i64 + 2);
        expect_eq(lo.next() ?? 0, min_i64);
        expect_eq(lo.isDone(), true);
        expect_eq(lo.next() == null, true);
    });

    test("6. @cap/num-iter — RangeStepU64 & U64_MAX overflow safety", () => {
        let r: RangeStepU64 = rangeStepU64(1, 10, 4);
        expect_eq(r.remaining(), 3);
        expect_eq(r.next() ?? 0, 1);
        expect_eq(r.nth(1) ?? 0, 9);
        expect_eq(r.isDone(), true);

        let u64_max: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        let top: RangeStepU64 = rangeStepU64(u64_max - 5, u64_max, 4);
        expect_eq(top.remaining(), 2);
        expect_eq(top.next() ?? 0, u64_max - 5);
        expect_eq(top.next() ?? 0, u64_max - 1);
        expect_eq(top.isDone(), true);
        expect_eq(top.next() == null, true);
    });
}
